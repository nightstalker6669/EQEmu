#!/usr/bin/env python3
"""Inspect or install the entire Vergalid batch. Never controls server services."""
import argparse,hashlib,json,math,os,pathlib,re,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parent

def read_json(path):return json.loads(path.read_text(encoding='utf-8'))
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def safe(root,rel):
 rel=pathlib.PurePosixPath(rel)
 if rel.is_absolute()or'..'in rel.parts:raise ValueError('Unsafe package path: '+str(rel))
 p=root
 for part in rel.parts:
  p=p/part
  if p.is_symlink():raise ValueError('Symlink in managed path: '+str(p))
 return p

def command(value):
 a=json.loads(value)
 if not isinstance(a,list)or not a or not all(isinstance(v,str)and v for v in a):raise ValueError('Expected a nonempty command array')
 if any(v.startswith(('-e','-f','--force','--execute','--init-command','--all-databases')) for v in a[1:]):raise ValueError('Force/execute/init/all-database options are not supported')
 return a

def package():
 rows=read_json(ROOT/'payload-manifest.json');seen=set()
 for row in rows:
  rel=row['path']
  if rel in seen:raise ValueError('Duplicate payload path: '+rel)
  seen.add(rel);f=safe(ROOT,rel)
  if not f.is_file()or digest(f)!=row['sha256']:raise ValueError('Payload integrity mismatch: '+rel)
 config=read_json(ROOT/'package-config.json')
 for rel in ['install.py','preflight.py','apply_quests.py','quest-manifest.json','package-config.json',config['readiness_file'],config['content_data'],config['world_policy'],config['orb_guardian_config']]:
  if rel not in seen:raise ValueError('Required file is not in the payload manifest: '+rel)
 # Every possible SQL input is inventoried, including any staged extra file.
 for f in (ROOT/'sql').glob('*.sql'):
  if str(f.relative_to(ROOT)) not in seen:raise ValueError('Uninventoried SQL file: '+f.name)
 missing=[rel for rel in config['sql_files'] if rel not in seen or not safe(ROOT,rel).is_file()]
 readiness=read_json(safe(ROOT,config['readiness_file']))
 if readiness['expected_counts']!=config['expected_counts']:raise ValueError('Readiness count contract differs from package configuration')
 return config,readiness,missing

def lua_value(v):
 if v is None:return 'nil'
 if isinstance(v,bool):return 'true'if v else'false'
 if isinstance(v,(int,float)):return str(v)
 if isinstance(v,str):return json.dumps(v,ensure_ascii=False)
 if isinstance(v,list):return '{'+','.join(map(lua_value,v))+'}'
 return '{'+','.join('['+lua_value(k)+']='+lua_value(x)for k,x in v.items())+'}'

def guardian_settings(config):
 g=read_json(safe(ROOT,config['orb_guardian_config']))
 if g.get('reviewed')is not True:raise ValueError('Orb guardian encounter is unreviewed')
 if type(g.get('replacement_seconds'))is not int or g['replacement_seconds']<1:raise ValueError('Orb guardian replacement delay must be explicitly configured')
 if type(g.get('idle_reset_seconds'))is not int or g['idle_reset_seconds']<1:raise ValueError('Orb idle reset must be explicitly configured')
 slots=g.get('slots');ids=set();roles=set()
 if not isinstance(slots,list)or len(slots)!=2:raise ValueError('Orb requires the original two guardian roles')
 for slot in slots:
  role=slot.get('role');npc=slot.get('npc');point=slot.get('point')
  if role not in('cleric','warrior')or role in roles:raise ValueError('Orb guardian roles must be distinct cleric/warrior')
  if type(npc)is not int or npc<1 or npc==404903 or npc in ids:raise ValueError('Orb requires distinct dedicated guardian NPC types')
  if not isinstance(point,dict)or any(type(point.get(k))not in(int,float)or not math.isfinite(point[k])or abs(point[k])>=100000 for k in('x','y','z')):raise ValueError('Missing or invalid guardian position')
  if 'heading'in point and(type(point['heading'])not in(int,float)or not math.isfinite(point['heading'])):raise ValueError('Invalid guardian heading')
  ids.add(npc);roles.add(role)
 text=safe(ROOT,config['content_data']).read_text(encoding='utf-8')
 if text.count('["guardians"]=')!=1 or '["guardians"]='+lua_value(g)not in text:raise ValueError('Orb guardian settings differ from the actual Lua runtime payload')
 return g

