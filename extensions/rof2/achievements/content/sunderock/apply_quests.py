#!/usr/bin/env python3
"""Compose Sunderock callbacks without replacing existing quest logic. Check by default."""
import argparse,hashlib,json,pathlib,sys,re
ROOT=pathlib.Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quests',required=True,type=pathlib.Path);p.add_argument('--additional-quests',action='append',type=pathlib.Path,default=[]);p.add_argument('--apply',action='store_true');p.add_argument('--backup',type=pathlib.Path);a=p.parse_args()
def fail(s):raise SystemExit('STOP: '+s)
def sha(data):return hashlib.sha256(data).hexdigest()
def safe(root,rel):
 rel=pathlib.PurePosixPath(rel)
 if rel.is_absolute()or'..'in rel.parts:fail('Unsafe payload path')
 cur=root
 for part in rel.parts:
  cur=cur/part
  if cur.is_symlink():fail('Symlink in managed/conflicting path: '+str(cur))
 return cur
for row in json.loads((ROOT/'payload-manifest.json').read_text()):
 f=safe(ROOT,row['path'])
 if not f.is_file()or sha(f.read_bytes())!=row['sha256']:fail('Payload integrity mismatch: '+row['path'])
roots=[a.quests.absolute()]+[x.absolute()for x in a.additional_quests]
if any(x.is_symlink()or not x.is_dir()for x in roots):fail('Quest roots must be real directories')
roots=[x.resolve()for x in roots];dest=roots[0];changes={}
req=json.loads((ROOT/'provenance/baseline-identities.json').read_text())
module_predecessors=json.loads((ROOT/'provenance/previous-module-sha256.json').read_text())
for row in json.loads((ROOT/'quest-manifest.json').read_text()):
 rel=row['path'];source=safe(ROOT,row['source']).read_bytes();target=safe(dest,rel);before=target.read_bytes()if target.exists()else b''
 if rel.startswith('lua_modules/'):
  if any(safe(root,rel).exists()for root in roots[1:]):fail('Additional-root module requires merge: '+rel)
  if before and before!=source and sha(before)not in module_predecessors.get(rel,[]):fail('Custom Sunderock module requires merge: '+rel)
  after=source
 else:
  zone,filename=rel.split('/');stem=pathlib.PurePosixPath(filename).stem
  stems={stem}
  if stem.isdigit():
   name=req['npc_types'][stem].replace('`','-')
   stems.update((name,name.lstrip('#'),name+'_'+stem,name.lstrip('#')+'_'+stem))
  for root in roots:
   folders={zone,'global'}
   folders.update(zone+'/'+v.name for v in(root/zone).glob('v*')if v.is_dir()and v.name[1:].isdigit())
   for folder in folders:
    for name in stems:
     for ext in('.lua','.pl'):
      other=folder+'/'+name+ext
      if root==dest and other==rel:continue
      # Existing Lua owns dispatch when a same-stem Perl file already coexists.
      if root==dest and folder==zone and name==stem and ext=='.pl'and target.exists():continue
      if safe(root,other).exists():fail('Alternate callback requires merge: '+str(root/other))
  fragment=source.decode().strip();text=before.decode();marker='-- BEGIN ROF2 SUNDEROCK '+rel
  if fragment not in text and stem.isdigit() and re.search(r'(?:function\s+event_trade\s*\(|event_trade\s*=)',text):fail('Existing hand-in owner requires an explicit custody merge: '+rel)
  if fragment in text:after=before
  else:
   if marker in text or 'local sunderock_previous_'in text:fail('Modified/partial Sunderock hook requires merge: '+rel)
   after=(text.rstrip()+'\n\n'+marker+'\n'+fragment+'\n-- END ROF2 SUNDEROCK '+rel+'\n').encode()
 if after!=before:changes[rel]=(before if target.exists()else None,after)
for name in req['external_modules']:
 if not any(safe(root,'lua_modules/'+name+'.lua').is_file()for root in roots):fail('Earlier required module missing: '+name)
print('PASS: %d quest files need installation/composition.'%len(changes))
if not a.apply:sys.exit(0)
if not a.backup:p.error('--apply requires --backup')
backup=a.backup.absolute()
if backup.is_symlink():fail('Backup cannot be a symlink')
backup=backup.resolve()
if any(backup==root or root in backup.parents for root in roots):fail('Backup must be outside every quest root')
backup.mkdir(parents=True,exist_ok=False);receipt=[]
for rel,(before,after)in changes.items():
 receipt.append({'path':rel,'before_sha256':sha(before)if before is not None else None,'after_sha256':sha(after)})
 if before is not None:
  f=backup/rel;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(before)
(backup/'changes.json').write_text(json.dumps(receipt,indent=2)+'\n')
try:
 for rel,(_,after)in changes.items():
  f=safe(dest,rel);f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(after)
except Exception:
 for rel,(before,_)in reversed(list(changes.items())):
  f=safe(dest,rel)
  if before is None:f.unlink(missing_ok=True)
  else:f.write_bytes(before)
 raise
print('Installed Sunderock modules and composed callbacks. No services controlled.')
