#!/usr/bin/env python3
"""Install the whole Sunderock candidate; inspect by default. No services controlled."""
import argparse,hashlib,json,pathlib,re,subprocess,sys,os
ROOT=pathlib.Path(__file__).resolve().parent

def command(value):
 c=json.loads(value)
 if not isinstance(c,list)or not c or not all(isinstance(x,str)and x for x in c):raise ValueError('Expected a nonempty command array')
 if any(x in('-e','-f')or x.startswith(('--force','--execute','--init-command'))for x in c):raise ValueError('Force/execute/init options are not supported')
 return c

def full_sql():
 # CREATE TEMPORARY TABLE does not commit in MariaDB. Eliminate the component
 # transaction boundaries and keep their success latches for the activation guard.
 # A client error closes the connection and rolls back every permanent mutation.
 parts=[]
 for f in sorted((ROOT/'sql').glob('*.sql')):
  s=f.read_text()
  s=re.sub(r'(?m)^START TRANSACTION;\s*$','',s)
  s=re.sub(r'(?m)^COMMIT;\s*$','',s)
  s=re.sub(r'(?m)^SET @sunderock_(?:ok|world_ok)=NULL;\s*$','',s)
  parts.append(s)
 return 'SET @sunderock_reviewed=1;SET @sunderock_world_reviewed=1;SET @sunderock_activation_reviewed=1;\nSTART TRANSACTION;\n'+'\n'.join(parts)+'\nCOMMIT;\n'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--mysql-command-json',required=True);p.add_argument('--dump-command-json');p.add_argument('--quests',required=True,type=pathlib.Path);p.add_argument('--additional-quests',action='append',type=pathlib.Path,default=[]);p.add_argument('--backup',type=pathlib.Path);p.add_argument('--apply',action='store_true');p.add_argument('--services-stopped',action='store_true');p.add_argument('--matching-engine-installed',action='store_true');a=p.parse_args()
 try:client=command(a.mysql_command_json)
 except(ValueError,json.JSONDecodeError)as e:p.error(str(e))
 for row in json.loads((ROOT/'payload-manifest.json').read_text()):
  rel=pathlib.PurePosixPath(row['path'])
  if rel.is_absolute()or'..'in rel.parts:raise SystemExit('Unsafe payload path')
  f=ROOT/rel
  if not f.is_file()or f.is_symlink()or hashlib.sha256(f.read_bytes()).hexdigest()!=row['sha256']:raise SystemExit('Payload integrity mismatch: '+str(rel))
 questcmd=[sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(a.quests)]
 for q in a.additional_quests:questcmd+=['--additional-quests',str(q)]
 subprocess.run([sys.executable,str(ROOT/'preflight.py'),'--mysql-command-json',a.mysql_command_json],check=True)
 subprocess.run(questcmd,check=True)
 if not a.apply:
  print('Inspection passed. No database or quest files changed. Apply requires fresh backup, stopped writers, and matching engine.');return
 if not(a.backup and a.dump_command_json and a.services_stopped and a.matching_engine_installed):p.error('--apply requires backup/dump command, stopped writers, and matching engine')
 # This is a complete candidate, not a tool that silently installs only scripts or
 # only SQL. Its generator stages the enabled module explicitly for installation.
 module=(ROOT/'quests/lua_modules/achievement_quest_sunderock_data.lua').read_text()
 if module.count('["enabled"]=true')!=16 or '["enabled"]=false'in module:raise SystemExit('The complete enabled module has not been staged; do not install a partial candidate')
 backup=a.backup.absolute()
 if backup.is_symlink():raise SystemExit('Backup cannot be a symlink')
 backup=backup.resolve()
 for q in[a.quests]+a.additional_quests:
  q=q.resolve()
  if backup==q or q in backup.parents:raise SystemExit('Backup must be outside all quest roots')
 try:dump=command(a.dump_command_json)
 except(ValueError,json.JSONDecodeError)as e:p.error(str(e))
 backup.mkdir(parents=True,exist_ok=False,mode=0o700)
 receipt={'status':'backup_pending','sql_transaction':'single all-content transaction','database_backup':'database-before.sql','quest_backup':'quests','native_gameplay_proof':False}
 def save(): (backup/'installation.json').write_text(json.dumps(receipt,indent=2)+'\n')
 save()
 with os.fdopen(os.open(backup/'database-before.sql',os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600),'wb')as f:
  subprocess.run(dump+['--single-transaction','--skip-lock-tables','--skip-add-locks','--no-tablespaces','--routines','--triggers','--events','--hex-blob'],stdout=f,check=True)
 receipt['status']='backup_complete';save()
 # Compose with rollback support while all writers are stopped; if SQL fails,
 # restore every composed quest file from the exact application receipt.
 subprocess.run(questcmd+['--apply','--backup',str(backup/'quests')],check=True)
 receipt['status']='quests_composed_database_pending';save()
 r=subprocess.run(client+['--batch'],input=full_sql(),text=True)
 if r.returncode:
  for row in reversed(json.loads((backup/'quests/changes.json').read_text())):
   dst=a.quests/row['path']
   if hashlib.sha256(dst.read_bytes()).hexdigest()!=row['after_sha256']:raise SystemExit('SQL failed; quest rollback encountered drift. Keep writers stopped and restore the private backup.')
   if row['before_sha256']is None:dst.unlink()
   else:dst.write_bytes((backup/'quests'/row['path']).read_bytes())
  receipt['status']='sql_failed_quests_restored_verify_database_commit_state';save();raise SystemExit('SQL failed and quest files were restored. Guard failures roll back the single transaction; connection failures can leave commit outcome uncertain. Keep writers stopped and inspect database state against the backup.')
 receipt['status']='installed_requires_native_acceptance';save()
 print('Whole Sunderock candidate installed and activated. No services started. Native gameplay and remaining historical fidelity work are not proved by this receipt.')

if __name__=='__main__':main()
