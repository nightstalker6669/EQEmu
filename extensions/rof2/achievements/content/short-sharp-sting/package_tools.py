"""Shared installer checks. No game-client control and no player-table writes."""
from pathlib import Path, PurePosixPath
import hashlib,json,os,re,stat,subprocess,tempfile
ROOT=Path(__file__).resolve().parent
ALLOWED={'tasks','task_activities','tradeskill_recipe','tradeskill_recipe_entries','achievement_criteria','npc_types','reward_sets','reward_sources','reward_options','reward_option_entries','rewards','npc_faction','npc_faction_entries'}
class Stop(RuntimeError):pass
def require(ok,message):
 if not ok:raise Stop(message)
def sha(data):return hashlib.sha256(data).hexdigest()
def load(path):return json.loads(path.read_text(encoding='utf-8'))
def safe(root,rel):
 p=PurePosixPath(rel);require(not p.is_absolute()and '..'not in p.parts and '\\'not in rel,'Unsafe relative path: '+rel)
 cur=root
 for part in p.parts:
  cur=cur/part
  try:s=cur.lstat()
  except FileNotFoundError:continue
  require(not stat.S_ISLNK(s.st_mode)and not(getattr(s,'st_file_attributes',0)&0x400),'Symlink/junction in managed path: '+str(cur))
 return cur
def integrity():
 rows=load(ROOT/'payload-manifest.json');require(rows,'Empty release manifest')
 for r in rows:
  f=safe(ROOT,r['path']);require(f.is_file()and sha(f.read_bytes())==r['sha256'],'Release integrity mismatch: '+r['path'])
def quote(v):
 if v is None:return 'NULL'
 if isinstance(v,(int,float)):return str(v)
 return "'"+str(v).replace('\\','\\\\').replace("'","''")+"'"
def command(value):
 try:c=json.loads(value)
 except (ValueError,TypeError):raise Stop('Database command must be a JSON argument array')
 require(isinstance(c,list)and c and all(isinstance(s,str)and s for s in c),'Database command must be a nonempty string array')
 require(not any(s in('-e','--execute','--force')or s.startswith(('--execute=','--init-command','--force='))for s in c),'Database execute/init/force options are not supported')
 return c
def run_sql(cmd,sql):return subprocess.run(cmd,input=sql,text=True,encoding='utf-8',capture_output=True)
def query(cmd,sql):
 r=run_sql(cmd,sql);require(r.returncode==0,'Database read failed: '+r.stderr[-2000:])
 try:return [json.loads(line)for line in r.stdout.splitlines()if line.strip()]
 except ValueError:raise Stop('Database command must return one raw JSON object per line; use --batch --raw --skip-column-names')
def contract():
 combined={}
 for rel in load(ROOT/'package.json')['row_contracts']:
  for table,d in load(safe(ROOT,rel)).items():
   require(table in ALLOWED,'Unexpected write table: '+table)
   v=combined.setdefault(table,{'scopes':[],'rows':[],'accepted_existing':[]});v['scopes'].append(d['scope']);v['rows']+=d['rows'];v['accepted_existing']+=d.get('accepted_existing',[])
 for d in combined.values():
  for r in d['rows']:
   if 'enabled'in r:r['enabled']=1
 return combined
def snapshot(cmd,plan):
 out={t:[]for t in plan};names=','.join(quote(t)for t in plan)
 metadata=query(cmd,"SELECT JSON_OBJECT('table',TABLE_NAME,'name',COLUMN_NAME) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN("+names+') ORDER BY TABLE_NAME,ORDINAL_POSITION;')
 bytable={t:[]for t in plan}
 for r in metadata:bytable[r['table']].append(r['name'])
 statements=[]
 for table,d in plan.items():
  cols=bytable[table];require(cols and all(re.fullmatch('[a-zA-Z0-9_]+',n)for n in cols),'Missing/invalid table schema: '+table)
  for row in d['rows']:require(set(row)<=set(cols),'Missing required columns: '+table)
  expr=','.join(quote(n)+',`'+n+'`'for n in cols)
  statements.append("SELECT JSON_OBJECT('table',"+quote(table)+",'row',JSON_OBJECT("+expr+")) FROM `"+table+'` WHERE '+' OR '.join('('+v+')'for v in d['scopes'])+';')
 for r in query(cmd,'\n'.join(statements)):out[r['table']].append(r['row'])
 return out

