#!/usr/bin/env python3
"""Check or install the complete Mesa quest extension, including every SQL/script file."""
import argparse,hashlib,json,pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mysql-command-json',required=True);p.add_argument('--dump-command-json',help='MariaDB dump argument array; required with --apply');p.add_argument('--quests',required=True,type=pathlib.Path);p.add_argument('--backup',type=pathlib.Path);p.add_argument('--apply',action='store_true');a=p.parse_args()
def command(value):
 c=json.loads(value)
 if not isinstance(c,list)or not c or not all(isinstance(v,str)and v for v in c):p.error('Expected nonempty command array')
 if any(v=='-f'or v.startswith(('--force','--execute','--init-command'))for v in c):p.error('Force/execute/init options are not supported')
 return c
client=command(a.mysql_command_json)
for r in json.loads((ROOT/'payload-manifest.json').read_text()):
 f=ROOT/r['path']
 if not f.is_file()or hashlib.sha256(f.read_bytes()).hexdigest()!=r['sha256']:raise SystemExit('Release integrity mismatch: '+r['path'])
subprocess.run([sys.executable,str(ROOT/'preflight.py'),'--mysql-command-json',a.mysql_command_json],check=True)
subprocess.run([sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(a.quests)],check=True)
if not a.apply:
 print('Preflight complete. No writes. --apply installs all SQL and quest files; first stop game services and supply --backup and --dump-command-json.');sys.exit(0)
if not a.backup or not a.dump_command_json:p.error('--apply requires --backup and --dump-command-json')
backup=a.backup.resolve();quests=a.quests.resolve()
if backup==quests or quests in backup.parents:raise SystemExit('Backup must be outside the quest directory')
dump=command(a.dump_command_json);backup.mkdir(parents=True,exist_ok=False)
tables=['achievements','achievement_components','achievement_categories','achievement_category_associations','npc_spells','tasks','task_activities','achievement_criteria','npc_types','spawnentry','loottable','lootdrop','loottable_entries','lootdrop_entries','tradeskill_recipe','tradeskill_recipe_entries','reward_sets','reward_sources','reward_options','reward_option_entries','rewards']
# The supplied dump command ends with the database name. Only content tables
# are backed up; never publish this backup or private connection configuration.
with(backup/'database-content.sql').open('wb')as out:
 out.write(('SET FOREIGN_KEY_CHECKS=0;\nSTART TRANSACTION;\n'+''.join('DELETE FROM `'+t+'`;\n'for t in reversed(tables))).encode())
 subprocess.run(dump+['--single-transaction','--skip-lock-tables','--skip-add-locks','--no-tablespaces','--no-create-info','--complete-insert']+tables,stdout=out,check=True)
 out.write(b'COMMIT;\nSET FOREIGN_KEY_CHECKS=1;\n')
sql='\n'.join(f.read_text()for f in sorted((ROOT/'sql').glob('*.sql')))
r=subprocess.run(client+['--batch'],input=sql,text=True)
if r.returncode:raise SystemExit('SQL import stopped. Keep game services stopped. Inspect the error and restore your backup before retrying; no quest files were changed.')
subprocess.run([sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(quests),'--apply','--backup',str(backup/'quests')],check=True)
(backup/'installation.json').write_text(json.dumps({'status':'All content SQL and scripts installed','database_tables':tables,'quest_backup':'quests','note':'Deployment does not start/restart services or claim native gameplay acceptance on this server.'},indent=2)+'\n')
print('Installed the complete Mesa extension. Deploy the matching engine and restart all game services before players reconnect.')
