"""Portable, guarded content installation. No server/process/version management."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
from datetime import datetime, timezone


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--quests-dir', required=True, type=Path)
    ap.add_argument('--backup-dir', required=True, type=Path)
    ap.add_argument('--db-command-file', required=True, type=Path,
                    help='JSON array: MariaDB client argv, including selected database; SQL uses stdin')
    ap.add_argument('--engine-reviewed', action='store_true')
    ap.add_argument('--server-stopped', action='store_true')
    ap.add_argument('--apply', action='store_true', help='Without this flag, inspect files only; no DB access')
    args = ap.parse_args()
    base = Path(__file__).resolve().parent
    manifest = json.loads((base / 'PAYLOAD-MANIFEST.json').read_text())
    for rel, expected in manifest['sha256'].items():
        if digest(base / rel) != expected:
            raise RuntimeError('Package checksum mismatch: ' + rel)
    quests = args.quests_dir.resolve(strict=True)
    targets = []
    for rel in sorted(manifest['sha256']):
        if not rel.startswith('quests/'):
            continue
        target = quests / Path(rel).relative_to('quests')
        if target.is_symlink() or quests not in target.resolve().parents:
            raise RuntimeError('Unsafe quest target: ' + str(target))
        old = digest(target) if target.exists() else None
        allowed = [None, manifest['sha256'][rel]] + manifest['reviewed_previous_sha256'].get(rel, [])
        if old not in allowed:
            raise RuntimeError('Unreviewed existing callback/module: ' + str(target))
        targets.append((rel, target, old))
    # Numeric Lua callbacks can coexist with named callbacks: refuse ambiguity.
    for name in ('Tabazin_Gearflinger.lua', 'Tabazin_Gearflinger.pl', '#Tabazin_Gearflinger.lua',
                 '#Tabazin_Gearflinger.pl', '436006.pl'):
        if (quests / 'mechanotus' / name).exists():
            raise RuntimeError('Existing Tabazin callback requires explicit composition: ' + name)
    for rel in ('global/global_npc.lua', 'global/global_npc.pl',
                'mechanotus/global_npc.lua', 'mechanotus/global_npc.pl'):
        callback = quests / rel
        if callback.exists() and re.search(r'\b(?:event_trade|EVENT_ITEM)\b', callback.read_text(errors='replace')):
            raise RuntimeError('Global trade callback requires custody review: ' + rel)
    command = json.loads(args.db_command_file.read_text())
    if not isinstance(command, list) or not command or not all(isinstance(x, str) and x for x in command):
        raise RuntimeError('DB command must be a nonempty JSON argv array')
    if not args.apply:
        print('File preflight passed. Database dependencies and collisions have not been checked.')
        return
    if not args.engine_reviewed or not args.server_stopped:
        raise RuntimeError('Review required engine dependency and stop the affected server before applying')
    backup = args.backup_dir.resolve()
    backup.mkdir(parents=True, exist_ok=False)
    receipt = {'utc': datetime.now(timezone.utc).isoformat(), 'status': 'ATTEMPT_STARTED',
               'targets': [{'path': str(t), 'before_sha256': h} for _, t, h in targets]}

    def save():
        (backup / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')

    save()
    staged = []
    try:
        for rel, target, old in targets:
            if old is not None:
                dest = backup / Path(rel)
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(target, dest)
            target.parent.mkdir(parents=True, exist_ok=True)
            temp = target.with_name(target.name + '.gearing9340.tmp')
            with temp.open('xb') as f:
                f.write((base / rel).read_bytes())
                f.flush()
                os.fsync(f.fileno())
            staged.append((rel, target, temp, old))
        sql = (base / 'sql/2026_11_03_gearing_up_achievement.sql').read_bytes()
        result = subprocess.run(command, input=sql, capture_output=True, shell=False)
        (backup / 'database.stdout').write_bytes(result.stdout)
        (backup / 'database.stderr').write_bytes(result.stderr)
        if result.returncode:
            raise RuntimeError('Database operation failed; inspect private backup logs before any retry')
        receipt['status'] = 'SQL_COMMITTED_FILES_PENDING'
        save()
        for rel, target, temp, old in staged:
            if (digest(target) if target.exists() else None) != old:
                raise RuntimeError('Target changed after preflight: ' + str(target))
            os.replace(temp, target)
            if digest(target) != manifest['sha256'][rel]:
                raise RuntimeError('Installed checksum mismatch: ' + str(target))
        receipt['status'] = 'CONTENT_INSTALLED_CACHE_AND_NATIVE_PENDING'
        save()
        print('Content installed. Refresh task/achievement caches and quest VMs before native verification.')
    except Exception as exc:
        receipt['failure'] = str(exc)
        receipt['automatic_retry_or_rollback'] = False
        save()
        raise


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