def normalize(rows):return sorted(json.dumps(r,sort_keys=True,separators=(',',':'))for r in rows)
def matches(current,desired,permit_missing=False):
 unique=load(ROOT/'baseline.json')['unique_keys']
 for table,want in desired.items():
  identities=[key for key in unique[table]if all(set(key)<=set(r)for r in want['rows'])]
  require(identities,'No complete packaged identity for '+table)
  pk=min(identities,key=len);remaining=list(want['rows'])
  for actual in current[table]:
   candidates=[r for r in remaining if all(actual[k]==r[k]for k in pk)]
   if len(candidates)!=1:return False
   target=candidates[0];projected={k:actual[k]for k in target}
   if permit_missing and 'enabled'in projected:
    if projected['enabled']not in(0,1):return False
    projected['enabled']=1
   if projected!=target:
    allowed=want.get('accepted_existing',[])if permit_missing else []
    if projected not in allowed:return False
   remaining.remove(target)
  if remaining and not permit_missing:return False
 return True
def database_preflight(cmd,plan):
 info=query(cmd,"SELECT JSON_OBJECT('database',DATABASE(),'version',VERSION());")[0]
 require(info['database']and info['database']not in('mysql','sys','information_schema','performance_schema'),'Select the target game database')
 require('mariadb'in info['version'].lower(),'This transaction installer currently requires MariaDB')
 names=','.join(quote(t)for t in plan)
 engines=query(cmd,'SELECT JSON_OBJECT(\'name\',TABLE_NAME,\'engine\',ENGINE) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN('+names+');')
 require(len(engines)==len(plan)and all(r['engine']=='InnoDB'for r in engines),'Every affected content table must be InnoDB')
 metadata=query(cmd,"SELECT JSON_OBJECT('table',TABLE_NAME,'name',INDEX_NAME,'column',COLUMN_NAME,'seq',SEQ_IN_INDEX) FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=DATABASE() AND NON_UNIQUE=0 AND TABLE_NAME IN("+names+') ORDER BY TABLE_NAME,INDEX_NAME,SEQ_IN_INDEX;')
 indexes={t:{}for t in plan}
 for r in metadata:indexes[r['table']].setdefault(r['name'],[]).append(r['column'])
 expected=load(ROOT/'baseline.json')['unique_keys']
 require(all(sorted(indexes[t].values())==expected[t]for t in plan),'Reserved content unique keys differ; review before installing')
 triggers=query(cmd,"SELECT JSON_OBJECT('name',TRIGGER_NAME,'table',EVENT_OBJECT_TABLE) FROM information_schema.TRIGGERS WHERE TRIGGER_SCHEMA=DATABASE() AND EVENT_OBJECT_TABLE IN("+names+');')
 require(not triggers,'Custom content-table triggers need review before installation')
 before=snapshot(cmd,plan);require(matches(before,plan,True),'Custom reserved content rows require a manual merge')
 # Read-only identity checks; SQL repeats them inside the write transaction.
 checks=[]
 for table,wanted in load(ROOT/'baseline.json')['identities'].items():
  for row in wanted:
   predicate=' AND '.join('`'+k+'` <=> '+quote(v)for k,v in row.items())
   label='Original identity '+table+'/'+str(row.get('id',row.get('achievement_id')))
   checks.append("SELECT JSON_OBJECT('label',"+quote(label)+",'n',COUNT(*)) FROM `"+table+'` WHERE '+predicate+';')
 for r in load(ROOT/'baseline.json')['spawn_routes']:
  fields={'s.id':r['id'],'s.spawngroupID':r['group'],'e.npcID':r['npc'],'s.zone':r['zone'],'s.x':r['x'],'s.y':r['y'],'s.z':r['z']}
  predicate=' AND '.join(k+' <=> '+quote(v)for k,v in fields.items())
  checks.append("SELECT JSON_OBJECT('label',"+quote('Original actor placement '+str(r['id']))+",'n',COUNT(*)) FROM spawn2 s JOIN spawnentry e ON e.spawngroupID=s.spawngroupID WHERE "+predicate+';')
 for r in query(cmd,'\n'.join(checks)):require(r['n']==1,r['label']+' differs')
 wanted=load(ROOT/'baseline.json')['loot_items']
 sql="SELECT DISTINCT JSON_OBJECT('id',d.item_id) FROM npc_types n JOIN loottable_entries l ON l.loottable_id=n.loottable_id JOIN lootdrop_entries d ON d.lootdrop_id=l.lootdrop_id JOIN spawnentry e ON e.npcid=n.id JOIN spawn2 s ON s.spawngroupID=e.spawngroupID WHERE s.zone IN('moors','stonehive') AND d.chance>0 AND d.multiplier>0 AND l.probability>0 AND l.multiplier>0 AND e.chance>0 AND d.item_id IN("+','.join(map(str,wanted))+');'
 require({r['id']for r in query(cmd,sql)}==set(wanted),'Original positive-chance acquisition route missing')
 return info,before
