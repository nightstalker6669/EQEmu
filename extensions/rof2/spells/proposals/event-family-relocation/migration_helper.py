"""Offline transaction prototype. No production connection factory or execution mode.

Only sqlite3 fixture connections are accepted. A reviewed MySQL adapter, engine/
schema checks, current owner receipts and maintenance authorization are still
required before this transaction design can execute against the server.
"""
import copy
import hashlib
import json
import re
import sqlite3


class GuardFailure(RuntimeError):
    pass


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':'), default=str).encode()).hexdigest()


def require(condition, message):
    if not condition:
        raise GuardFailure(message)


def ident(name):
    require(bool(re.fullmatch(r'\w+', name)), 'invalid identifier')
    return '"' + name + '"'


class FixtureMigration:
    """Concrete SQL ordering/atomicity prototype, deliberately fixture-only."""

    def __init__(self, connection, payload, archive, inputs):
        require(type(connection) is sqlite3.Connection, 'only an isolated sqlite fixture is supported')
        require(not payload['apply_allowed'] and not payload['registered_installer_stage'], 'expected held source payload')
        self.db = connection
        self.db.row_factory = sqlite3.Row
        self.payload = copy.deepcopy(payload)
        self.archive = copy.deepcopy(archive)
        self.inputs = copy.deepcopy(inputs)
        self.mapping = self.payload['mapping']
        self.old = {r['id']: r for r in self.archive['definitions']}
        self.new = {r['id']: r for r in self.payload['full_event_destination_rows']}
        self.native = self.payload['native_restoration']['after']
        self.targets = sorted(set(self.old) | set(self.new))
        require(set(self.old) == set(range(132477, 132486)), 'old scope changed')
        require(set(self.new) == set(range(900000, 900009)), 'new scope changed')
        require(len(self.mapping) == 9, 'mapping cardinality changed')
        require({r['old_id'] for r in self.mapping} == set(self.old) and {r['new_id'] for r in self.mapping} == set(self.new), 'mapping is not bijective')
        for row in self.mapping:
            before, after = self.old[row['old_id']], self.new[row['new_id']]
            require({k: v for k, v in before.items() if k != 'id'} == {k: v for k, v in after.items() if k != 'id'}, 'event definition changed')
        require(self.native['id'] == 132477 and self.native['scrolleffect'] == 30477, 'native identity changed')
        require(self.native['classes'] == 32768 and self.native['scrolllevel'] == 95, 'native eligibility changed')
        self.binding = digest({'payload': self.payload, 'archive': self.archive, 'inputs': self.inputs})

    def rows(self, table, where='', args=()):
        result = self.db.execute('SELECT * FROM ' + ident(table) + (' WHERE ' + where if where else ''), args)
        return [dict(row) for row in result.fetchall()]

    def item(self, item_id):
        rows = self.rows('items', 'id=?', (item_id,))
        return rows[0] if rows else None

    def physical_guards(self):
        marks = ','.join('?' for _ in self.targets)
        count = self.db.execute('SELECT COUNT(*) FROM inventory WHERE item_id IN (' + marks + ') OR augment_one IN (' + marks + ')', self.targets * 2).fetchone()[0]
        require(count == 0, 'current authoritative item/augment copy: stop without changing player state')
        reward_ids = [r['reward_id'] for r in self.payload['acquisition_inserts']['rewards']]
        count = self.db.execute('SELECT COUNT(*) FROM claims WHERE reward_id IN (' + ','.join('?' for _ in reward_ids) + ')', reward_ids).fetchone()[0]
        require(count == 0, 'pending or consumed native reward claim: stop')
        require(not self.rows('recovery_matches'), 'reachable recovery/codec match: stop')
        require(not self.rows('unclassified_readers'), 'unclassified materialization reader: stop')
        require(not self.rows('sessions', 'active<>0'), 'fixture actor/session not quiescent')

    def fixed_guards(self):
        require(self.item(132476) == self.inputs['template_row'], 'native sibling template drift')
        spell = self.rows('spells_new', 'id=30477')
        keys = self.inputs['native_spell_fields']
        require(len(spell) == 1 and {k: spell[0][k] for k in keys} == keys, 'native spell eligibility drift')
        require(self.rows('tasks', 'id=920119') == self.inputs['task_rows'], 'task preimage drift')
        require(self.rows('task_spell_exchange_policies', 'task_id=920119') == self.inputs['policy_rows'], 'language/choice policy drift')
        neighbors = sorted(self.rows('reward_options', 'reward_set_id=920119 AND class_mask=32768 AND option_id<>1048578'), key=lambda r: r['option_id'])
        require(neighbors == self.inputs['neighbor_class_options'], 'neighbor class choice drift')

    def expected_rows(self, state):
        if state == 'before':
            require(all(self.item(i) == row for i, row in self.old.items()), 'old definition preimage drift')
            require(all(self.item(i) is None for i in self.new), 'destination collision')
        else:
            require(all(self.item(i) == row for i, row in self.new.items()), 'relocated postimage drift')
            require(self.item(132477) == self.native, 'native postimage drift')
            require(all(self.item(i) is None for i in range(132478, 132486)), 'old slot unexpectedly occupied')
        for table, inserts in self.payload['acquisition_inserts'].items():
            for row in inserts:
                key = ' AND '.join(ident(k) + '=?' for k in self.keys(table))
                actual = self.rows(table, key, tuple(row[k] for k in self.keys(table)))
                require(actual == ([] if state == 'before' else [row]), 'acquisition pre/postimage drift: ' + table)

    @staticmethod
    def keys(table):
        return {'reward_options': ('reward_set_id', 'option_id'), 'rewards': ('reward_id',),
                'reward_option_entries': ('reward_set_id', 'option_id', 'sequence')}[table]

    def protected_digest(self):
        result = {}
        for record in self.db.execute("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name"):
            table = record[0]
            rows = self.rows(table)
            if table == 'items':
                rows = [r for r in rows if r['id'] not in self.targets]
            elif table in self.payload['acquisition_inserts']:
                keys = self.keys(table)
                selected = {tuple(r[k] for k in keys) for r in self.payload['acquisition_inserts'][table]}
                rows = [r for r in rows if tuple(r[k] for k in keys) not in selected]
            result[table] = sorted(rows, key=lambda r: json.dumps(r, sort_keys=True, default=str))
        return digest(result)

    def insert(self, table, row):
        columns = list(row)
        cursor = self.db.execute('INSERT INTO ' + ident(table) + '(' + ','.join(ident(k) for k in columns) + ') VALUES(' + ','.join('?' for _ in columns) + ')', tuple(row[k] for k in columns))
        require(cursor.rowcount == 1, 'insert count mismatch')

    def run(self, direction='forward', receipt=None, failpoint=None):
        require(direction in ('forward', 'reverse'), 'invalid direction')
        require(not self.db.in_transaction, 'caller transaction already open')
        if direction == 'reverse':
            require(receipt is not None and receipt.get('fixture_only') is True, 'missing fixture forward receipt')
            require(receipt.get('binding') == self.binding and receipt.get('direction') == 'forward', 'receipt binding mismatch')
        before_state, after_state = ('before', 'after') if direction == 'forward' else ('after', 'before')
        steps = []
        self.db.execute('BEGIN IMMEDIATE')
        try:
            self.physical_guards()
            self.fixed_guards()
            self.expected_rows(before_state)
            protected = self.protected_digest()
            if direction == 'reverse':
                require(protected == receipt['protected_digest'], 'out-of-scope state changed since forward receipt')
            initial_count = self.db.execute('SELECT COUNT(*) FROM items').fetchone()[0]
            def write(label, action):
                index = len(steps) + 1
                if failpoint:
                    failpoint(f'before_{index:02d}', self.db)
                action()
                steps.append(label)
                if failpoint:
                    failpoint(f'after_{index:02d}', self.db)
            def move(source, destination):
                cursor = self.db.execute('UPDATE items SET id=? WHERE id=?', (destination, source))
                require(cursor.rowcount == 1, 'move count mismatch')
            if direction == 'forward':
                for row in self.mapping:
                    write(f"move:{row['old_id']}:{row['new_id']}", lambda r=row: move(r['old_id'], r['new_id']))
                write('insert:native132477', lambda: self.insert('items', self.native))
                for table in ['reward_options', 'rewards', 'reward_option_entries']:
                    for row in self.payload['acquisition_inserts'][table]:
                        write('insert:' + table, lambda t=table, r=row: self.insert(t, r))
            else:
                for table in ['reward_option_entries', 'rewards', 'reward_options']:
                    for row in self.payload['acquisition_inserts'][table]:
                        def remove(t=table, r=row):
                            keys = self.keys(t)
                            cursor = self.db.execute('DELETE FROM ' + ident(t) + ' WHERE ' + ' AND '.join(ident(k) + '=?' for k in keys), tuple(r[k] for k in keys))
                            require(cursor.rowcount == 1, 'reverse acquisition count mismatch')
                        write('delete:' + table, remove)
                def remove_native():
                    cursor = self.db.execute('DELETE FROM items WHERE id=132477')
                    require(cursor.rowcount == 1, 'native delete count mismatch')
                write('delete:native132477', remove_native)
                for row in self.mapping:
                    write(f"reverse:{row['new_id']}:{row['old_id']}", lambda r=row: move(r['new_id'], r['old_id']))
            self.physical_guards()
            self.fixed_guards()
            self.expected_rows(after_state)
            delta = 1 if direction == 'forward' else -1
            require(self.db.execute('SELECT COUNT(*) FROM items').fetchone()[0] == initial_count + delta, 'item count mismatch')
            require(self.protected_digest() == protected, 'out-of-scope mutation')
            if failpoint:
                failpoint('before_commit', self.db)
            # Validate again after the final caller/test hook; no native/custody writes occur.
            self.physical_guards()
            self.fixed_guards()
            self.expected_rows(after_state)
            require(self.protected_digest() == protected, 'late out-of-scope mutation')
            self.db.commit()
            return {'fixture_only': True, 'production_acceptance': False, 'binding': self.binding,
                    'direction': direction, 'protected_digest': protected, 'steps': steps}
        except BaseException:
            self.db.rollback()
            raise


if __name__ == '__main__':
    print('SOURCE/OFFLINE ONLY: use test_migration_helper.py with isolated fixtures; no production execution entry point.')
