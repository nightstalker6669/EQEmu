#!/usr/bin/env python3
"""Check/install Roost modules and compose callbacks; refuse unknown conflicts."""
import argparse,hashlib,json,pathlib,shutil,sys
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quests',type=pathlib.Path,required=True);p.add_argument('--additional-quests',type=pathlib.Path,action='append',default=[]);p.add_argument('--apply',action='store_true');p.add_argument('--backup',type=pathlib.Path);a=p.parse_args();dest=a.quests.resolve()
def sha(data):return hashlib.sha256(data).hexdigest()
def fail(s):raise SystemExit('STOP: '+s)
for row in json.loads((ROOT/'payload-manifest.json').read_text()):
 f=ROOT/row['path']
 if not f.is_file()or sha(f.read_bytes())!=row['sha256']:fail('Release integrity mismatch: '+row['path'])
roots=[dest]+[x.resolve()for x in a.additional_quests]
if any(not x.is_dir()for x in roots):fail('Every quest root must exist')
def safe(root,rel):
 rel=pathlib.PurePosixPath(rel)
 if rel.is_absolute()or'..'in rel.parts:fail('Unsafe relative path')
 cur=root
 for part in rel.parts:
  cur=cur/part
  if cur.is_symlink():fail('Symlink in managed/conflicting path: '+str(cur))
 return cur
req=json.loads((ROOT/'provenance/baseline-identities.json').read_text());changes={};records=json.loads((ROOT/'quest-manifest.json').read_text())
for row in records:
 rel=row['path'];source=(ROOT/'quests'/rel).read_bytes();target=safe(dest,rel)
 if target.exists()and target.read_bytes()!=source and sha(target.read_bytes())not in req['accepted_previous_scripts'].get(rel,[]):fail('Custom managed script requires merge: '+rel)
 if not target.exists()or target.read_bytes()!=source:changes[rel]=source
 if rel.startswith('lua_modules/'):
  if any(safe(root,rel).exists()for root in roots[1:]):fail('Module in additional root requires merge: '+rel)
  continue
 zone,filename=rel.split('/');numeric=pathlib.PurePosixPath(filename).stem;name=req['npc_types'][numeric].replace('`','-')
 for root in roots:
  folders=[zone,'global']+[zone+'/'+v.name for v in(root/zone).glob('v*')if v.is_dir()and v.name[1:].isdigit()]
  for folder in folders:
   for stem in(numeric,name,name.lstrip('#'),name+'_'+numeric,name.lstrip('#')+'_'+numeric):
    for ext in('.lua','.pl'):
     other=folder+'/'+stem+ext
     if root==dest and other==rel:continue
     if safe(root,other).exists():fail('Alternate NPC callback requires merge: '+str(root/other))
for name in req['external_modules']:
 if not any(safe(root,'lua_modules/'+name+'.lua').is_file()for root in roots):fail('Required earlier module missing: '+name)
rel='global/global_player.lua';target=safe(dest,rel);text=target.read_text()if target.exists()else'';fragment=(ROOT/'hooks/global-player-roost.fragment.lua').read_text();marker='-- BEGIN ROF2 ROOST global-player'
legacy_fragment=fragment.replace('-- Roost achievement routes: preserve existing task callbacks.','-- Roost achievement routes: preserve housing and other task callbacks.')
installed=fragment.strip()in text or legacy_fragment.strip()in text
if marker in text and not installed:fail('Modified Roost player hook requires merge')
if 'local roost_previous_'in text and not installed:fail('Unrecognized prior Roost hook requires merge')
for root in roots:
 if safe(root,'global/global_player.pl').exists()and(root!=dest or not target.exists()):fail('Lua hook would shadow Perl global player')
 if root!=dest and safe(root,rel).exists():fail('Additional global-player callback requires merge')
if not installed:changes[rel]=(text.rstrip()+'\n\n'+marker+'\n'+fragment+'-- END ROF2 ROOST global-player\n').encode()
print('PASS: %d quest files need installation/composition.'%len(changes))
if not a.apply:sys.exit(0)
if not a.backup:p.error('--apply requires --backup outside quest tree')
backup=a.backup.resolve()
if backup==dest or dest in backup.parents:fail('Backup must be outside quest tree')
backup.mkdir(parents=True,exist_ok=False);receipt=[]
for rel,data in changes.items():
 f=safe(dest,rel);before=f.read_bytes()if f.exists()else None
 receipt.append({'path':rel,'before_sha256':sha(before)if before is not None else None,'after_sha256':sha(data)})
 if before is not None:
  old=backup/rel;old.parent.mkdir(parents=True,exist_ok=True);old.write_bytes(before)
(backup/'changes.json').write_text(json.dumps(receipt,indent=2)+'\n')
try:
 for rel,data in changes.items():
  f=safe(dest,rel);f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(data)
except Exception:
 for row in reversed(receipt):
  f=safe(dest,row['path']);old=backup/row['path']
  if old.exists():f.write_bytes(old.read_bytes())
  elif f.exists():f.unlink()
 raise
print('Installed Roost scripts and composed global callback; no services controlled.')