def settings_text(d):
 b=d['buzz']
 if not b['reviewed']:return "-- Required original values remain unset; activation is refused.\nreturn {buzz={reviewed=false,copy_npc=396941,faction_id=nil,maximum_raw_faction=nil,copy_positions=nil}}\n"
 require(b['copy_npc']==396941 and isinstance(b['faction_id'],int)and b['faction_id']>0,'Invalid reviewed Prankster identity')
 require(type(b['maximum_raw_faction'])is int and b['maximum_raw_faction']<0,'Invalid reviewed faction threshold')
 points=b['copy_positions'];require(isinstance(points,list)and len(points)==3,'Three reviewed copy positions required')
 pieces=[]
 for p in points:
  for k in ('x','y','z','h'):
   v=p[k];require(type(v)in(int,float)and abs(v)!=float('inf')and v==v,'Non-finite reviewed placement')
  require(type(p['immediate_attack'])is bool,'Each copy needs an explicit attack role')
  pieces.append('{'+','.join(k+'='+str(p[k])for k in('x','y','z','h'))+',immediate_attack='+str(p['immediate_attack']).lower()+'}')
 require(sum(p['immediate_attack']for p in points)==2,'Exactly two copies must attack immediately')
 return '-- Reviewed settings; provenance in review.json.\nreturn {buzz={reviewed=true,copy_npc=396941,faction_id='+str(b['faction_id'])+',maximum_raw_faction='+str(b['maximum_raw_faction'])+',copy_positions={'+','.join(pieces)+'}}}\n'
def reviewed():
 d=load(ROOT/'review.json');require(d['ready']is True,'Package review incomplete: '+ '; '.join(d.get('remaining',[])))
 require(d.get('faction_routes_reviewed')is True,'Original kill-faction routes still need review')
 policy=load(ROOT/'faction-policy.json')
 require(d['buzz']['faction_id']==policy['gate_faction_id']and d['buzz']['maximum_raw_faction']==policy['maximum_raw_faction'],'Faction policy and reviewed gate differ')
 hits={str(r['faction_id']):r['value']for r in load(ROOT/'faction-row-contract.json')['npc_faction_entries']['rows']}
 require(hits==policy['kill_hits'],'Faction policy and packaged kill amounts differ')
 expected=settings_text(d);actual=(ROOT/'quests/lua_modules/achievement_quest_sting_settings.lua').read_text(encoding='utf-8')
 require(expected==actual,'Reviewed settings and Lua configuration differ')
 return sha(json.dumps(d,sort_keys=True,separators=(',',':')).encode())
