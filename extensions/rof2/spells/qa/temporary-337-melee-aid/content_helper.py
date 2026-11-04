#!/usr/bin/env python3
"""One QA spell insert/removal. No flags is offline; never touches character rows."""
import argparse
import hashlib
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


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


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--remove', action='store_true')
    mode.add_argument('--verify', action='store_true')
    ap.add_argument('--config', type=Path)
    ap.add_argument('--receipt-dir', type=Path)
    ap.add_argument('--expected-manifest-sha256')
    ap.add_argument('--aid-cleanup-receipt', type=Path)
    args = ap.parse_args()
    if not (args.apply or args.remove or args.verify):
        print('OFFLINE: QA337 spell39999, one content row; no connection or runtime action')
        return
    require(args.config and args.receipt_dir, 'Config and NEW receipt directory required')
    require(args.expected_manifest_sha256 == sha(ROOT / 'manifest.json'), 'Exact published manifest required')
    manifest = json.loads((ROOT / 'manifest.json').read_text())
    for name, digest in manifest['files'].items():
        require(sha(ROOT / name) == digest, 'Source drift: ' + name)
    row = json.loads((ROOT / 'data.json').read_text())['inserts']['spells_new'][0]
    require(row['id'] == 39999 and row['name'] == 'QA337 Temporary Blunt Damage', 'Wrong payload')
    if args.remove:
        require(args.aid_cleanup_receipt, 'Native337 removal receipt required before content removal')
        cleanup = json.loads(args.aid_cleanup_receipt.read_text())
        require(cleanup.get('character') == 337 and cleanup.get('spell_id') == 39999
                and cleanup.get('aid_absent') is True, 'Missing exact native aid absence')
    config = json.loads(args.config.read_text())['server']['database']
    require(config['db'] == 'peq_housing_test', 'Wrong configured database')
    args.receipt_dir.mkdir(mode=0o700, parents=True, exist_ok=False)
    import pymysql
    connection = pymysql.connect(host='127.0.0.1', port=3306, user=config['username'],
                                 password=config['password'], database=config['db'], autocommit=False,
                                 cursorclass=pymysql.cursors.DictCursor)
    receipt = {'character': 337, 'spell_id': 39999, 'manifest_sha256': args.expected_manifest_sha256,
               'operation': 'remove' if args.remove else 'verify' if args.verify else 'insert',
               'before': {}, 'player_rows_written': False, 'runtime_actions': False}
    try:
        with connection.cursor() as cur:
            cur.execute('SELECT DATABASE() AS target')
            require(cur.fetchone()['target'] == 'peq_housing_test', 'Wrong live target')
            cur.execute('SELECT * FROM spells_new WHERE id=39999 FOR UPDATE')
            before = list(cur.fetchall())
            require(before == ([row] if args.remove or args.verify else []), 'Spell preimage drift')
            receipt['before']['spells_new'] = before
            if args.verify:
                connection.rollback()
                receipt['status'] = 'PASS_EXACT_CONTENT_ROW_NOT_CACHE_OR_NATIVE_PROOF'
                save(args.receipt_dir / 'readback.json', receipt)
                return
            cur.execute("SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() "
                        "AND TABLE_NAME='spells_new'")
            require(cur.fetchone() == {'ENGINE': 'InnoDB'}, 'Nontransactional spell table')
            cur.execute('SHOW COLUMNS FROM spells_new')
            require({r['Field'] for r in cur.fetchall()} == set(row), 'Spell schema drift')
            guards = json.loads((ROOT / 'namespace-preimage.json').read_text())['checks']
            for name, guard in guards.items():
                if name == 'definition':
                    # Check the name independently while allowing only our exact ID on removal.
                    cur.execute("SELECT id FROM spells_new WHERE name='QA337 Temporary Blunt Damage' "
                                'AND id<>39999 FOR UPDATE')
                    require(not cur.fetchall(), 'QA name reused')
                    continue
                cur.execute(guard['query'] + ' FOR UPDATE')
                found = list(cur.fetchall())
                require(found == guard['rows'], 'Spell consumer changed: ' + name)
                receipt['before'][name] = found
            receipt['intended_after'] = [] if args.remove else [row]
            if args.remove:
                receipt['native_cleanup_receipt_sha256'] = sha(args.aid_cleanup_receipt)
            save(args.receipt_dir / 'prepared.json', receipt)
            if args.remove:
                cur.execute('DELETE FROM spells_new WHERE id=39999')
            else:
                columns = list(row)
                cur.execute('INSERT INTO spells_new (' + ','.join('`'+k+'`' for k in columns) +
                            ') VALUES (' + ','.join(['%s']*len(columns)) + ')', tuple(row[k] for k in columns))
            require(cur.rowcount == 1, 'Unexpected content mutation count')
            cur.execute('SELECT * FROM spells_new WHERE id=39999')
            receipt['after'] = list(cur.fetchall())
            require(receipt['after'] == receipt['intended_after'], 'Full spell postimage mismatch')
        connection.commit()
        receipt['status'] = 'COMMITTED_ONE_QA_CONTENT_ROW_CHANGE_NO_CACHE_OR_CLIENT_ACTION'
        save(args.receipt_dir / 'completed.json', receipt)
    except BaseException:
        connection.rollback()
        # Stop on prepared-only/uncertain commit; reconcile, never blindly retry.
        raise
    finally:
        connection.close()


if __name__ == '__main__':
    main()
