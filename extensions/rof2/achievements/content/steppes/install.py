#!/usr/bin/env python3
"""Install the complete Steppes payload, including activation. Read-only by default."""
import argparse,hashlib,json,pathlib,re,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--mysql-command-json',required=True);p.add_argument('--dump-command-json');p.add_argument('--quests',required=True,type=pathlib.Path);p.add_argument('--additional-quests',action='append',type=pathlib.Path,default=[]);p.add_argument('--backup',type=pathlib.Path);p.add_argument('--apply',action='store_true');p.add_argument('--services-stopped',action='store_true',help='Confirm all game/content writers are stopped');p.add_argument('--matching-engine-installed',action='store_true',help='Confirm the matching Steppes engine was built and deployed');a=p.parse_args()
def command(value):
 c=json.loads(value)
 if not isinstance(c,list)or not c or not all(isinstance(v,str)and v for v in c):p.error('Expected nonempty command array')
 if any(v in('-e','-f')or v.startswith(('--force','--execute','--init-command'))for v in c):p.error('Force/execute/init options not supported')
 return c
client=command(a.mysql_command_json)
for row in json.loads((ROOT/'payload-manifest.json').read_text()):
 f=ROOT/row['path']
 if not f.is_file()or hashlib.sha256(f.read_bytes()).hexdigest()!=row['sha256']:raise SystemExit('Payload integrity mismatch: '+row['path'])
questcmd=[sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(a.quests)]
for root in a.additional_quests:questcmd+=['--additional-quests',str(root)]
subprocess.run([sys.executable,str(ROOT/'preflight.py'),'--mysql-command-json',a.mysql_command_json],check=True)
subprocess.run(questcmd,check=True)
if not a.apply:
 print('Read-only preflight complete. Apply requires a fresh external backup directory, database dump command, stopped writers and matching deployed engine.');sys.exit(0)
if not(a.backup and a.dump_command_json and a.services_stopped and a.matching_engine_installed):p.error('--apply requires --backup, --dump-command-json, --services-stopped and --matching-engine-installed')
backup=a.backup.absolute()
if backup.is_symlink():raise SystemExit('Backup cannot be a symlink')
backup=backup.resolve()
for root in[a.quests]+a.additional_quests:
 root=root.resolve()
 if backup==root or root in backup.parents:raise SystemExit('Backup must be outside every quest root')
dump=command(a.dump_command_json);backup.mkdir(parents=True,exist_ok=False)
# Full private backup preserves every housing/player/content table and existing policy.
# DDL is not transactional in MariaDB. A later SQL guard failure requires restoring
# this backup while writers remain stopped; never continue with --force.
with(backup/'database-before.sql').open('wb')as f:
 subprocess.run(dump+['--single-transaction','--skip-lock-tables','--skip-add-locks','--no-tablespaces','--routines','--triggers','--events','--hex-blob'],stdout=f,check=True)
files=sorted((ROOT/'sql').glob('*.sql'));text='\n'.join(f.read_text()for f in files)
flags=sorted(set(re.findall(r'@steppes_(?:[a-z_]+_)?reviewed',text)))
sql='\n'.join('SET '+flag+'=1;'for flag in flags)+'\n'+text
receipt={'status':'backup_complete','sql_files':[f.name for f in files],'activation_included':True,'quest_backup':'quests','database_backup':'database-before.sql','private_backup':True}
record=backup/'installation.json';record.write_text(json.dumps(receipt,indent=2)+'\n')
r=subprocess.run(client+['--batch'],input=sql,text=True)
if r.returncode:
 receipt['status']='sql_failed_keep_writers_stopped_restore_backup';record.write_text(json.dumps(receipt,indent=2)+'\n');raise SystemExit('SQL stopped. No quest files changed. Keep writers stopped and restore database-before.sql before retrying.')
receipt['status']='sql_complete_quest_install_pending';record.write_text(json.dumps(receipt,indent=2)+'\n')
try:subprocess.run(questcmd+['--apply','--backup',str(backup/'quests')],check=True)
except Exception:
 receipt['status']='quest_install_failed_keep_writers_stopped';record.write_text(json.dumps(receipt,indent=2)+'\n');raise
receipt['status']='installed_requires_server_start_and_native_acceptance';record.write_text(json.dumps(receipt,indent=2)+'\n')
print('Complete Steppes SQL, scripts and activation installed. Start the matching engine and verify gameplay. No services controlled; this receipt is not native acceptance.')