def source_preflight(root):
 requirements=load(ROOT/'baseline.json')['engine_requirements']
 for rel,needles in requirements.items():
  p=safe(root,rel);require(p.is_file(),'Engine source unavailable: '+rel)
  text=p.read_text(encoding='utf-8');require(all(n in text for n in needles),'Missing required engine change: '+rel)
 # This checks source prerequisites, not the identity of an installed binary.
def quest_plan(dest,additional):
 roots=[dest]+additional;require(all(r.is_dir()for r in roots),'Every quest root must exist')
 baseline=load(ROOT/'baseline.json');dependency=safe(dest,'lua_modules/achievement_quest_blightfire.lua')
 require(dependency.is_file()and sha(dependency.read_bytes())in baseline['shared_delivery_sha256'],'Install the complete Stone Hive foundation first, or merge its shared delivery module')
 changes={}
 for rel in load(ROOT/'quest-manifest.json'):
  source=safe(ROOT/'quests',rel).read_bytes();target=safe(dest,rel)
  if target.exists()and target.read_bytes()!=source:
   require(sha(target.read_bytes())in baseline.get('accepted_previous_scripts',{}).get(rel,[]),'Custom managed callback requires merge: '+rel)
   changes[rel]=source
  if not target.exists():changes[rel]=source
  if rel.startswith('lua_modules/'):
   for other in additional:require(not safe(other,rel).exists(),'Shared module conflict in additional quest root: '+rel)
   continue
  zone,filename=rel.split('/');id=PurePosixPath(filename).stem;name=baseline['npc_names'][id]
  for r in roots:
   folders=[zone,'global']+[zone+'/'+v.name for v in (r/zone).glob('v*')if v.is_dir()and v.name[1:].isdigit()]
   for folder in folders:
    for stem in (id,name,name.lstrip('#'),name+'_'+id,name.lstrip('#')+'_'+id):
     for ext in('.lua','.pl'):
      alt=folder+'/'+stem+ext
      if r==dest and alt==rel:continue
      require(not safe(r,alt).exists(),'Alternate NPC callback requires merge: '+str(r/alt))
  # Native dispatch calls both local and global handlers. Permit only exact
  # reviewed local compositions; do not infer dynamic Lua compatibility from
  # a superficial function-name scan.
 rel='global/global_player.lua';target=safe(dest,rel);text=target.read_text(encoding='utf-8')if target.exists()else''
 for r in roots:
  require(not safe(r,'global/global_player.pl').exists()or(r==dest and target.exists()),'Lua global player hook would shadow a Perl callback')
  if r!=dest:require(not safe(r,rel).exists(),'Additional quest root has a competing global player callback')
  for zone in('moors','stonehive'):
   for f in list((r/zone).glob('player*.pl'))+list((r/zone).glob('v*/player.*'))+list((r/zone).glob('player_v*.lua')):
    if f.is_file():raise Stop('Zone/versioned player callback needs explicit composition: '+str(f))
   player=safe(r,zone+'/player.lua')
   if player.exists():
    allowed=baseline.get('accepted_player_callbacks',{}).get(zone+'/player.lua',[])
    require(r==dest and sha(player.read_bytes())in allowed,'Zone player callback needs explicit compatibility review: '+str(player))
 # Journal item scripts run before the new global client callback. Refuse
 # competing handlers/defaults rather than silently starting two quest routes.
 for r in roots:
  folders=['global/items','moors/items','stonehive/items']
  for zone in('moors','stonehive'):
   folders += [zone+'/'+v.name+'/items'for v in(r/zone).glob('v*')if v.is_dir()and v.name[1:].isdigit()]
  for folder in folders:
   for stem in('script_30019','default'):
    for ext in('.lua','.pl'):
     alias=safe(r,folder+'/'+stem+ext)
     require(not alias.exists(),'Journal item callback needs explicit composition: '+str(alias))
 for hook in load(ROOT/'package.json')['hooks']:
  fragment=safe(ROOT,hook).read_text(encoding='utf-8').rstrip();begin='-- BEGIN ROF2 STING '+hook;end='-- END ROF2 STING '+hook
  require((begin in text)==(end in text),'Incomplete managed global hook marker')
  if begin in text:
   segment=text.split(begin,1)[1].split(end,1)[0].strip();require(segment==fragment,'Custom managed global hook needs merge: '+hook)
  else:text=text.rstrip()+'\n\n'+begin+'\n'+fragment+'\n'+end+'\n'
 if not target.exists()or target.read_text(encoding='utf-8')!=text:changes[rel]=text.encode()
 return changes

