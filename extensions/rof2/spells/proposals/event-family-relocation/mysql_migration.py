"""Manual, coordinated MySQL migration. Default invocation only prints the plan.

--apply is for the coordinator's agreed maintenance window after the current
canonical-custody check. No player or terminal-receipt rows are ever changed.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import uuid

from emit_mysql_plan import compile_plan, identifier, staging_payload


def normalized(value):
    return json.loads(json.dumps(value, default=str))


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def durable(path, value):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as stream:
        json.dump(value, stream, indent=2, default=str)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(directory)
    finally:
        os.close(directory)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--payload-dir', type=Path, default=Path(__file__).parent)
    parser.add_argument('--inputs', type=Path, default=Path(__file__).with_name('migration-helper-inputs.json'))
    parser.add_argument('--reference-columns', type=Path, default=Path(__file__).with_name('mysql-reference-columns.json'))
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--reverse', type=Path, help='Exact committed forward completion receipt')
    parser.add_argument('--config', type=Path)
    parser.add_argument('--backup-dir', type=Path)
    parser.add_argument('--custody-evidence', type=Path, help='Existing owner evidence used by the coordinator; not a new approval document')
    parser.add_argument('--custody-checked', action='store_true', help='Current target copies/recovery checked; unrelated sessions need not close')
    args = parser.parse_args()
    data = staging_payload(json.loads((args.payload_dir/'data.json').read_text()))
    archive = json.loads((args.payload_dir/'archive-event-definitions.json').read_text())
    inputs = json.loads(args.inputs.read_text())
    plan = compile_plan(data, archive)
    if not args.apply:
        print(json.dumps({'mode':'source plan only','mapping':[(r['old_id'],r['new_id']) for r in plan['mapping']],
                          'native_item_restorations':[132477],'acquisition_staged_disabled':True,
                          'unrelated_session_gate':False,'forward_writes':13,'reverse_writes':13,'database_connected':False},indent=2))
        return
    require(args.config and args.backup_dir and args.custody_evidence and args.custody_checked,
            '--apply requires config, recoverable backup path and the coordinated current custody/window check')
    config = json.loads(args.config.read_text())['server']['database']
    require(config['db'] == 'peq_housing_test', 'wrong target database')
    evidence_hash = hashlib.sha256(args.custody_evidence.read_bytes()).hexdigest()
    binding = plan['binding_sha256']
    previous = json.loads(args.reverse.read_text()) if args.reverse else None
    if previous:
        require(previous.get('status')=='COMMITTED' and previous.get('direction')=='forward' and previous.get('binding')==binding,
                'not the matching committed forward receipt')
        require(Path(previous['prepared_file']).is_file() and hashlib.sha256(Path(previous['prepared_file']).read_bytes()).hexdigest()==previous['prepared_sha256'], 'prepared backup missing or changed')
    args.backup_dir.mkdir(parents=True,exist_ok=True,mode=0o700)
    operation = uuid.uuid4().hex
    prepared_file = args.backup_dir/(operation+'.prepared.json')
    completed_file = args.backup_dir/(operation+'.completed.json')
    import pymysql
    db = pymysql.connect(host='127.0.0.1',port=3306,user=config['username'],password=config['password'],database=config['db'],
                         cursorclass=pymysql.cursors.DictCursor,autocommit=False)
    committed = False
    targets = list(range(132477,132486))+list(range(900000,900009))
    marks = ','.join(['%s']*len(targets))
    old = {r['id']:r for r in archive['definitions']}
    new = {r['id']:r for r in data['full_event_destination_rows']}
    native = data['native_restoration']['after']
    direction = 'reverse' if previous else 'forward'
    with db.cursor() as cursor:
        def rows(sql, parameters=()):
            cursor.execute(sql,parameters)
            return normalized(cursor.fetchall())
        def current_items():
            return {r['id']:r for r in rows('SELECT * FROM items WHERE id IN ('+marks+') ORDER BY id FOR UPDATE',targets)}
        def acquisition(expected_present):
            snapshots = {}
            for table,inserts in data['acquisition_inserts'].items():
                keys = {'reward_options':['reward_set_id','option_id'],'rewards':['reward_id'],'reward_option_entries':['reward_set_id','option_id','sequence']}[table]
                snapshots[table] = []
                for row in inserts:
                    actual = rows('SELECT * FROM '+identifier(table)+' WHERE '+' AND '.join(identifier(k)+'=%s' for k in keys)+' FOR UPDATE',[row[k] for k in keys])
                    require(actual==([row] if expected_present else []),'acquisition pre/postimage differs: '+table)
                    snapshots[table].extend(actual)
            return snapshots
        def active_references():
            for entry in json.loads(args.reference_columns.read_text())['columns']:
                hit=rows('SELECT 1 AS found FROM '+identifier(entry['table'])+' WHERE '+identifier(entry['column'])+' IN ('+marks+') LIMIT 1 FOR UPDATE',targets)
                require(not hit,'active/actionable item reference in '+entry['table']+'.'+entry['column']+'; preserve it and stop')
            count=rows('SELECT COUNT(*) n FROM rewards WHERE reward_type=0 AND reward_data_id IN ('+marks+') AND reward_id<>3810916021',targets)[0]['n']
            require(count==0,'another physical item reward references the family')
            for table,column in [('tasks','reward_id_list'),('task_activities','item_id_list'),('task_activities','item_list')]:
                for row in rows('SELECT '+identifier(column)+' AS text_value FROM '+identifier(table)+' WHERE '+identifier(column)+" IS NOT NULL AND "+identifier(column)+"<>''"):
                    require(not any(int(token) in targets for token in re.findall(r'(?<!\d)\d+(?!\d)',str(row['text_value']))),'task item/list reference; classify and preserve before migration')
            if previous:
                require(not rows('SELECT 1 FROM character_task_rewards WHERE reward_id=3810916021 LIMIT 1'),'native reward claim history exists; reverse must preserve earned progress')
                require(not rows('SELECT 1 FROM character_task_reward_selections WHERE reward_set_id=920119 AND selected_option_id=1048578 LIMIT 1'),'native selection exists; reverse must preserve it')
        try:
            cursor.execute('START TRANSACTION')
            active_references()
            before=current_items()
            expected_before=dict(new) if previous else old
            if previous:
                expected_before=dict(new);expected_before[132477]=native
            require(before==expected_before,'item preimages/destinations differ; stop')
            require(rows('SELECT * FROM items WHERE id=132476 FOR UPDATE')==[inputs['template_row']],'native sibling template changed')
            keys=list(inputs['native_spell_fields'])
            require(rows('SELECT '+','.join(identifier(k) for k in keys)+' FROM spells_new WHERE id=30477 FOR UPDATE')==[inputs['native_spell_fields']],'native spell/class/level differs')
            require(rows('SELECT * FROM tasks WHERE id=920119 FOR UPDATE')==inputs['task_rows'],'target task changed')
            require(rows('SELECT * FROM task_spell_exchange_policies WHERE task_id=920119 FOR UPDATE')==inputs['policy_rows'],'target language/choice policy changed')
            require(rows('SELECT * FROM reward_options WHERE reward_set_id=920119 AND class_mask=32768 AND option_id<>1048578 ORDER BY option_id FOR UPDATE')==inputs['neighbor_class_options'],'neighboring choices changed')
            reward_before=acquisition(bool(previous))
            initial_count=rows('SELECT COUNT(*) n FROM items')[0]['n']
            prepared={'status':'PREPARED NOT COMMITTED','direction':direction,'binding':binding,'database':config['db'],
                      'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'custody_evidence':str(args.custody_evidence.resolve()),'custody_evidence_sha256':evidence_hash,
                      'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'item_before':before,'reward_before':reward_before,
                      'archive':archive,'payload':data,'previous_completion':previous}
            durable(prepared_file,prepared)
            for step in plan[direction]:
                cursor.execute(step['sql'],step['parameters'])
                require(cursor.rowcount==1,'unexpected affected-row count; entire transaction must roll back')
            expected_after=old if previous else dict(new)
            if not previous:expected_after[132477]=native
            require(current_items()==expected_after,'full item postimage differs')
            acquisition(not previous)
            require(rows('SELECT COUNT(*) n FROM items')[0]['n']==initial_count+(-1 if previous else 1),'item count delta differs')
            active_references()
            db.commit();committed=True
            completion={'status':'COMMITTED','direction':direction,'binding':binding,'database':config['db'],'prepared_file':str(prepared_file.resolve()),
                        'prepared_sha256':hashlib.sha256(prepared_file.read_bytes()).hexdigest(),'writes':13,'item_postimages':expected_after,
                        'custody_evidence_sha256':evidence_hash,'acquisition_enabled':False,'runtime_cache_or_native_acceptance':False}
            durable(completed_file,completion)
            print(json.dumps({'status':'COMMITTED','direction':direction,'item_definitions':len(expected_after),'writes':13,'receipt':str(completed_file.resolve())},indent=2))
        except BaseException:
            if not committed:
                db.rollback()
            else:
                print('DB COMMIT SUCCEEDED; completion receipt failed. Preserve prepared backup and inspect exact postimages. DO NOT RETRY.')
            raise
        finally:
            cursor.close()
            db.close()


if __name__=='__main__':
    main()
