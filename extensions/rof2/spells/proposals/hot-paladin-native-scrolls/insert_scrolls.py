#!/usr/bin/env python3
"""Insert two absent native HoT scroll definitions. Default: offline plan only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from datetime import datetime, timezone


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, default=str).encode()).hexdigest()


def save(path, value):
    # Exclusive durable receipts: never overwrite an earlier migration attempt.
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


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--config', type=Path, help='Private eqemu_config.json; never copied into receipts')
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=3306)
    parser.add_argument('--receipt-dir', type=Path)
    args = parser.parse_args()
    data = json.loads(Path(__file__).with_name('data.json').read_text())
    rows = data['inserts']['items']
    require([r['id'] for r in rows] == [125060, 125066], 'Unexpected insert scope')
    expected = {
        125060: 'd0aa78f53fe5a1a9219339fc83c5a7d817fc7e9d800b6afedb97739d4d43f7d0',
        125066: 'c406b2abf301524684908bf4bc136c9727733c54a9596bb2078df323abc2dd5b',
    }
    require(all(digest(r) == expected[r['id']] for r in rows), 'Candidate payload changed')
    if not args.apply:
        print(json.dumps({'mode': 'offline_plan', 'database_connected': False,
                          'insert_ids': list(expected), 'reward_changes': False}))
        return
    require(args.config and args.receipt_dir, '--apply requires --config and a new --receipt-dir')
    args.receipt_dir.mkdir(mode=0o700, parents=True, exist_ok=False)
    import pymysql
    config = json.loads(args.config.read_text())['server']['database']
    require(config['db'] == 'peq_housing_test', 'Wrong database target')
    connection = pymysql.connect(host=args.host, port=args.port, user=config['username'],
        password=config['password'], database=config['db'], autocommit=False,
        cursorclass=pymysql.cursors.DictCursor)
    receipt = {'time_utc': datetime.now(timezone.utc).isoformat(), 'target': config['db'],
               'candidate_sha256': expected, 'before_items': [], 'after_items': rows,
               'scope': 'Two item definitions only; no player, reward, task, or lease writes'}
    try:
        with connection.cursor() as cursor:
            cursor.execute('SELECT DATABASE() AS target')
            require(cursor.fetchone()['target'] == 'peq_housing_test', 'Connection target changed')
            cursor.execute("SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='items'")
            require(cursor.fetchone()['ENGINE'] == 'InnoDB', 'Items must be transactional')
            cursor.execute('SHOW FULL COLUMNS FROM items')
            columns = [c['Field'] for c in cursor.fetchall()]
            require(len(columns) == 287 and all(set(r) == set(columns) for r in rows), 'Item schema changed')
            # Lock original identities and sibling source rows in one transaction.
            cursor.execute('SELECT * FROM items WHERE id IN (125059,125060,125065,125066) ORDER BY id FOR UPDATE')
            current = {r['id']: r for r in cursor.fetchall()}
            require(not (set(expected) & set(current)), 'Native target already exists; refusing replacement')
            for key, value in data['required_full_template_sha256'].items():
                require(int(key) in current and digest(current[int(key)]) == value, 'Sibling template changed')
            for spell in data['required_spells']:
                cursor.execute('SELECT id,name,`rank`,classes3 FROM spells_new WHERE id=%s LOCK IN SHARE MODE', (spell['id'],))
                require(cursor.fetchone() == spell, 'Native spell identity/Paladin level changed')
            receipt['source_templates'] = list(current.values())
            save(args.receipt_dir / 'prepared.json', receipt)
            names = ','.join('`' + c + '`' for c in columns)
            placeholders = ','.join(['%s'] * len(columns))
            for row in rows:
                cursor.execute('INSERT INTO items (' + names + ') VALUES (' + placeholders + ')',
                               tuple(row[c] for c in columns))
                require(cursor.rowcount == 1, 'Unexpected insert count')
            cursor.execute('SELECT * FROM items WHERE id IN (125060,125066) ORDER BY id')
            after = cursor.fetchall()
            require(after == rows, 'Inserted item readback differs')
            receipt['readback_sha256'] = digest(after)
        connection.commit()
        receipt['status'] = 'COMMITTED_TWO_NATIVE_SCROLL_DEFINITIONS'
        save(args.receipt_dir / 'completed.json', receipt)
        print(json.dumps({'status': receipt['status'], 'insert_ids': list(expected)}))
    except BaseException:
        connection.rollback()
        # A prepared receipt without completed.json is ambiguous after a commit/
        # receipt failure. Read exact database rows before any further action.
        raise
    finally:
        connection.close()


if __name__ == '__main__':
    main()