def full_sql(config):
 policy=read_json(safe(ROOT,config['world_policy']))
 if policy.get('npc_profiles_reviewed')is not True or policy.get('quest_drop_minimums_reviewed')is not True:raise ValueError('Actor profiles and minimum quest-drop policy have not been reviewed')
 chances=policy.get('named_chances',{})
 if any(type(chances.get(k))is not int or not 1<=chances[k]<=99 for k in ['skullrend','vuryn']):raise ValueError('Explicit integral named chances1..99 are required; no defaults exist')
 if policy.get('actor_sql_sha256')!=digest(safe(ROOT,'sql/040-world-actors.review-required.sql')):raise ValueError('Actor SQL changed after policy review')
 prefix='SET @vergalid_actor_profiles_reviewed=1;SET @vergalid_skullrend_chance='+str(chances['skullrend'])+';SET @vergalid_vuryn_chance='+str(chances['vuryn'])+';\n'
 parts=[]
 for rel in config['sql_files']:
  s=safe(ROOT,rel).read_text(encoding='utf-8')
  # Only temporary-table DDL is permitted inside the whole-content transaction.
  if re.search(r'(?im)^\s*(?:ALTER\s+TABLE|TRUNCATE\b|CREATE\s+(?!TEMPORARY\b)TABLE|DROP\s+(?!TEMPORARY\b)TABLE)',s):raise ValueError('Nontransactional DDL in '+rel)
  s=re.sub(r'(?m)^START TRANSACTION;\s*$','',s)
  s=re.sub(r'(?m)^COMMIT;\s*$','',s)
  s=re.sub(r'(?m)^SET @vergalid_[A-Za-z0-9_]*ok\s*=\s*NULL;\s*$','',s)
  parts.append(s)
 return prefix+'SET @vergalid_reviewed=1;SET @vergalid_world_reviewed=1;SET @vergalid_activation_reviewed=1;\nSTART TRANSACTION;\n'+'\n'.join(parts)+'\nCOMMIT;\n'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--inspect-package',action='store_true',help='Inspect completeness and integrity without a database connection')
 p.add_argument('--mysql-command-json');p.add_argument('--dump-command-json')
 p.add_argument('--quests',type=pathlib.Path);p.add_argument('--additional-quests',action='append',default=[],type=pathlib.Path)
 p.add_argument('--backup',type=pathlib.Path);p.add_argument('--apply',action='store_true')
 p.add_argument('--services-stopped',action='store_true');p.add_argument('--matching-engine-installed',action='store_true')
 a=p.parse_args();config,ready,missing=package()
 if a.inspect_package:
  if a.apply:p.error('--inspect-package cannot be combined with --apply')
  print(json.dumps({'feature':config['feature'],'expected_counts':config['expected_counts'],'installable':ready['installable']and not missing,'missing_payloads':missing,'unresolved':ready.get('unresolved',[]),'native_gameplay_verified':ready.get('native_gameplay_verified',False),'database_or_quest_writes':False},indent=2));return
 if not a.mysql_command_json or not a.quests:p.error('Database/quest inspection requires --mysql-command-json and --quests')
 if a.apply and(not ready['installable']or not ready['runtime_enabled']or missing or ready.get('unresolved')):raise SystemExit('STOP: whole Vergalid batch is incomplete; inspect with --inspect-package. No backup, database or quest mutation was attempted.')
 if a.apply:
  guardian_settings(config)
  module=safe(ROOT,config['content_data'])
  text=module.read_text(encoding='utf-8')
  if ready.get('runtime_data_sha256')!=digest(module)or text.count('[\"enabled\"]=true')!=config['expected_enabled_entities']or '[\"enabled\"]=false'in text:raise SystemExit('STOP: enabled runtime data does not match the complete reviewed package')
 client=command(a.mysql_command_json)
 questcmd=[sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(a.quests)]
 for q in a.additional_quests:questcmd+=['--additional-quests',str(q)]
 subprocess.run([sys.executable,str(ROOT/'preflight.py'),'--mysql-command-json',a.mysql_command_json],check=True)
 subprocess.run(questcmd,check=True)
 if not a.apply:
  print('Read-only database/schema and quest-conflict inspection passed. Whole-package installability: '+str(ready['installable']and not missing)+'. This does not prove a running engine or gameplay.');return
 if not(a.backup and a.dump_command_json and a.services_stopped and a.matching_engine_installed):p.error('--apply requires --backup, --dump-command-json, --services-stopped and --matching-engine-installed')
 # Assemble and validate every component before creating a backup or writing files.
 sql=full_sql(config);dump=command(a.dump_command_json)
 backup=a.backup.absolute()
 if backup.is_symlink():raise ValueError('Backup cannot be a symlink')
 backup=backup.resolve()
 for root in[a.quests]+a.additional_quests:
  root=root.resolve()
  if backup==root or root in backup.parents:raise ValueError('Backup must be outside every quest root')
 backup.mkdir(parents=True,exist_ok=False,mode=0o700)
 receipt={'status':'backup_pending','sql_transaction':'single complete content transaction','expected_counts':config['expected_counts'],'native_gameplay_proof':False}
 def save(): (backup/'installation.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
 save()
 try:
  with os.fdopen(os.open(backup/'database-before.sql',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600),'wb')as f:
   subprocess.run(dump+['--single-transaction','--skip-lock-tables','--skip-add-locks','--no-tablespaces','--routines','--triggers','--events','--hex-blob'],stdout=f,check=True)
 except Exception:
  receipt['status']='backup_failed_no_content_writes';save();raise
 receipt['status']='backup_complete';save()
 try:subprocess.run(questcmd+['--apply','--backup',str(backup/'quests')],check=True)
 except Exception:
  receipt['status']='quest_composition_failed_no_SQL_run';save();raise
 receipt['status']='quests_composed_database_pending';save()
 def restore_quests():
  for row in reversed(read_json(backup/'quests/changes.json')):
   dst=safe(a.quests,row['path'])
   if not dst.is_file()or digest(dst)!=row['after_sha256']:
    receipt['status']='SQL_failed_quest_rollback_drift_keep_writers_stopped';save();raise SystemExit('Quest files changed after composition; restore the private backup with writers stopped.')
   if row['before_sha256']is None:dst.unlink()
   else:dst.write_bytes(safe(backup/'quests',row['path']).read_bytes())
 try:r=subprocess.run(client+['--batch'],input=sql,text=True,encoding='utf-8')
 except Exception:
  restore_quests();receipt['status']='SQL_exception_quests_restored_verify_commit_state';save();raise
 if r.returncode:
  restore_quests();receipt['status']='SQL_failed_quests_restored_verify_commit_state';save();raise SystemExit('SQL failed; exact quest files restored. Keep writers stopped and inspect database state against the backup: a connection failure near commit can leave its outcome uncertain.')
 receipt['status']='installed_requires_native_acceptance';save()
 print('Whole Vergalid batch installed. No services started; verify native gameplay before declaring completion.')

if __name__=='__main__':main()
