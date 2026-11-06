#pragma once
#include "zonedump.h"
#include <cstring>
// SOURCE ONLY. Exact pinned loader/struct ABI; compare BEFORE NPC construction.
// Local baseline only: never authorizes a spawn, acceptance, custody or serving activation.
// Whole-object equality is intentional: loader and expected are memset-zeroed, including padding.
// A different loader/ABI requires source requalification, not silent field omission.
namespace FireHelperLocalSourceProfile {
inline bool MatchesExpectedLoadedNPCType(const NPCType& actual) {
 NPCType expected; std::memset(&expected,0,sizeof(expected));
 switch(actual.npc_id) {
 case 490913:
  std::memcpy(expected.name,"Destructive_Golem",17);
  std::memcpy(expected.lastname,"",0);
  expected.current_hp = 300000;
  expected.max_hp = 300000;
  expected.size = 0;
  expected.runspeed = 1.25;
  expected.gender = 0;
  expected.race = 649;
  expected.class_ = 1;
  expected.bodytype = 1;
  expected.deity = 0;
  expected.level = 85;
  expected.npc_id = 490913;
  expected.texture = 0;
  expected.helmtexture = 0;
  expected.herosforgemodel = 0;
  expected.loottable_id = 0;
  expected.npc_spells_id = 0;
  expected.npc_spells_effects_id = 0;
  expected.npc_faction_id = 0;
  expected.faction_amount = 0;
  expected.merchanttype = 0;
  expected.alt_currency_type = 0;
  expected.adventure_template = 0;
  expected.trap_template = 0;
  expected.light = 0;
  expected.AC = 0;
  expected.Mana = 0;
  expected.ATK = 0;
  expected.STR = 75;
  expected.STA = 75;
  expected.DEX = 75;
  expected.AGI = 75;
  expected.INT = 80;
  expected.WIS = 75;
  expected.CHA = 75;
  expected.MR = 0;
  expected.FR = 0;
  expected.CR = 0;
  expected.PR = 0;
  expected.DR = 0;
  expected.Corrup = 0;
  expected.PhR = 0;
  expected.haircolor = 1;
  expected.beardcolor = 1;
  expected.eyecolor1 = 1;
  expected.eyecolor2 = 1;
  expected.hairstyle = 1;
  expected.luclinface = 1;
  expected.beard = 0;
  expected.drakkin_heritage = 0;
  expected.drakkin_tattoo = 0;
  expected.drakkin_details = 0;
  expected.min_dmg = 0;
  expected.max_dmg = 0;
  expected.charm_ac = 0;
  expected.charm_min_dmg = 0;
  expected.charm_max_dmg = 0;
  expected.charm_attack_delay = 0;
  expected.charm_accuracy_rating = 0;
  expected.charm_avoidance_rating = 0;
  expected.charm_atk = 0;
  expected.attack_count = -1;
  std::memcpy(expected.special_abilities,"",0);
  expected.d_melee_texture1 = 0;
  expected.d_melee_texture2 = 0;
  std::memcpy(expected.ammo_idfile,"IT10",4);
  expected.prim_melee_type = 28;
  expected.sec_melee_type = 28;
  expected.ranged_type = 7;
  expected.hp_regen = 0;
  expected.hp_regen_per_second = 0;
  expected.mana_regen = 0;
  expected.aggroradius = 70;
  expected.assistradius = 70;
  expected.see_invis = 0;
  expected.see_invis_undead = false;
  expected.see_hide = false;
  expected.see_improved_hide = false;
  expected.qglobal = false;
  expected.npc_aggro = false;
  expected.spawn_limit = 0;
  expected.mount_color = 0;
  expected.attack_speed = 0;
  expected.attack_delay = 3000;
  expected.accuracy_rating = 0;
  expected.avoidance_rating = 0;
  expected.findable = false;
  expected.trackable = true;
  expected.is_quest_npc = true;
  expected.slow_mitigation = 0;
  expected.maxlevel = 0;
  expected.scalerate = 100;
  expected.private_corpse = false;
  expected.unique_spawn_by_name = false;
  expected.underwater = false;
  expected.emoteid = 0;
  expected.spellscale = 100;
  expected.healscale = 100;
  expected.no_target_hotkey = false;
  expected.raid_target = false;
  expected.armtexture = 0;
  expected.bracertexture = 0;
  expected.handtexture = 0;
  expected.legtexture = 0;
  expected.feettexture = 0;
  expected.ignore_despawn = false;
  expected.show_name = true;
  expected.untargetable = false;
  expected.skip_global_loot = true;
  expected.rare_spawn = false;
  expected.skip_auto_scale = false;
  expected.stuck_behavior = 0;
  expected.use_model = 0;
  expected.flymode = -1;
  expected.always_aggro = false;
  expected.exp_mod = 0;
  expected.heroic_strikethrough = 0;
  expected.keeps_sold_items = true;
  expected.is_parcel_merchant = false;
  expected.greed = 0;
  expected.multiquest_enabled = false;
  expected.m_npc_tint_id = 0;
  break;
 default: return false;
 }
 return std::memcmp(&actual,&expected,sizeof(NPCType))==0;
}
} // namespace FireHelperLocalSourceProfile
