#!/usr/bin/env python3
"""Four Underquarry prerequisites. Default is an offline plan; never installs quests."""
import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, default=str, separators=(',', ':'))


def save(path, value):
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), 'w') as f:
        json.dump(value, f, indent=2, default=str)
        f.write('\n')
        f.flush()
        os.fsync(f.fileno())
    fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def postimages(cur, inserts):
    keys = {'tasks': ['id'], 'task_activities': ['taskid', 'activityid'],
            'rewards': ['reward_id'], 'reward_sets': ['reward_set_id'],
            'reward_sources': ['source_type', 'source_id'],
            'reward_options': ['reward_set_id', 'option_id'],
            'reward_option_entries': ['reward_set_id', 'option_id', 'sequence']}
    result = {}
    for table, rows in inserts.items():
        result[table] = []
        for row in rows:
            pk = keys[table]
            cur.execute('SELECT * FROM `' + table + '` WHERE ' +
                        ' AND '.join('`' + k + '`=%s' for k in pk), tuple(row[k] for k in pk))
            found = cur.fetchall()
            require(len(found) == 1 and canonical(found[0]) == canonical(row),
                    'Full postimage mismatch: ' + table + ' ' + str([row[k] for k in pk]))
            result[table].append(found[0])
    return result


def preserved_references(cur, refs, lock=False):
    for name, guard in refs.items():
        cur.execute(guard['query'] + (' LOCK IN SHARE MODE' if lock else ''))
        require(canonical(cur.fetchall()) == canonical(guard['rows']),
                'Preserved reference changed: ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--verify', action='store_true')
    parser.add_argument('--config', type=Path)
    parser.add_argument('--quest-root', type=Path)
    parser.add_argument('--receipt-dir', type=Path)
    parser.add_argument('--expected-helper-sha256')
    parser.add_argument('--expected-data-sha256')
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=3306)
    args = parser.parse_args()
    data = json.loads((ROOT / 'data.json').read_text())
    inserts = data['inserts']
    require([r['id'] for r in inserts['tasks']] == [920132, 920133, 920134, 920135], 'Wrong tasks')
    require(len(inserts['task_activities']) == 42 and sum(map(len, inserts.values())) == 56, 'Wrong scope')
    require([r['reward_id'] for r in inserts['rewards']] == [3839201321, 3839201341], 'Wrong entries')
    for name, key in [('reference-preimages.json', 'reference_preimages_sha256'),
                      ('namespace-preimage.json', 'namespace_preimage_sha256')]:
        require(sha(ROOT / name) == data[key], 'Input hash mismatch: ' + name)
    if not args.apply and not args.verify:
        print(json.dumps({'mode': 'offline_plan', 'database_connected': False,
                          'inserts': {k: len(v) for k, v in inserts.items()},
                          'helper_sha256': sha(Path(__file__)), 'data_sha256': sha(ROOT / 'data.json')}))
        return
    require(args.config and args.receipt_dir, 'Live modes require config and a new receipt directory')
    require(args.expected_helper_sha256 == sha(Path(__file__)) and
            args.expected_data_sha256 == sha(ROOT / 'data.json'), 'Published helper/data revision required')
    if args.apply:
        require(data.get('reward_entry_reservation') == 'COORDINATOR_RESERVED',
                'Reward-entry reservation is still pending')
        require(args.quest_root, 'Apply requires intended runtime quest root')
        require(not (args.quest_root / 'underquarry').exists(),
                'Previously absent local quest directory is now occupied; reconcile callbacks')
        helper = args.quest_root / 'lua_modules/achievement_native_delivery_handin.lua'
        require(helper.is_file() and sha(helper) == data['required_delivery_helper_sha256'],
                'Native delivery helper changed')
        for name in data['runtime_files']:
            target = args.quest_root / name
            require(not target.exists() and not target.is_symlink(), 'Exclusive quest path occupied: ' + name)
    cfg = json.loads(args.config.read_text())['server']['database']
    require(cfg['db'] == 'peq_housing_test', 'Wrong configured database')
    args.receipt_dir.mkdir(mode=0o700, parents=True, exist_ok=False)
    import pymysql
    connection = pymysql.connect(host=args.host, port=args.port, user=cfg['username'],
                                 password=cfg['password'], database=cfg['db'], autocommit=False,
                                 cursorclass=pymysql.cursors.DictCursor)
    receipt = {'utc': datetime.now(timezone.utc).isoformat(), 'database': cfg['db'],
               'helper_sha256': sha(Path(__file__)), 'data_sha256': sha(ROOT / 'data.json'),
               'before': {}, 'intended_after': inserts, 'quests_installed': False}
    try:
        with connection.cursor() as cur:
            cur.execute('SELECT DATABASE() AS target')
            require(cur.fetchone()['target'] == 'peq_housing_test', 'Wrong live target')
            if args.verify:
                receipt['after'] = postimages(cur, inserts)
                preserved_references(cur, json.loads((ROOT / 'reference-preimages.json').read_text()))
                connection.rollback()
                receipt['status'] = 'PASS_INDEPENDENT_FULL_CONTENT_READBACK_NOT_NATIVE_ACCEPTANCE'
                save(args.receipt_dir / 'readback.json', receipt)
                print(receipt['status'])
                return
            for table in inserts:
                cur.execute('SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() '
                            'AND TABLE_NAME=%s', (table,))
                require(cur.fetchone() == {'ENGINE': 'InnoDB'}, 'Nontransactional table: ' + table)
                cur.execute('SHOW COLUMNS FROM `' + table + '`')
                require({r['Field'] for r in cur.fetchall()} == set(inserts[table][0]),
                        'Unexpected full-row schema: ' + table)
            namespaces = json.loads((ROOT / 'namespace-preimage.json').read_text())['checks']
            for name, guard in namespaces.items():
                cur.execute(guard['query'] + ' FOR UPDATE')
                rows = cur.fetchall()
                require(not rows and not guard['rows'], 'Namespace/consumer conflict: ' + name)
                receipt['before'][name] = rows
            # Guard dangling legitimate progress without copying player rows.
            for table in ['character_tasks', 'character_activities', 'completed_tasks']:
                cur.execute('SELECT taskid FROM `' + table + '` WHERE taskid IN '
                            '(920132,920133,920134,920135) LIMIT 1 FOR UPDATE')
                require(not cur.fetchall(), 'Existing task progress/history references: ' + table)
            refs = json.loads((ROOT / 'reference-preimages.json').read_text())
            receipt['preserved_references'] = refs
            preserved_references(cur, refs, lock=True)
            save(args.receipt_dir / 'prepared.json', receipt)
            for table, rows in inserts.items():
                for row in rows:
                    columns = list(row)
                    cur.execute('INSERT INTO `' + table + '` (' +
                                ','.join('`' + k + '`' for k in columns) + ') VALUES (' +
                                ','.join(['%s'] * len(columns)) + ')', tuple(row[k] for k in columns))
                    require(cur.rowcount == 1, 'Unexpected insert count')
            receipt['after'] = postimages(cur, inserts)
            preserved_references(cur, refs)
        connection.commit()
        receipt['status'] = 'COMMITTED_FOUR_PREREQUISITES_56_ROWS_QUESTS_NOT_INSTALLED'
        save(args.receipt_dir / 'completed.json', receipt)
        print(receipt['status'])
    except BaseException:
        connection.rollback()
        # A commit/receipt exception can be ambiguous. Stop; reconcile actual
        # rows against prepared.json before any retry or subsequent component.
        raise
    finally:
        connection.close()


if __name__ == '__main__':
    main()
