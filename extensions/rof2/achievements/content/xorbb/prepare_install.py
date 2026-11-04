"""Prepare a reviewable Xorbb upgrade stage. Never writes server files or SQL."""
from pathlib import Path
import argparse,hashlib,json,re,shutil

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def readjson(p):return json.loads(p.read_text())
def safe_target(root,rel):
 p=Path(rel)
 if p.is_absolute()or '..'in p.parts:raise RuntimeError('Unsafe relative path: '+rel)
 target=root/p
 if root not in target.resolve().parents:raise RuntimeError('Target escapes quest directory: '+rel)
 for part in [target,*target.parents]:
  if part==root.parent:break
  if part.is_symlink():raise RuntimeError('Symbolic-link target requires review: '+rel)
 if target.exists()and not target.is_file():raise RuntimeError('Non-file quest target: '+rel)
 return target

def prepare(base,quests,stage):
 base=base.resolve(strict=True);quests=quests.resolve(strict=True)
 if not quests.is_dir():raise RuntimeError('Quest directory is not a directory')
 stage=stage.resolve()
 if stage.exists():raise RuntimeError('Use a new staging directory; existing stage is preserved')
 if stage==quests or quests in stage.parents:raise RuntimeError('Stage must be outside the active quest directory')
 manifest=readjson(base/'SOURCE-MANIFEST.json')
 if manifest.get('ready_to_apply')is not False:raise RuntimeError('Unexpected source-stage contract')
 for rel,h in manifest['sha256'].items():
  p=safe_target(base,rel)
  if not p.is_file()or digest(p)!=h:raise RuntimeError('Package checksum mismatch: '+rel)
 contract=readjson(base/'INSTALL-CONTRACT.json');outputs={};origins={};conflicts=[];preimages={}
 # Both required generations are staged. Duplicate paths must have identical
 # bytes; no unknown callback/module is silently replaced.
 for source_root in contract['quest_source_roots']:
  root=base/source_root
  for p in sorted(root.rglob('*.lua')):
   rel=p.relative_to(root).as_posix();data=p.read_bytes()
   if rel in outputs and outputs[rel]!=data:raise RuntimeError('Conflicting required package sources: '+rel)
   outputs[rel]=data;origins.setdefault(rel,[]).append(p.relative_to(base).as_posix())
 for rel,fragments in contract['global_fragments'].items():
  target=safe_target(quests,rel)
  if not target.exists():raise RuntimeError('Missing predecessor global handler: '+rel)
  data=target.read_bytes();preimages[rel]=data
  for fragment in fragments:
   addition=(base/fragment).read_bytes()
   markers=re.findall(rb'^-- BEGIN[^\r\n]+',addition,re.M)
   if not markers:raise RuntimeError('Unmarked global fragment: '+fragment)
   present=[m in data for m in markers]
   if any(present):
    # An exact complete append is reusable; partial/different installations
    # need explicit composition instead of a duplicate callback chain.
    if not all(present)or addition not in data:raise RuntimeError('Partial/different global fragment: '+fragment)
   else:data=data.rstrip()+b'\n\n'+addition
  outputs[rel]=data;origins[rel]=fragments
 names=readjson(base/'NPC-CALLBACK-NAMES.json')
 for ident,name in names.items():
  numeric='xorbb/'+ident+'.lua'
  if numeric not in outputs:raise RuntimeError('Callback inventory mismatch: '+ident)
  aliases={name,name.lstrip('#'),name.replace(' ','_'),name.lstrip('#').replace(' ','_'),ident}
  for alias in aliases:
   for suffix in ['.lua','.pl']:
    rel='xorbb/'+alias+suffix
    if rel==numeric:continue
    p=safe_target(quests,rel)
    if p.exists():conflicts.append({'path':rel,'reason':'Alternate NPC callback may own the same event; explicit merge needed','sha256':digest(p)})
 for rel in ['global/global_npc.lua','global/global_npc.pl','xorbb/global_npc.lua','xorbb/global_npc.pl']:
  p=safe_target(quests,rel)
  if p.exists()and re.search(r'\b(?:event_trade|EVENT_ITEM)\b',p.read_text(errors='replace')):
   conflicts.append({'path':rel,'reason':'Global physical hand-in owner requires custody composition review','sha256':digest(p)})
 changes=[]
 for rel,data in outputs.items():
  target=safe_target(quests,rel)
  if rel not in preimages:preimages[rel]=target.read_bytes()if target.exists()else None
  before=hashlib.sha256(preimages[rel]).hexdigest()if preimages[rel]is not None else None;after=hashlib.sha256(data).hexdigest()
  if rel not in contract['global_fragments']and before not in [None,after]:
   conflicts.append({'path':rel,'reason':'Existing module/NPC handler differs; do not replace without composition','sha256':before})
  changes.append({'path':rel,'before_sha256':before,'after_sha256':after,'action':'unchanged'if before==after else('create'if before is None else'merge'if rel in contract['global_fragments']else'CONFLICT'),'sources':origins[rel]})
 stage.mkdir(parents=True)
 try:
  for rel,data in outputs.items():
   dest=stage/'quests'/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
   original=safe_target(quests,rel)
   if preimages[rel]is not None:
    dest=stage/'preimages/quests'/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(preimages[rel])
  sql=base/contract['canonical_sql'];dest=stage/'sql'/sql.name;dest.parent.mkdir();shutil.copy2(sql,dest)
  for change in changes:
   target=safe_target(quests,change['path']);current=digest(target)if target.exists()else None
   if current!=change['before_sha256']:
    conflicts.append({'path':change['path'],'reason':'Predecessor changed during staging; recompose from the new version','sha256':current})
  result={'status':'REVIEW_STAGE_ONLY','active_quest_files_changed':0,'database_actions':0,'activation':False,'ready_to_apply':False,'target_quests':str(quests),'source_manifest_sha256':digest(base/'SOURCE-MANIFEST.json'),'canonical_sql_sha256':digest(sql),'changes':sorted(changes,key=lambda x:x['path']),'conflicts':conflicts,'remaining':'Coordinator must finish migration review/connection proof/backup+controlled apply/activation and native acceptance; this tool has no apply mode.'}
  (stage/'INSTALL-PLAN.private.json').write_text(json.dumps(result,indent=2)+'\n')
  (stage/'REVIEW.txt').write_text('Review stage only. No server files or databases changed.\nDo not copy this directory blindly onto a server.\nConflicts: '+str(len(conflicts))+'\nSource SQL remains disabled and requires the complete coordinated application contract.\n')
  return result
 except Exception:
  # Preserve a partially written review stage as evidence, never auto-retry.
  (stage/'STAGING-FAILED.txt').write_text('Staging interrupted. Active server files and SQL were not changed. Preserve/review this directory before retrying.\n')
  raise

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--quests-dir',type=Path,required=True);ap.add_argument('--stage-dir',type=Path,required=True);args=ap.parse_args()
 r=prepare(Path(__file__).resolve().parent,args.quests_dir,args.stage_dir)
 print(json.dumps({'status':r['status'],'staged_quest_files':len(r['changes']),'conflicts':len(r['conflicts']),'active_files_changed':0,'database_actions':0,'ready_to_apply':False}))
if __name__=='__main__':main()
