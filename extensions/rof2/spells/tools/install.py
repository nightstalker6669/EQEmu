#!/usr/bin/env python3
"""Plan/apply the spell content overlay; never start or reload game services."""
import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def equal(left, right):
    if isinstance(left, (datetime.date, datetime.datetime)):
        left = str(left)
    if isinstance(left, (int, float)) and isinstance(right, (int, float)):
        return math.isclose(left, right, rel_tol=1e-6, abs_tol=1e-6)
    return left == right


def sql_key(key):
    return " AND ".join(f"`{name}`=%s" for name in key), tuple(key.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db-config", type=Path, required=True,
                        help="Private JSON: host, port, user, password, database")
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--include-estimates", action="store_true",
                        help="Also select the explicitly estimated pet/aura reconstruction")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--backup", type=Path,
                        help="New private backup directory; required with --apply")
    args = parser.parse_args()
    if args.apply and (not args.backup or args.backup.exists()):
        parser.error("--apply requires a new --backup directory")
    try:
        import pymysql
    except ImportError:
        parser.error("Install PyMySQL in a private virtual environment first")
    config = json.loads(args.db_config.read_text())
    allowed = {"host", "port", "user", "password", "database", "unix_socket"}
    if not set(config) <= allowed or not {"user", "database"} <= set(config):
        parser.error("Invalid database configuration fields")
    manifest = json.loads((ROOT / "manifest.json").read_text())
    selected = [s for s in manifest["stages"]
                if not s["contains_estimates"] or args.include_estimates]
    payloads = [json.loads((ROOT / s["path"]).read_text()) for s in selected]
    files = {s["path"]: hashlib.sha256((ROOT / s["path"]).read_bytes()).hexdigest()
             for s in selected}
    source = ROOT / "quests/shardslanding/#Tomekeeper_Kerkaz.lua"
    destination = args.server / "quests/shardslanding/#Tomekeeper_Kerkaz.lua"
    if not destination.parent.is_dir():
        parser.error("Expected existing server quests/shardslanding directory")
    if destination.exists() and destination.read_bytes() != source.read_bytes():
        parser.error("Kerkaz script already exists with different contents; merge it first")
    connection = pymysql.connect(**config, autocommit=False,
                                 cursorclass=pymysql.cursors.DictCursor)
    inserts, updates, counts = [], [], {}
    script_created = False
    committed = False
    try:
        with connection.cursor() as cursor:
            cursor.execute("SELECT GET_LOCK('rof2_spells_install',0) AS locked")
            if cursor.fetchone()["locked"] != 1:
                raise RuntimeError("Another spell content installer holds the lock")
            tables = {table for p in payloads for table in p["inserts"]}
            tables.update(u["table"] for p in payloads for u in p["updates"])
            for table in tables:
                cursor.execute("SHOW TABLE STATUS WHERE Name=%s", (table,))
                status = cursor.fetchone()
                if not status or status["Engine"].lower() != "innodb":
                    raise RuntimeError(f"{table} must exist and use InnoDB")
            # Exact schema preflight: missing custom/modern columns are not silently dropped.
            columns = {}
            for table in tables:
                cursor.execute(f"SHOW COLUMNS FROM `{table}`")
                columns[table] = {r["Field"] for r in cursor.fetchall()}
            for payload in payloads:
                for table, rows in payload["inserts"].items():
                    counts[table] = counts.get(table, 0) + len(rows)
                    pk = manifest["primary_keys"][table]
                    for row in rows:
                        missing = set(row) - columns[table]
                        if missing:
                            raise RuntimeError(f"Unsupported schema {table}: missing {sorted(missing)}")
                        key = {k: row[k] for k in pk}
                        where, values = sql_key(key)
                        cursor.execute(f"SELECT * FROM `{table}` WHERE {where} FOR UPDATE", values)
                        old = cursor.fetchone()
                        if old:
                            differing = [k for k, v in row.items() if not equal(old[k], v)]
                            if differing:
                                raise RuntimeError(f"Conflicting {table} key {key}: {differing}")
                        else:
                            inserts.append((table, row))
                for update in payload["updates"]:
                    table = update["table"]
                    where, values = sql_key(update["key"])
                    cursor.execute(f"SELECT * FROM `{table}` WHERE {where} FOR UPDATE", values)
                    old = cursor.fetchone()
                    if not old or any(old[k] != v for k, v in update["identity"].items()):
                        raise RuntimeError(f"NPC identity differs: {update['key']}")
                    if all(equal(old[k], v) for k, v in update["after"].items()):
                        continue
                    if not all(equal(old[k], v) for k, v in update["before"].items()):
                        raise RuntimeError(f"Vendor binding was customized: {update['key']}")
                    updates.append(update)
            incoming = {}
            for payload in payloads:
                for table, rows in payload["inserts"].items():
                    if table in ("items", "npc_types", "npc_spells", "rewards"):
                        field = "reward_id" if table == "rewards" else "id"
                        incoming.setdefault(table, set()).update(r[field] for r in rows)
            dependencies = {"items": set(), "npc_types": {752146},
                            "npc_spells": set(), "spells_new": set()}
            for payload in payloads:
                for table, rows in payload["inserts"].items():
                    for row in rows:
                        for target, field in {
                            "merchantlist": [("items", "item")],
                            "pets": [("npc_types", "npcID")],
                            "auras": [("npc_types", "npc_type"), ("spells_new", "type"),
                                      ("spells_new", "spell_id")],
                            "npc_types": [("npc_spells", "npc_spells_id")],
                            "npc_spells_entries": [("spells_new", "spellid"),
                                                   ("npc_spells", "npc_spells_id")],
                            "items": [("spells_new", "scrolleffect"), ("spells_new", "clickeffect")],
                            "rewards": [("items", "reward_data_id")],
                        }.get(table, []):
                            value = row[field]
                            if value and value > 0:
                                dependencies[target].add(value)
            for table, ids in dependencies.items():
                needed = sorted(ids - incoming.get(table, set()))
                for start in range(0, len(needed), 400):
                    batch = needed[start:start + 400]
                    cursor.execute(f"SELECT id FROM `{table}` WHERE id IN (" +
                                   ",".join(["%s"] * len(batch)) + ")", batch)
                    missing = set(batch) - {r["id"] for r in cursor.fetchall()}
                    if missing:
                        raise RuntimeError(f"Missing {table} dependencies: {sorted(missing)}")
            cursor.execute("SELECT name FROM npc_types WHERE id=752146")
            if cursor.fetchone()["name"] != "#Tomekeeper_Kerkaz":
                raise RuntimeError("Kerkaz NPC identity differs")
            for name in ("TaskSystem:EnableTaskSystem", "Items:AlwaysReturnHandins"):
                cursor.execute("SELECT rule_value FROM rule_values WHERE rule_name=%s", (name,))
                if any(r["rule_value"].lower() not in ("true", "1") for r in cursor.fetchall()):
                    raise RuntimeError(f"Required rule has a disabled ruleset: {name}")
            # Every spell referenced by the task catalogue must retain the expected era level/rank.
            catalog = json.loads((ROOT / "provenance/kerk-rewards.json").read_text())
            for task in catalog:
                for cls, choices in task["classes"].items():
                    for choice in choices:
                        for item in choice:
                            cursor.execute(f"SELECT name,rank,`classes{int(cls)}` AS level "
                                           "FROM spells_new WHERE id=%s", (item["spell_id"],))
                            spell = cursor.fetchone()
                            if not spell or spell != {"name": item["name"], "rank": task["rank"],
                                                      "level": task["level"]}:
                                raise RuntimeError(f"Spell era/class/rank mismatch: {item['spell_id']}")
            report = {"stages": [s["name"] for s in selected], "content_rows": counts,
                      "new_rows": len(inserts), "vendor_updates": len(updates),
                      "quest_script_new": not destination.exists(), "applied": False,
                      "payload_sha256": files}
            if not args.apply:
                print(json.dumps(report, indent=2))
                connection.rollback()
                return
            # No credentials are copied into the backup/receipt.
            args.backup.mkdir(parents=True, mode=0o700)
            backup = args.backup / "changes.json"
            fd = os.open(backup, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
            with os.fdopen(fd, "w") as handle:
                json.dump({"new_rows": inserts, "updates": updates,
                           "quest_script_preimage": None if not destination.exists() else
                           destination.read_text()}, handle, default=str)
            for table, row in inserts:
                names = list(row)
                cursor.execute(f"INSERT INTO `{table}` (" + ",".join(f"`{k}`" for k in names) +
                               ") VALUES (" + ",".join(["%s"] * len(names)) + ")", tuple(row.values()))
            for update in updates:
                where, values = sql_key(update["key"])
                after = update["after"]
                cursor.execute(f"UPDATE `{update['table']}` SET " +
                               ",".join(f"`{k}`=%s" for k in after) + " WHERE " + where,
                               tuple(after.values()) + values)
            if not destination.exists():
                # Exclusive creation ensures that concurrent quest edits cannot be overwritten.
                with destination.open("xb") as handle:
                    handle.write(source.read_bytes())
                script_created = True
            connection.commit()
            committed = True
            report["applied"] = True
            report["quest_script_sha256"] = hashlib.sha256(source.read_bytes()).hexdigest()
            (args.backup / "receipt.json").write_text(json.dumps(report, indent=2) + "\n")
            print(json.dumps(report, indent=2))
    except Exception:
        if not committed:
            connection.rollback()
            if script_created and destination.read_bytes() == source.read_bytes():
                destination.unlink()
        raise
    finally:
        with connection.cursor() as cursor:
            cursor.execute("SELECT RELEASE_LOCK('rof2_spells_install')")
        connection.close()


if __name__ == "__main__":
    main()
