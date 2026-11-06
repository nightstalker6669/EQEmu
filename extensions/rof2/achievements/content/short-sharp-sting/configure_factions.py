#!/usr/bin/env python3
"""Configure the labelled faction defaults before installation; no database writes."""
from pathlib import Path
import argparse,json,re
import package_tools as T
P=Path(__file__).resolve().parent

def generate():
 policy=T.load(P/'faction-policy.json');routes=T.load(P/'faction-routes.json')
 T.require(policy['gate_faction_id']==1124,'The reviewed gate uses Legion of Stonehive')
 T.require(type(policy['maximum_raw_faction'])is int and -2000<=policy['maximum_raw_faction']<-750,'Gate must be a raw scowling value in [-2000,-751]')
 hits=policy['kill_hits'];T.require(set(hits)=={'1123','1124','1129'},'Expected Drones, Legion and Crystalwing')
 T.require(all(type(v)is int and -100<=v<=100 and v!=0 for v in hits.values()),'Kill amounts must be nonzero integers within [-100,100]')
 T.require(hits['1123']<0 and hits['1124']<0 and hits['1129']>0,'Kill-faction directions must match the sources')
 group=routes['group_id'];actors=routes['actors'];ids=[a['id']for a in actors]
 T.require(group==396942 and len(ids)==len(set(ids))and routes['unmodified_original']not in ids,'Invalid reserved faction route')
 groups=[{'id':group,'name':'May2013 Stonehive labelled kill defaults','primaryfaction':0,'ignore_primary_assist':0}]
 entries=[{'npc_faction_id':group,'faction_id':int(k),'value':v,'npc_value':0,'temp':0}for k,v in sorted(hits.items())]
 patches=[{'id':a['id'],'npc_faction_id':group}for a in actors]
 old=[{'id':a['id'],'npc_faction_id':a['previous_group']}for a in actors]
 contract={'npc_faction':{'scope':'id='+str(group),'rows':groups},'npc_faction_entries':{'scope':'npc_faction_id='+str(group),'rows':entries},'npc_types':{'scope':'id IN('+','.join(map(str,ids))+')','rows':patches,'accepted_existing':old}}
 sql=['-- User-approved labelled faction defaults; original numerical values are unverified.', '-- Source actors: only npc_faction_id changes; primaryfaction remains zero.', 'SET NAMES utf8mb4;','SET SESSION check_constraint_checks=1;','START TRANSACTION;', 'CREATE TEMPORARY TABLE _sting_faction_guard(label VARCHAR(80)PRIMARY KEY,ok TINYINT NOT NULL,CHECK(ok=1))ENGINE=InnoDB;']
 def exact(r):return ' AND '.join('`'+k+'` <=> '+T.quote(v)for k,v in r.items())
 def guard(label,condition):sql.append('INSERT INTO _sting_faction_guard VALUES('+T.quote(label)+',('+condition+'));')
 guard('client_factions',"(SELECT COUNT(*) FROM faction_list WHERE base=0 AND((id=1123 AND name='Drones of Stonehive')OR(id=1124 AND name='Legion of Stonehive')OR(id=1129 AND name='Circle of the Crystalwing')))=3")
 guard('reachable_gate','EXISTS(SELECT 1 FROM faction_base_data WHERE client_faction_id=1124 AND min<= '+str(policy['maximum_raw_faction'])+' AND max=0)')
 for table,rows,pk in [('npc_faction',groups,['id']),('npc_faction_entries',entries,['npc_faction_id','faction_id'])]:
  scope=contract[table]['scope'];guard(table,'NOT EXISTS(SELECT 1 FROM '+table+' WHERE ('+scope+') AND NOT('+' OR '.join('('+exact(r)+')'for r in rows)+'))')
  keys=list(rows[0]);sql.append('INSERT INTO '+table+'('+','.join('`'+k+'`'for k in keys)+')VALUES '+','.join('('+','.join(T.quote(r[k])for k in keys)+')'for r in rows)+' ON DUPLICATE KEY UPDATE '+','.join('`'+k+'`=VALUES(`'+k+'`)'for k in keys if k not in pk)+';')
 for a in actors:
  guard('actor_'+str(a['id']),'EXISTS(SELECT 1 FROM npc_types WHERE id='+str(a['id'])+' AND name='+T.quote(a['name'])+' AND npc_faction_id IN('+str(a['previous_group'])+','+str(group)+'))')
 # UPDATE, rather than full-row UPSERT, preserves every source combat field.
 sql.append('UPDATE npc_types SET npc_faction_id='+str(group)+' WHERE id IN('+','.join(map(str,ids))+');')
 sql+=['DROP TEMPORARY TABLE _sting_faction_guard;','COMMIT;']
 (P/'faction-row-contract.json').write_text(json.dumps(contract,indent=2)+'\n');(P/'sql/010-factions.sql').write_text('\n'.join(sql)+'\n')
 # The copy template is private to this package; original Prankster is untouched.
 c=T.load(P/'avellone-row-contract.json');clone=c['npc_types']['rows'][0];old_group=clone['npc_faction_id']
 # Generate through the source builder initially; subsequent reconfiguration
 # only changes hit values/threshold and never changes this fixed group ID.
 T.require(old_group==group,'Copy template must use the reserved kill group; rebuild the source package')
 review=T.load(P/'review.json');review['buzz']['faction_id']=policy['gate_faction_id'];review['buzz']['maximum_raw_faction']=policy['maximum_raw_faction']
 review['faction_policy']={'kind':'labelled configurable emulator defaults','file':'faction-policy.json','kill_hits':hits,'original_values_verified':False}
 (P/'review.json').write_text(json.dumps(review,indent=2)+'\n');(P/'quests/lua_modules/achievement_quest_sting_settings.lua').write_text(T.settings_text(review))
 activation=P/'sql/090-activate.sql'
 if activation.exists():
  digest=T.sha(json.dumps(review,sort_keys=True,separators=(',',':')).encode());activation.write_text(re.sub("@sting_package_review_sha='[a-f0-9]{64}'","@sting_package_review_sha='"+digest+"'",activation.read_text()))
 return policy