def game_writers(server):
 """Match this installation only, including bind-mounted Linux cwd aliases."""
 names={'world','zone','ucs','eqlaunch','loginserver','queryserv'};found=[]
 if os.name=='posix'and Path('/proc').is_dir():
  identity=lambda p:(p.stat().st_dev,p.stat().st_ino)
  base=identity(server)
  for proc in Path('/proc').iterdir():
   if not proc.name.isdigit():continue
   try:
    name=(proc/'comm').read_text().strip().lower().removesuffix('.exe')
    if name not in names:continue
    def under(path):
     seen=set()
     for _ in range(24):
      key=identity(path)
      if key==base:return True
      if key in seen:return False
      seen.add(key);path=path/'..'
     return False
    cwd_matches=under(proc/'cwd')
    executable=os.readlink(proc/'exe').removesuffix(' (deleted)')
    executable_parent=proc/'root'/str(Path(executable).parent).lstrip('/')
    if cwd_matches or under(executable_parent):found.append(int(proc.name))
   except(FileNotFoundError,ProcessLookupError):continue
   except PermissionError:raise Stop('Cannot verify the location of a running server process; use an account able to inspect the stopped installation')
 elif os.name=='nt':
  import ctypes as c
  from ctypes import wintypes as w
  class PE(c.Structure):_fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',c.c_size_t),('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('exe',w.WCHAR*260)]
  k=c.WinDLL('kernel32',use_last_error=True);k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE;k.Process32FirstW.argtypes=[w.HANDLE,c.POINTER(PE)];k.Process32FirstW.restype=w.BOOL;k.Process32NextW.argtypes=[w.HANDLE,c.POINTER(PE)];k.Process32NextW.restype=w.BOOL;k.OpenProcess.restype=w.HANDLE;k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.QueryFullProcessImageNameW.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,c.POINTER(w.DWORD)];k.CloseHandle.argtypes=[w.HANDLE]
  h=k.CreateToolhelp32Snapshot(2,0);require(h!=w.HANDLE(-1).value,'Cannot enumerate native server processes');p=PE();p.size=c.sizeof(p);ok=k.Process32FirstW(h,c.byref(p))
  try:
   while ok:
    if p.exe.lower().removesuffix('.exe')in names:
     ph=k.OpenProcess(0x1000,False,p.pid);require(ph,'Cannot verify a running server process; stop it before installing')
     try:
      n=w.DWORD(32768);s=c.create_unicode_buffer(n.value);require(k.QueryFullProcessImageNameW(ph,0,s,c.byref(n)),'Cannot locate a running server executable')
      exe=Path(s.value).resolve();base=server.resolve()
      if base in exe.parents:found.append(p.pid)
     finally:k.CloseHandle(ph)
    ok=k.Process32NextW(h,c.byref(p))
  finally:k.CloseHandle(h)
 else:raise Stop('Cannot verify stopped server processes on this platform')
 return found

