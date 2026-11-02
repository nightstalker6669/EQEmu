#!/usr/bin/env python3
"""Read-only schema/native identity checks; SQL retains authoritative drift guards."""
import argparse,json,pathlib,re,subprocess
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mysql-command-json',required=True);a=p.parse_args();cmd=json.loads(a.mysql_command_json)
if not isinstance(cmd,list)or not cmd or not all(isinstance(s,str)and s for s in cmd):p.error('Expected nonempty command array')
if any(s in('-e','-f')or s.startswith(('--force','--execute','--init-command'))for s in cmd):p.error('Execute/init/force options not supported')
def query(sql):
 r=subprocess.run(cmd+['-N','-B','--raw'],input=sql,text=True,capture_output=True)
 if r.returncode:raise SystemExit('Preflight failed: '+r.stderr)
 return[json.loads(v)for v in r.stdout.splitlines()if v]
def q(s):return"'"+s.replace('\\','\\\\').replace("'","''")+"'"
columns={};errors=[]
for f in(ROOT/'sql').glob('*.sql'):
 for table,body in re.findall(r'INSERT INTO\s+`?([a-z_]+)`?\s*\(([^)]+)\)',f.read_text(),re.I):
  if not table.startswith('_roost_'):columns.setdefault(table,set()).update(c.strip().strip('`')for c in body.split(','))
for table,wanted in columns.items():
 actual={r['column']for r in query("SELECT JSON_OBJECT('column',COLUMN_NAME)FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()AND TABLE_NAME="+q(table))}
 if wanted-actual:errors.append('Missing schema: '+table+'/'+','.join(sorted(wanted-actual)))
req=json.loads((ROOT/'provenance/baseline-identities.json').read_text());found={str(r['id']):r['name']for r in query("SELECT JSON_OBJECT('id',id,'name',name)FROM npc_types WHERE id IN("+','.join(req['npc_types'])+')')}
for key,name in req['npc_types'].items():
 if found.get(key)!=name:errors.append('Native NPC identity differs: '+key+'/'+name)
if errors:raise SystemExit('\n'.join('STOP: '+s for s in errors))
print('PASS read-only required schema/NPC identity checks. Import SQL separately guards tasks/items/spawns/rewards/catalogue drift.')
