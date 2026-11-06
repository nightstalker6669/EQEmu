#!/usr/bin/env python3
"""Read-only Vergalid content/schema checks. Import SQL additionally guards row drift."""
import argparse,json,pathlib,re,subprocess
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mysql-command-json',required=True);a=p.parse_args();cmd=json.loads(a.mysql_command_json)
if not isinstance(cmd,list)or not cmd or not all(isinstance(v,str)and v for v in cmd):p.error('Expected a nonempty command array')
if any(v in('-e','-f')or v.startswith(('--force','--execute','--init-command'))for v in cmd):p.error('Force/execute/init options not supported')
def query(sql):
 r=subprocess.run(cmd+['-N','-B','--raw'],input=sql,text=True,capture_output=True)
 if r.returncode:raise SystemExit('Preflight failed: '+r.stderr)
 return[json.loads(v)for v in r.stdout.splitlines()if v]
def q(s):return"'"+s.replace('\\','\\\\').replace("'","''")+"'"
text='\n'.join(f.read_text(encoding='utf-8')for f in sorted((ROOT/'sql').glob('*.sql')))
aliases=dict(re.findall(r'CREATE TEMPORARY TABLE\s+`?([a-z_]+)`?\s+LIKE\s+`?([a-z_]+)`?',text,re.I));columns={}
for table,body in re.findall(r'INSERT INTO\s+`?([a-z_]+)`?\s*\(([^)]+)\)',text,re.I):
 table=aliases.get(table,table)
 if not table.startswith('_')and table!='shared_task_admission_policies':columns.setdefault(table,set()).update(v.strip().strip('`')for v in body.split(','))
columns.update({'character_shared_task_reward_instances':{'character_id','shared_task_id','task_id','occurrence_id'},'character_task_reward_instances':{'occurrence_id','character_id','task_id','accepted_time'}})
errors=[]
for table,wanted in columns.items():
 actual={r['column']for r in query("SELECT JSON_OBJECT('column',COLUMN_NAME)FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()AND TABLE_NAME="+q(table))}
 if wanted-actual:errors.append('Missing engine/content schema '+table+': '+','.join(sorted(wanted-actual)))
req=json.loads((ROOT/'provenance/baseline-identities.json').read_text(encoding='utf-8'));found={str(r['id']):r['name']for r in query("SELECT JSON_OBJECT('id',id,'name',name)FROM npc_types WHERE id IN("+','.join(req['npc_types'])+')')}
for key,name in req['npc_types'].items():
 if found.get(key)!=name:errors.append('Native NPC identity differs: '+key+'/'+name)
if errors:raise SystemExit('\n'.join('STOP: '+s for s in errors))
print('PASS: required content/engine schema and native NPC identities. SQL row guards remain authoritative; this does not verify the running binary.')
