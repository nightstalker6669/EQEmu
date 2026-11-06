#!/usr/bin/env python3
"""Read-only schema and native identity checks for the complete Mesa extension."""
import argparse,json,pathlib,re,subprocess
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mysql-command-json',required=True);a=p.parse_args();cmd=json.loads(a.mysql_command_json)
if not isinstance(cmd,list)or not cmd or not all(isinstance(s,str)and s for s in cmd):p.error('Expected a nonempty client argument array')
if any(s.startswith(('--force','--execute','--init-command'))or s in('-e','-f')for s in cmd):p.error('Client execute/init/force options are not supported')
def query(sql):
 r=subprocess.run(cmd+['-N','-B','--raw'],input=sql,text=True,capture_output=True)
 if r.returncode:raise SystemExit('Database preflight failed: '+r.stderr)
 return[json.loads(s)for s in r.stdout.splitlines()if s]
def q(s):return"'"+str(s).replace('\\','\\\\').replace("'","''")+"'"
errors=[];columns={}
for f in(ROOT/'sql').glob('*.sql'):
 for table,body in re.findall(r'INSERT INTO\s+`?([a-z_]+)`?\s*\(([^)]+)\)',f.read_text(),re.I):
  if not table.startswith('_mesa_'):columns.setdefault(table,set()).update(c.strip().strip('`')for c in body.split(','))
for table,wanted in columns.items():
 actual={r['column']for r in query("SELECT JSON_OBJECT('column',COLUMN_NAME)FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()AND TABLE_NAME="+q(table))}
 if wanted-actual:errors.append('Required schema columns missing: '+table+'/'+','.join(sorted(wanted-actual)))
req=json.loads((ROOT/'provenance/baseline-identities.json').read_text())
found={str(r['id']):r['name']for r in query("SELECT JSON_OBJECT('id',id,'name',name)FROM npc_types WHERE id IN("+','.join(req['npc_types'])+')')}
for key,name in req['npc_types'].items():
 if int(key)in req['new_npc_ids']and key not in found:continue
 if found.get(key)!=name:errors.append('Native NPC identity differs: '+key+'/'+name)
for table,want in req['catalogue_rows'].items():
 key='achievement_id'if table in('achievement_components','achievement_category_associations')else'id';keys=','.join(str(v[key])for v in want);cols=list(want[0]);expr=','.join(q(c)+',`'+c+'`'for c in cols)
 for row in query('SELECT JSON_OBJECT('+expr+')FROM `'+table+'` WHERE `'+key+'` IN('+keys+')'):
  if row not in want:errors.append('Custom catalogue metadata requires merge: '+table+'/'+str(row[key]))
if errors:raise SystemExit('\n'.join('STOP: '+e for e in errors))
print('PASS: required schema/NPC/catalogue identities. Read-only checks only; SQL performs additional guarded content checks during import.')
