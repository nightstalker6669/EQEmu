#!/usr/bin/env python3
"""Install every required Mesa centaur SQL/script component using existing overlay tooling."""
import argparse,hashlib,json,pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--mysql-command-json',required=True);p.add_argument('--dump-command-json');p.add_argument('--quests',type=pathlib.Path,required=True);p.add_argument('--backup',type=pathlib.Path);p.add_argument('--apply',action='store_true');a=p.parse_args()
def command(value):
 c=json.loads(value)
 if not isinstance(c,list)or not c or not all(isinstance(v,str)and v for v in c):p.error('Expected a command argument array')
 if any(v=='-f'or v.startswith(('--force','--execute','--init-command'))for v in c):p.error('Force/execute/init options are not supported')
 return c
def fail(message):raise SystemExit(message)
manifest=json.loads((ROOT/'payload-manifest.json').read_text())
for row in manifest['files']:
 f=ROOT/row['path']
 if not f.is_file()or hashlib.sha256(f.read_bytes()).hexdigest()!=row['sha256']:fail('Release file changed: '+row['path'])
# The existing overlay helper checks only affected scripts and their active aliases.
check=subprocess.run([sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(a.quests)])
if check.returncode:fail('Resolve the reported affected quest-script conflict before installing.')
if not a.apply:
 print('Source paths checked; no writes. Use --apply with a backup and database dump command to install every required component.');sys.exit(0)
if not a.backup or not a.dump_command_json:p.error('--apply requires --backup and --dump-command-json')
client=command(a.mysql_command_json);dump=command(a.dump_command_json);backup=a.backup.resolve();quests=a.quests.resolve()
if backup==quests or quests in backup.parents:fail('Put the backup outside the quest directory.')
backup.mkdir(parents=True,exist_ok=False)
# Back up affected rows only; this does not dump whole content/player/housing tables.
scopes={'tasks':'id BETWEEN 3040461 AND 3040464','task_activities':'taskid BETWEEN 3040461 AND 3040464','achievement_criteria':'achievement_id=500041000 AND component_type=1 AND component_id BETWEEN 50171 AND 50174','reward_sets':'reward_set_id=3040464','reward_sources':'(source_type=2 AND source_id=3040464)OR reward_set_id=3040464','reward_options':'reward_set_id=3040464','reward_option_entries':'reward_set_id=3040464 OR reward_id BETWEEN 304046401 AND 304046403','rewards':'reward_id BETWEEN 304046401 AND 304046403','npc_faction':'id IN(3040461,3040462)','npc_types':'id=397196','spawn2':'id=112270'}
for table,where in scopes.items():
 with(backup/(table+'.sql')).open('wb')as out:
  r=subprocess.run(dump+['--single-transaction','--skip-lock-tables','--skip-add-locks','--no-tablespaces','--no-create-info','--complete-insert','--where='+where,table],stdout=out,stderr=subprocess.PIPE)
 if r.returncode:fail('Affected-row backup failed for '+table+'; nothing has been installed.')
forward=manifest['install_sql']
text='\n'.join('-- REQUIRED FILE: '+name+'\n'+(ROOT/name).read_text()for name in forward)
text+="\nSELECT 'centaur tasks',COUNT(*)FROM tasks WHERE id BETWEEN 3040461 AND 3040464 AND enabled=1;\nSELECT 'centaur criteria',COUNT(*)FROM achievement_criteria WHERE achievement_id=500041000 AND component_id BETWEEN 50171 AND 50174 AND enabled=1;\nSELECT 'reward choices',COUNT(*)FROM reward_options WHERE reward_set_id=3040464 AND enabled=1;\nSELECT id,hp,mindmg,maxdmg,see_invis FROM npc_types WHERE id=397196;\nSELECT id,respawntime,variance FROM spawn2 WHERE id=112270;\n"
r=subprocess.run(client+['--batch'],input=text,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
(backup/'sql-output.txt').write_text(r.stdout);(backup/'sql-error.txt').write_text(r.stderr)
(backup/'installation.json').write_text(json.dumps({'sql_files':forward,'affected_row_backups':scopes,'sql_returncode':r.returncode,'scripts_installed':False},indent=2)+'\n')
if r.returncode:fail('SQL stopped on a concrete conflict/error. Keep services stopped; inspect the saved error and affected-row backups. No quest files were installed; do not silently retry.')
r=subprocess.run([sys.executable,str(ROOT/'apply_quests.py'),'--quests',str(quests),'--apply','--backup',str(backup/'quests')])
if r.returncode:fail('Content SQL installed but script installation stopped. Keep services stopped and recover using the saved backups; do not retry blindly.')
(backup/'installation.json').write_text(json.dumps({'status':'All required centaur content SQL and scripts installed','sql_files':forward,'affected_row_backups':scopes,'scripts_installed':True,'restart':'Restart game services to load the completed installation. Installer does not control services.'},indent=2)+'\n')
print('Installed the complete four-quest centaur package, rewards and corruption encounter. Restart game services before players reconnect.')