def refresh_manifest():
 files=[{'path':p.relative_to(P).as_posix(),'sha256':T.sha(p.read_bytes())}for p in sorted(P.rglob('*'))if p.is_file()and p.name!='payload-manifest.json'and '__pycache__'not in p.parts]
 (P/'payload-manifest.json').write_text(json.dumps(files,indent=2)+'\n')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--maximum-raw-faction',type=int);p.add_argument('--drones-hit',type=int);p.add_argument('--legion-hit',type=int);p.add_argument('--crystalwing-hit',type=int);a=p.parse_args();T.integrity()
 policy=T.load(P/'faction-policy.json')
 if a.maximum_raw_faction is not None:policy['maximum_raw_faction']=a.maximum_raw_faction
 for arg,key in [('drones_hit','1123'),('legion_hit','1124'),('crystalwing_hit','1129')]:
  value=getattr(a,arg)
  if value is not None:policy['kill_hits'][key]=value
 policy['baseline_from_zero_kills']=(-policy['maximum_raw_faction']+abs(policy['kill_hits']['1124'])-1)//abs(policy['kill_hits']['1124'])if policy['kill_hits']['1124']else None
 # Validate policy before changing any generated files.
 T.require(-2000<=policy['maximum_raw_faction']<-750 and all(type(v)is int and 0<abs(v)<=100 for v in policy['kill_hits'].values())and policy['kill_hits']['1123']<0 and policy['kill_hits']['1124']<0 and policy['kill_hits']['1129']>0,'Invalid faction policy')
 (P/'faction-policy.json').write_text(json.dumps(policy,indent=2)+'\n');generate();refresh_manifest();print('Configured labelled faction policy. No database writes. Run install.py preflight before applying; custom existing amounts require a reviewed migration.')
if __name__=='__main__':
 try:main()
 except T.Stop as e:raise SystemExit('STOP: '+str(e))
