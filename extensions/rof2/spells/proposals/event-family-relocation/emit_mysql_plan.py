"""Emit reviewable MySQL statements/parameters. No database imports or execution."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re


def identifier(value):
    if not re.fullmatch(r'\w+', value):
        raise ValueError('invalid SQL identifier')
    return '`' + value + '`'


def insert(table, row):
    columns = list(row)
    return {'sql': 'INSERT INTO ' + identifier(table) + '(' + ','.join(identifier(k) for k in columns) + ') VALUES(' + ','.join('%s' for _ in columns) + ')',
            'parameters': [row[k] for k in columns], 'expected_affected_rows': 1}


def staging_payload(data):
    data = copy.deepcopy(data)
    for table in ('reward_options', 'rewards'):
        for row in data['acquisition_inserts'][table]:
            row['enabled'] = 0
    return data


def compile_plan(data, archive):
    data = staging_payload(data)
    if data['apply_allowed'] or data['registered_installer_stage']:
        raise ValueError('this emitter accepts only the held source proposal')
    mapping = data['mapping']
    old = sorted(r['old_id'] for r in mapping)
    new = sorted(r['new_id'] for r in mapping)
    if old != list(range(132477,132486)) or new != list(range(900000,900009)):
        raise ValueError('scope changed')
    forward = [{'sql': 'UPDATE `items` SET `id`=%s WHERE `id`=%s', 'parameters': [r['new_id'],r['old_id']], 'expected_affected_rows':1} for r in mapping]
    forward.append(insert('items',data['native_restoration']['after']))
    for table in ['reward_options','rewards','reward_option_entries']:
        forward.extend(insert(table,row) for row in data['acquisition_inserts'][table])
    reverse = []
    keys = {'reward_option_entries':['reward_set_id','option_id','sequence'], 'rewards':['reward_id'], 'reward_options':['reward_set_id','option_id']}
    for table in ['reward_option_entries','rewards','reward_options']:
        for row in data['acquisition_inserts'][table]:
            reverse.append({'sql':'DELETE FROM '+identifier(table)+' WHERE '+' AND '.join(identifier(k)+'=%s' for k in keys[table]),
                            'parameters':[row[k] for k in keys[table]],'expected_affected_rows':1})
    reverse.append({'sql':'DELETE FROM `items` WHERE `id`=%s','parameters':[132477],'expected_affected_rows':1})
    reverse.extend({'sql':'UPDATE `items` SET `id`=%s WHERE `id`=%s','parameters':[r['old_id'],r['new_id']],'expected_affected_rows':1} for r in mapping)
    assert len(forward)==len(reverse)==13
    return {'format':1,'status':'SOURCE SQL PLAN ONLY; NOT EXECUTABLE AUTHORIZATION','apply_allowed':False,
            'acquisition_enabled':False,'activation':'Enable the two new option/reward rows only after coordinated item-cache refresh.',
            'mapping':mapping,'binding_sha256':hashlib.sha256(json.dumps({'data':data,'archive':archive},sort_keys=True,separators=(',',':')).encode()).hexdigest(),
            'transaction':{'begin':'START TRANSACTION','failure':'ROLLBACK','success':'COMMIT only after all reviewed preflight/postimage checks'},
            'locked_definition_preimage_query':{'sql':'SELECT * FROM `items` WHERE `id` IN ('+','.join('%s' for _ in old+new)+') ORDER BY `id` FOR UPDATE','parameters':old+new},
            'forward':forward,'reverse':reverse,
            'runner_requirements':['Current owner52canonical semantics and reused historical fingerprints must cover the exact targets; unrelated active sessions are not a gate',
                'Fresh exact schema/engines/FK,114-column reference/task/physical-reward and full old/new/template/spell/task/policy/neighbor checks',
                'Durable reviewed preimages and namespace ledger before first mutation; exact reviewed helper/contract/payload hashes',
                'Every affected-row count must be1; every error stops with transaction rollback; never retry partial operations',
                'Full postimages, item-count delta and out-of-scope immutable-receipt preservation before commit',
                'Reverse only after receipt ownership/current postimages and no new copies, consumed/pending claims or affected recovery state',
                'SQLite proof does not establish MySQL locking; actual apply uses target/copy locks, recoverable backup, one transaction and exact readback']}


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--payload-dir',type=Path,default=Path(__file__).parent)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    data=json.loads((args.payload_dir/'data.json').read_text())
    archive=json.loads((args.payload_dir/'archive-event-definitions.json').read_text())
    args.output.write_text(json.dumps(compile_plan(data,archive),indent=2)+'\n')
    print('Wrote source-only13forward/13reverse MySQL parameter plans; no database connection made.')
