#!/usr/bin/env python3
"""Preflight/install the complete Short Sharp Sting package in one transaction."""
from pathlib import Path
import argparse,datetime,json,os,sys,uuid,types
import package_tools as T
ROOT=Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--quests',type=Path,required=True)
 p.add_argument('--additional-quests',type=Path,action='append',default=[])
 p.add_argument('--server-source',type=Path,required=True,help='EQEmu source used to build the stopped game server')
 p.add_argument('--mysql-command-json',required=True,help='Complete SQL client argument array with --batch --raw --skip-column-names; no --force')
 p.add_argument('--apply',action='store_true')
 p.add_argument('--backup',type=Path,help='New backup folder outside every quest tree')
 a=p.parse_args();dest=a.quests.resolve();additional=[v.resolve()for v in a.additional_quests];cmd=T.command(a.mysql_command_json)
 T.integrity();T.source_preflight(a.server_source.resolve());plan=T.contract();info,before=T.database_preflight(cmd,plan);changes=T.quest_plan(dest,additional)
 print(json.dumps({'preflight':'PASS','database':info['database'],'quest_files_to_compose':len(changes),'content_tables':sorted(plan),'scope':'No character, inventory, housing, guild or player-progress tables are written'},indent=2))
 if not a.apply:
  try:T.reviewed();print('Review: ready for installation while game services are stopped')
  except T.Stop as e:print('Review: not ready for activation: '+str(e))
  print('No writes performed. Source checks do not prove an installed binary was rebuilt.')
  return 0
 T.require(a.backup is not None,'--apply requires a new --backup folder outside the quest tree')
 review=T.reviewed();T.require(not T.game_writers(dest.parent),'Game services for this installation are still running; stop all game writers before applying')
 backup=a.backup.resolve();T.require(all(backup!=r and r not in backup.parents for r in [dest]+additional),'Backup must be outside every quest tree');T.require(not backup.exists(),'Backup folder must be new')
 # One installer per quest root. An interrupted lock is intentionally retained
 # if the process dies; inspect its backup receipt before manual recovery.
 lock=dest/'.rof2-sting.install.lock';token=uuid.uuid4().hex
 fd=os.open(str(lock),os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
 try:
  os.write(fd,json.dumps({'pid':os.getpid(),'token':token,'backup':str(backup)}).encode());os.fsync(fd)
  backup.mkdir(parents=True,exist_ok=False);os.chmod(backup,0o700)
  saved={};file_rows=[]
  for rel,data in changes.items():
   f=T.safe(dest,rel);old=f.read_bytes()if f.exists()else None;saved[rel]=old
   file_rows.append({'path':rel,'before_sha256':T.sha(old)if old is not None else None,'after_sha256':T.sha(data)})
   if old is not None:T.atomic_write(backup/'quests'/rel,old)
  T.atomic_write(backup/'content-rows-before.json',(json.dumps(before,indent=2)+'\n').encode())
  T.integrity();sql=T.sql_batch(review);T.atomic_write(backup/'applied-content.sql',sql.encode())
  receipt={'state':'backed_up','database':info['database'],'review_sha256':review,'sql_sha256':T.sha(sql.encode()),'files':file_rows,'player_data_mutations':False,'backup_scope':'Only affected content rows and changed quest files; no full database dump'}
  def record(state):
   receipt['state']=state;T.atomic_write(backup/'receipt.json',(json.dumps(receipt,indent=2)+'\n').encode())
  record('backed_up')
  written=[]
  def restore_files():
   for rel in reversed(written):
    old=saved[rel]
    f=T.safe(dest,rel)
    if old is None:
     if f.exists():f.unlink()
    else:T.atomic_write(f,old)
  try:
   # Refuse a changed preimage between preflight and backup/install.
   T.require(T.same_snapshot(T.snapshot(cmd,plan),before),'Reserved content changed during preflight; re-run after reviewing it')
   T.require(not T.game_writers(dest.parent),'A game writer started during preflight')
   T.require(T.quest_plan(dest,additional)==changes,'Quest files changed during preflight; re-run after reviewing them')
   for rel,data in changes.items():
    f=T.safe(dest,rel);current=f.read_bytes()if f.exists()else None
    T.require(current==saved[rel],'Quest preimage changed before write: '+rel)
    T.atomic_write(f,data);written.append(rel)
   record('quest_files_installed_sql_pending')
  except Exception:
   restore_files();record('rolled_back_before_sql');raise
  try:result=T.run_sql(cmd,sql)
  except Exception as error:result=types.SimpleNamespace(returncode=-1,stderr=str(error))
  # A lost SQL connection can make COMMIT outcome ambiguous. Inspect exact
  # reserved rows rather than blindly restoring scripts after a possible commit.
  try:after=T.snapshot(cmd,plan)
  except Exception:
   record('sql_outcome_unknown_keep_services_stopped');raise T.Stop('Cannot determine SQL commit outcome. Keep services stopped and inspect '+str(backup))
  if T.matches(after,plan)and(result.returncode==0 or not T.same_snapshot(after,before)):
   record('complete');print('Installed all required quest scripts, both hooks, quests, recipe, rewards and achievement bindings.')
   if result.returncode:print('SQL client lost confirmation; exact committed content was verified before declaring completion.')
   print('Rebuild/deploy the required engine and restart game services before native acceptance. Backup: '+str(backup));return 0
  if T.same_snapshot(after,before):
   restore_files();record('rolled_back');raise T.Stop('SQL did not confirm completion; original content state and quest files restored. '+result.stderr[-1800:])
  record('content_outcome_unknown_keep_services_stopped')
  raise T.Stop('Unexpected content state; automatic rollback could overwrite another change. Keep services stopped and inspect '+str(backup))
 finally:
  os.close(fd)
  if lock.exists():
   try:owned=T.load(lock).get('token')==token
   except(ValueError,OSError):owned=False
   if owned:lock.unlink()

if __name__=='__main__':
 try:raise SystemExit(main())
 except(T.Stop,OSError,ValueError)as e:raise SystemExit('STOP: '+str(e))
