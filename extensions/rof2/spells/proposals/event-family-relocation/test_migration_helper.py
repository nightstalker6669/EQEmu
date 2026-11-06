"""Real SQLite transactions on generated fixtures; never connects to EQEmu DBs."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sqlite3
import tempfile

from migration_helper import FixtureMigration, GuardFailure, ident


def load(path):
    return json.loads(path.read_text())


def fixture(path, data, archive, inputs):
    db = sqlite3.connect(path)
    db.row_factory = sqlite3.Row
    def table(name, examples, keys=()):
        columns = list(examples[0])
        ddl = ','.join(ident(k) for k in columns)
        if keys:
            ddl += ',PRIMARY KEY(' + ','.join(ident(k) for k in keys) + ')'
        db.execute('CREATE TABLE ' + ident(name) + '(' + ddl + ')')
    def insert(name, row):
        columns = list(row)
        db.execute('INSERT INTO ' + ident(name) + '(' + ','.join(ident(k) for k in columns) + ') VALUES(' + ','.join('?' for _ in columns) + ')', tuple(row[k] for k in columns))
    table('items', archive['definitions'], ('id',))
    for row in archive['definitions'] + [inputs['template_row']]:
        insert('items', row)
    unrelated = copy.deepcopy(archive['definitions'][0])
    unrelated['id'], unrelated['Name'] = 42, 'Fixture unrelated item'
    insert('items', unrelated)
    for name in ['reward_options', 'rewards', 'reward_option_entries']:
        table(name, data['acquisition_inserts'][name], FixtureMigration.keys(name))
    for row in inputs['neighbor_class_options']:
        insert('reward_options', row)
    insert('rewards', {'reward_id': 1, 'reward_type': 1, 'reward_data_id': 1, 'amount': 3, 'description': 'Fixture unrelated reward', 'enabled': 1})
    insert('reward_option_entries', {'reward_set_id': 920119, 'option_id': 1048577, 'sequence': 1, 'reward_id': 1})
    table('spells_new', [inputs['native_spell_fields']], ('id',))
    insert('spells_new', inputs['native_spell_fields'])
    for name, rows, keys in [('tasks', inputs['task_rows'], ('id',)), ('task_spell_exchange_policies', inputs['policy_rows'], ('task_id',))]:
        table(name, rows, keys)
        for row in rows:
            insert(name, row)
    db.execute('CREATE TABLE inventory(guid TEXT PRIMARY KEY,item_id INTEGER,augment_one INTEGER,charges INTEGER,slot INTEGER,owner INTEGER)')
    db.execute("INSERT INTO inventory VALUES('fixture-preserved-guid',42,0,7,22,999001)")
    db.execute('CREATE TABLE claims(reward_id INTEGER,state TEXT)')
    db.execute('CREATE TABLE recovery_matches(native_item_id INTEGER,kind TEXT)')
    db.execute('CREATE TABLE unclassified_readers(kind TEXT)')
    db.execute('CREATE TABLE sessions(character_id INTEGER,active INTEGER)')
    db.execute('CREATE TABLE terminal_receipts(id INTEGER PRIMARY KEY,payload BLOB,digest TEXT)')
    raw = b'fixture immutable historical event-id=132477\x00original digest witness'
    db.execute('INSERT INTO terminal_receipts VALUES(1,?,?)', (raw, hashlib.sha256(raw).hexdigest()))
    db.commit()
    return db


def state(db):
    result = {}
    for row in db.execute("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name"):
        name = row[0]
        rows = [dict(x) for x in db.execute('SELECT * FROM ' + ident(name))]
        result[name] = sorted(rows, key=lambda x: json.dumps(x, sort_keys=True, default=str))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--payload-dir', type=Path, default=Path(__file__).parent)
    parser.add_argument('--inputs', type=Path, default=Path(__file__).with_name('migration-helper-inputs.json'))
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    data = load(args.payload_dir / 'data.json')
    archive = load(args.payload_dir / 'archive-event-definitions.json')
    inputs = load(args.inputs)
    results = []
    with tempfile.TemporaryDirectory(prefix='rof-event-migration-offline-') as directory:
        serial = 0
        def fresh():
            nonlocal serial
            serial += 1
            path = Path(directory) / f'fixture-{serial}.sqlite'
            db = fixture(path, data, archive, inputs)
            return path, db, FixtureMigration(db, data, archive, inputs)
        def check(name, action):
            action()
            results.append({'name': name, 'passed': True})
        def roundtrip():
            path, db, helper = fresh()
            before = state(db)
            receipt = helper.run()
            assert len(receipt['steps']) == 13
            committed = state(db)
            assert committed['inventory'] == before['inventory']
            assert committed['terminal_receipts'] == before['terminal_receipts']
            db.close()
            db = sqlite3.connect(path)
            db.row_factory = sqlite3.Row
            assert state(db) == committed
            helper = FixtureMigration(db, data, archive, inputs)
            reverse = helper.run('reverse', receipt)
            assert len(reverse['steps']) == 13
            assert state(db) == before
            db.close()
        check('forward_commit_reopen_exact_guarded_reverse', roundtrip)
        points = [f'{side}_{n:02d}' for n in range(1, 14) for side in ('before', 'after')] + ['before_commit']
        for direction in ('forward', 'reverse'):
            for point in points:
                def fail_test(direction=direction, point=point):
                    _, db, helper = fresh()
                    receipt = helper.run() if direction == 'reverse' else None
                    before = state(db)
                    def fail(name, _db):
                        if name == point:
                            raise GuardFailure('injected ' + point)
                    try:
                        helper.run(direction, receipt, fail)
                        raise AssertionError('failpoint did not stop')
                    except GuardFailure as error:
                        assert str(error) == 'injected ' + point
                    assert not db.in_transaction and state(db) == before
                    db.close()
                check(direction + '_atomic_failure_' + point, fail_test)
        forward_cases = {
            'destination_collision': "INSERT INTO items SELECT 900000," + ','.join(ident(k) for k in archive['definitions'][0] if k != 'id') + ' FROM items WHERE id=42',
            'old_definition_drift': "UPDATE items SET Name='drift' WHERE id=132477",
            'template_drift': "UPDATE items SET Name='drift' WHERE id=132476",
            'native_level_drift': 'UPDATE spells_new SET classes16=94 WHERE id=30477',
            'task_drift': 'UPDATE tasks SET enabled=0 WHERE id=920119',
            'policy_drift': 'UPDATE task_spell_exchange_policies SET language_skill_required=74 WHERE task_id=920119',
            'neighbor_choice_drift': 'UPDATE reward_options SET enabled=0 WHERE option_id=1048577',
            'old_player_copy': 'UPDATE inventory SET item_id=132477',
            'new_player_copy': 'UPDATE inventory SET item_id=900008',
            'augment_copy': 'UPDATE inventory SET augment_one=132479',
            'reachable_recovery': "INSERT INTO recovery_matches VALUES(132480,'fixture nested/HEV1 match')",
            'unclassified_reader': "INSERT INTO unclassified_readers VALUES('fixture pending producer')",
            'active_session': 'INSERT INTO sessions VALUES(999001,1)',
            'existing_reward_target': "INSERT INTO rewards VALUES(3810916021,0,42,1,'drift',1)",
        }
        for name, sql in forward_cases.items():
            def guard_test(sql=sql):
                _, db, helper = fresh()
                db.execute(sql)
                db.commit()
                before = state(db)
                try:
                    helper.run()
                    raise AssertionError('guard did not stop')
                except GuardFailure:
                    pass
                assert state(db) == before
                db.close()
            check('forward_stop_' + name, guard_test)
        reverse_cases = {
            'native_tome_copy': 'UPDATE inventory SET item_id=132477',
            'relocated_event_copy': 'UPDATE inventory SET item_id=900000',
            'consumed_reward_claim': "INSERT INTO claims VALUES(3810916021,'consumed')",
            'pending_reward_claim': "INSERT INTO claims VALUES(3810916021,'pending')",
            'relocated_postimage_drift': "UPDATE items SET Name='drift' WHERE id=900000",
            'native_postimage_drift': "UPDATE items SET Name='drift' WHERE id=132477",
            'unrelated_player_progress': 'UPDATE inventory SET charges=6',
        }
        for name, sql in reverse_cases.items():
            def reverse_guard_test(sql=sql):
                _, db, helper = fresh()
                receipt = helper.run()
                db.execute(sql)
                db.commit()
                before = state(db)
                try:
                    helper.run('reverse', receipt)
                    raise AssertionError('reverse guard did not stop')
                except GuardFailure:
                    pass
                assert state(db) == before
                db.close()
            check('reverse_preserves_current_state_' + name, reverse_guard_test)
        for name, sql in {
            'late_native_drift': "UPDATE items SET Name='late drift' WHERE id=132477",
            'late_player_copy': 'UPDATE inventory SET item_id=132477',
            'late_terminal_rewrite': "UPDATE terminal_receipts SET payload=x'00'",
        }.items():
            def late_guard_test(sql=sql):
                _, db, helper = fresh()
                before = state(db)
                def inject(point, connection):
                    if point == 'before_commit':
                        connection.execute(sql)
                try:
                    helper.run(failpoint=inject)
                    raise AssertionError('late guard did not stop')
                except GuardFailure:
                    pass
                assert state(db) == before
                db.close()
            check(name + '_rolls_back', late_guard_test)
        def forged_receipt():
            _, db, helper = fresh()
            receipt = helper.run()
            receipt['binding'] = 'not this proposal'
            before = state(db)
            try:
                helper.run('reverse', receipt)
                raise AssertionError('wrong receipt accepted')
            except GuardFailure:
                pass
            assert state(db) == before
            db.close()
        check('wrong_receipt_binding_stops', forged_receipt)
        def production_rejected():
            class UnapprovedConnection:
                touched = False
                def execute(self, *_):
                    self.touched = True
            connection = UnapprovedConnection()
            try:
                FixtureMigration(connection, data, archive, inputs)
                raise AssertionError('non-fixture connection accepted')
            except GuardFailure:
                pass
            assert not connection.touched
        check('non_fixture_connection_rejected_before_sql', production_rejected)
    report = {'status': 'PASS OFFLINE SQLITE FIXTURES ONLY', 'test_count': len(results), 'tests': results,
              'helper_sha256': hashlib.sha256(Path(__file__).with_name('migration_helper.py').read_bytes()).hexdigest(),
              'payload_sha256': hashlib.sha256((args.payload_dir/'data.json').read_bytes()).hexdigest(),
              'inputs_sha256': hashlib.sha256(args.inputs.read_bytes()).hexdigest(),
              'forward_dml_steps': 13, 'reverse_dml_steps': 13, 'failpoints_per_direction': 27,
              'scope_limits': ['SQLite transactions, not MySQL/InnoDB lock/engine acceptance', 'Synthetic semantic recovery guards, not Housing decoder evidence', 'No production adapter, maintenance authorization or current owner clearance', 'No server DB/client/build/cache action']}
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'tests'}, indent=2))


if __name__ == '__main__':
    main()