def atomic_write(path,data):
 path.parent.mkdir(parents=True,exist_ok=True)
 fd,temp=tempfile.mkstemp(prefix='.'+path.name+'.',dir=str(path.parent))
 try:
  with os.fdopen(fd,'wb')as f:f.write(data);f.flush();os.fsync(f.fileno())
  old=path.stat()if path.exists()else None
  os.chmod(temp,stat.S_IMODE(old.st_mode)if old else 0o644)
  if old and hasattr(os,'chown')and hasattr(os,'geteuid')and os.geteuid()==0:os.chown(temp,old.st_uid,old.st_gid)
  os.replace(temp,path)
 finally:
  if os.path.exists(temp):os.unlink(temp)
def sql_batch(review_hash):
 pieces=[]
 for rel in load(ROOT/'package.json')['sql']:
  s=safe(ROOT,rel).read_text(encoding='utf-8')
  require(len(re.findall(r'^START TRANSACTION;\s*$',s,re.M))==1 and len(re.findall(r'^COMMIT;\s*$',s,re.M))==1,'SQL file transaction shape changed: '+rel)
  s=re.sub(r'^(START TRANSACTION|COMMIT);\s*$', '',s,flags=re.M);pieces.append(s)
 sql="SET @sting_package_review_sha="+quote(review_hash)+";\nSTART TRANSACTION;\nCREATE TEMPORARY TABLE _sting_lock_guard(ok TINYINT NOT NULL,CHECK(ok=1))ENGINE=InnoDB;\nINSERT INTO _sting_lock_guard VALUES(GET_LOCK(CONCAT('rof2-sting:',DATABASE()),0));\n"+'\n'.join(pieces)+'\nDROP TEMPORARY TABLE _sting_lock_guard;\nCOMMIT;\n'
 validate_sql(sql)
 return sql

def same_snapshot(a,b):return set(a)==set(b)and all(normalize(a[t])==normalize(b[t])for t in a)
def sql_statements(text):
 """Split packaged SQL without treating semicolons in literals as statements."""
 out=[];buf=[];quote_char=None;i=0
 while i<len(text):
  ch=text[i];nxt=text[i:i+2]
  if quote_char:
   buf.append(ch)
   if ch=='\\'and i+1<len(text):i+=1;buf.append(text[i])
   elif ch==quote_char:
    if i+1<len(text)and text[i+1]==quote_char:i+=1;buf.append(text[i])
    else:quote_char=None
  elif ch in("'",'"','`'):quote_char=ch;buf.append(ch)
  elif nxt=='/*':
   end=text.find('*/',i+2);require(end>=0,'Unterminated SQL comment');i=end+1;buf.append(' ')
  elif ch=='#'or(nxt=='--'and(i+2==len(text)or text[i+2].isspace())):
   end=text.find('\n',i);i=(len(text)-1 if end<0 else end);buf.append('\n')
  elif ch==';':
   s=''.join(buf).strip()
   if s:out.append(s)
   buf=[]
  else:buf.append(ch)
  i+=1
 require(quote_char is None,'Unterminated SQL literal');require(not ''.join(buf).strip(),'Every packaged SQL statement must end in a semicolon')
 return out
def validate_sql(sql):
 for s in sql_statements(sql):
  if re.match(r'^(SET\b|START\s+TRANSACTION\b|COMMIT\b)',s,re.I):continue
  m=re.match(r'^(?:INSERT\s+INTO|UPDATE)\s+`?([a-z_]+)`?\b',s,re.I)
  if m:
   t=m.group(1).lower();require(t in ALLOWED or t.startswith('_bixie_')or t.startswith('_sting_'),'SQL attempts to write a non-content table: '+t);continue
  m=re.match(r'^(?:CREATE|DROP)\s+TEMPORARY\s+TABLE\s+(?:IF\s+EXISTS\s+)?`?([a-z_]+)',s,re.I)
  require(m and(m.group(1).startswith('_bixie_')or m.group(1).startswith('_sting_')),'Unexpected packaged SQL statement: '+s[:80])
