#include "recharge_recipe8.h"
#include "final_recipe302.h"
#include "housing_recipe301.h"
#include "trophy_certificate55.h"
/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "client.h"
#include "charged_recipe32.h"
#include "final_recipe300.h"
#include "doors.h"
#include "tanaan_forge28_actor.h"
#include "world_loom13_actor.h"
#include "gnomish_tailoring35_actor.h"
#include "cultural_armor_bulk_actor.h"
#include "research_tablet335_actor.h"
#include "cultural_tailoring_bulk_actor.h"

#include "common/events/player_event_logs.h"
#include "common/repositories/char_recipe_list_repository.h"
#include "common/repositories/criteria/content_filter_criteria.h"
#include "common/repositories/tradeskill_recipe_entries_repository.h"
#include "common/repositories/tradeskill_recipe_repository.h"
#include "common/rulesys.h"
#include "zone/queryserv.h"
#include "zone/quest_parser_collection.h"
#include "zone/string_ids.h"
#include "zone/titles.h"
#include "zone/worldserver.h"
#include "zone/zonedb.h"

#include <algorithm>
#include <list>
#include <array>
#include <vector>
#include <stdexcept>
#include "inventory_session_owner.h"

extern QueryServ* QServ;
extern WorldServer worldserver;


namespace {


struct Final300InputTree {
 struct Node {const EQ::ItemInstance* original;uint32_t id;int32_t serial;int16_t charges;std::vector<std::pair<uint8_t,Node>> children;};
 Node root;
 static Node Capture(const EQ::ItemInstance& item) {
  Node n{&item,item.GetID(),item.GetSerialNumber(),item.GetCharges(),{}};
  for(const auto& child:*item.GetContents()) {
   if(!child.second)throw std::runtime_error("Missing finite recipe ingredient");
   n.children.emplace_back(child.first,Capture(*child.second));
  }
  return n;
 }
 static bool Same(const Node& n,const EQ::ItemInstance* item) {
  if(!item || item!=n.original || item->GetID()!=n.id || item->GetSerialNumber()!=n.serial ||
   item->GetCharges()!=n.charges || item->GetContents()->size()!=n.children.size())return false;
  for(const auto& child:n.children) {
   auto at=item->GetContents()->find(child.first);
   if(at==item->GetContents()->end() || !Same(child.second,at->second))return false;
  }
  return true;
 }
};


bool Final302Output(const DBTradeskillRecipe_Struct& spec,const FinalRecipe302::Requirement& r) {
 return uint32_t(spec.tradeskill)==r.skill&&!spec.quest&&!spec.replace_container&&spec.onsuccess.size()==1&&spec.onsuccess.begin()->first==r.output&&spec.onsuccess.begin()->second==r.yield;
}
bool Final302Kit(Client& user,const FinalRecipe302::Requirement& r,int16_t slot,const EQ::ItemInstance* expected) {
 if(!EQ::ValueWithin(slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END))return false;
 const auto* kit=user.GetInv().GetItem(slot);const auto* data=kit&&kit->IsClassBag()?kit->GetItem():nullptr;
 return kit==expected&&data&&data->ID==r.kit&&data->BagType==r.type&&data->BagSlots==r.slots&&data->BagSize==r.size;
}

bool Final300OutputValid(const DBTradeskillRecipe_Struct& spec) {
 if (!FinalRecipe300::Selected(spec.recipe_id)) return true;
 if (uint32_t(spec.tradeskill)!=FinalRecipe300::Skill(spec.recipe_id) || spec.quest) return false;
 if (spec.recipe_id == 3102882) {
  const auto* item=database.GetItem(23382);
  if(!item || item->ItemClass!=0 || item->ItemType!=8 || item->MaxCharges!=0 ||
   spec.nofail || spec.must_learn || spec.replace_container || spec.skill_needed || spec.trivial!=115)return false;
  const auto has=[](const auto& items,uint32_t id,uint32_t count) {
   for(const auto& p:items)if(p.first==id)return p.second==count;
   return false;
  };
  return spec.onsuccess.size()==2 && has(spec.onsuccess,23382,1) && has(spec.onsuccess,10489,1) &&
   spec.onfail.size()==1 && has(spec.onfail,10489,1);
 }
 if (spec.recipe_id == 3102867 || spec.recipe_id == 3102868) {
  if (spec.nofail || spec.must_learn || spec.replace_container) return false;
  const auto has=[](const auto& items,uint32_t id,uint32_t count) {
   for(const auto& p:items) if(p.first==id) return p.second==count;
   return false;
  };
  if(spec.recipe_id==3102867) return spec.trivial==43 &&
   spec.onsuccess.size()==1 && has(spec.onsuccess,3444,1) &&
   spec.onfail.size()==1 && has(spec.onfail,3474,1);
  return spec.trivial==153 && spec.onsuccess.size()==3 &&
   has(spec.onsuccess,2074,1) && has(spec.onsuccess,6027,1) && has(spec.onsuccess,7052,1) &&
   spec.onfail.size()==2 && has(spec.onfail,6027,1) && has(spec.onfail,7052,1);
 }
 if (spec.recipe_id!=3102766) return true;
 const auto* item=database.GetItem(75815);
 if (!item || item->ItemClass!=0 || item->Stackable || item->MaxCharges!=8) return false;
 unsigned found=0;
 for (const auto& pair:spec.onsuccess) if(pair.first==75815) { if(pair.second!=1)return false; ++found; }
 return found==1;
}
bool Final300Font(Client& user,uint32_t recipe) {
 if(zone->GetZoneID()!=490 || zone->GetInstanceVersion()!=0)return false;
 const auto p=FinalRecipe300::Position(recipe);
 auto* door=entity_list.FindDoor(p.door);
 return door && door->GetDoorID()==p.door && FinalRecipe300::InsideFont(p,door->GetDoorName(),
  door->GetX(),door->GetY(),door->GetZ(),user.GetX(),user.GetY(),user.GetZ());
}
bool Final300Kit(Client& user,int16_t slot,const EQ::ItemInstance* expected) {
 if(!EQ::ValueWithin(slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END))return false;
 const auto* kit=user.GetInv().GetItem(slot);
 const auto* data=kit&&kit->IsType(EQ::item::ItemClassBag)?kit->GetItem():nullptr;
 return kit==expected && data && data->ID==17187 && data->BagType==20 && data->BagSlots==6 && data->BagSize==3;
}

struct ChargedPacking32Plan {
 struct Take {int16_t slot=INVALID_INDEX;const EQ::ItemInstance* original=nullptr;int32_t serial=0;};
 int16_t kit_slot=INVALID_INDEX;const EQ::ItemInstance* kit=nullptr;int32_t kit_serial=0;
 std::array<Take,10> takes{};size_t count=0;
};
bool Charged32OutputValid(const DBTradeskillRecipe_Struct& spec) {
 const auto* r=ChargedRecipe32::Find(spec.recipe_id);if(!r)return true;
 if(uint32_t(spec.tradeskill)!=r->skill||spec.quest)return false;
 const auto* output=database.GetItem(r->output);
 if(!output||output->ItemClass!=0||output->Stackable||output->MaxCharges!=int(r->charges))return false;
 unsigned found=0;for(const auto& pair:spec.onsuccess)if(pair.first==r->output){if(pair.second!=1)return false;++found;}
 return found==1&&(!r->units||(spec.nofail&&spec.trivial==0&&spec.skill_needed==0&&spec.replace_container));
}
bool Charged32SinglePoison(const EQ::ItemInstance* item,uint32_t id) {
 const auto* d=item?item->GetItem():nullptr;
 return d&&d->ID==id&&d->ItemClass==0&&!item->IsStackable()&&d->MaxCharges==1&&item->GetCharges()==1&&item->GetContents()->empty();
}
bool Charged32Kit(Client& user,const ChargedRecipe32::Requirement& r,int16_t slot,const EQ::ItemInstance* expected) {
 const auto* inst=user.GetInv().GetItem(slot);const auto* d=inst&&inst->IsType(EQ::item::ItemClassBag)?inst->GetItem():nullptr;
 return inst==expected&&d&&d->ID==r.kit&&d->BagType==12&&d->BagSlots==10&&d->BagSize==2;
}
bool Charged32PreparePacking(Client& user,const ChargedRecipe32::Requirement& r,int16_t slot,bool manual,ChargedPacking32Plan& plan) {
 if(!r.units||user.GetClass()!=Class::Rogue||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user.GetInv()))return false;
 auto& inv=user.GetInv();const auto* kit=inv.GetItem(slot);
 if(!Charged32Kit(user,r,slot,kit))return false;
 plan.kit_slot=slot;plan.kit=kit;plan.kit_serial=kit->GetSerialNumber();
 const auto add=[&](int16_t at,const EQ::ItemInstance* item){
  if(!Charged32SinglePoison(item,r.input)||plan.count>=r.units)return false;
  for(size_t n=0;n<plan.count;++n)if(plan.takes[n].original==item||plan.takes[n].slot==at)return false;
  plan.takes[plan.count++]={at,item,item->GetSerialNumber()};return true;
 };
 // Bottle contents have priority, so no selected contents can be left behind
 // when the guaranteed no-fail one-use bottle is removed.
 for(const auto& child:*kit->GetContents())if(child.second){
  if(child.first>=10||!add(EQ::InventoryProfile::CalcSlotId(slot,child.first),child.second))return false;
 }
 if(manual)return plan.count==r.units;
 if(!EQ::ValueWithin(slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END))return false;
 for(int16_t root=EQ::invslot::GENERAL_BEGIN;root<=EQ::invslot::GENERAL_END&&plan.count<r.units;++root) {
  if(root==slot||!inv.GetLookup()||!((uint64_t(1)<<root)&inv.GetLookup()->PossessionsBitmask))continue;
  const auto* item=inv.GetItem(root);if(!item)continue;
  if(Charged32SinglePoison(item,r.input)){if(!add(root,item))return false;continue;}
  const auto* data=item->GetItem();if(!data||!item->IsClassBag())continue;
  for(uint8_t index=0;index<data->BagSlots&&index<EQ::invbag::SLOT_COUNT&&plan.count<r.units;++index){
   const auto* child=item->GetItem(index);if(Charged32SinglePoison(child,r.input)&&!add(EQ::InventoryProfile::CalcSlotId(root,index),child))return false;
  }
 }
 return plan.count==r.units;
}
bool Charged32PackingStillOriginal(Client& user,const ChargedRecipe32::Requirement& r,const ChargedPacking32Plan& plan) {
 if(user.GetClass()!=Class::Rogue||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user.GetInv())||
  !Charged32Kit(user,r,plan.kit_slot,plan.kit)||plan.kit->GetSerialNumber()!=plan.kit_serial||plan.count!=r.units)return false;
 for(size_t n=0;n<plan.count;++n){const auto& take=plan.takes[n];const auto* item=user.GetInv().GetItem(take.slot);if(item!=take.original||!Charged32SinglePoison(item,r.input)||item->GetSerialNumber()!=take.serial)return false;}
 for(const auto& child:*plan.kit->GetContents())if(child.second){bool selected=false;for(size_t n=0;n<plan.count;++n)if(plan.takes[n].slot==EQ::InventoryProfile::CalcSlotId(plan.kit_slot,child.first)&&plan.takes[n].original==child.second)selected=true;if(!selected)return false;}
 return true;
}
}

static const EQ::skills::SkillType TradeskillUnknown = EQ::skills::Skill1HBlunt; /* an arbitrary non-tradeskill */

void Object::HandleAugmentation(Client* user, const AugmentItem_Struct* in_augment, Object *worldo)
{
	if (!user || !in_augment) {
		LogError("Client or AugmentItem_Struct not set in Object::HandleAugmentation");
		return;
	}

	EQ::ItemInstance* container = nullptr;

	if (worldo) {
		container = worldo->m_inst;
	} else { // Check to see if they have an inventory container type 53 that is used for this.
		EQ::InventoryProfile& user_inv = user->GetInv();
		EQ::ItemInstance* inst = nullptr;

		inst = user_inv.GetItem(in_augment->container_slot);
		if (inst) {
			const EQ::ItemData* item = inst->GetItem();
			if (item && inst->IsType(EQ::item::ItemClassBag) && (item->BagType == EQ::item::BagTypeAugmentationSealer || item->BagType == RuleI(Inventory, AlternateAugmentationSealer))) { // We have found an appropriate inventory augmentation sealer
				container = inst;

				// Verify that no more than two items are in container to guarantee no inadvertant wipes.
				uint8 items_found = 0;
				for (uint8 i = EQ::invbag::SLOT_BEGIN; i < EQ::invtype::WORLD_SIZE; i++) {
					const EQ::ItemInstance* inst = container->GetItem(i);
					if (inst) {
						items_found++;
					}
				}

				if (items_found < 2) {
					user->Message(Chat::Red, "Error: Too few items in augmentation container.");
					return;
				} else if (items_found > 2) {
					user->Message(Chat::Red, "Error: Too many items in augmentation container.");
					return;
				}
			}
		}
	}

	if(!container) {
		LogError("Player tried to augment an item without a container set");
		user->Message(Chat::Red, "Error: This item is not a container!");
		return;
	}

	EQ::ItemInstance *tobe_auged = nullptr, *auged_with = nullptr;
	int8 slot = -1;

	if (container->GetItem(0) && container->GetItem(1)) { // Verify 2 items in the augmentation device
		// Verify 1 item is augmentable and the other is not
		if (container->GetItem(0)->IsAugmentable() && !container->GetItem(1)->IsAugmentable()) {
			tobe_auged = container->GetItem(0);
			auged_with = container->GetItem(1);
		} else if (!container->GetItem(0)->IsAugmentable() && container->GetItem(1)->IsAugmentable()) {
			tobe_auged = container->GetItem(1);
			auged_with = container->GetItem(0);
		} else {
			// Either 2 augmentable items found or none found
			// This should never occur due to client restrictions, but prevent in case of a hack
			user->Message(Chat::Red, "Error: There must be 1 augmentable item in the sealer.");
			return;
		}
	} else { // This happens if the augment button is clicked more than once quickly while augmenting
		if (!container->GetItem(0))	{
			user->Message(Chat::Red, "Error: No item in the first slot of sealer.");
		}

		if (!container->GetItem(1)) {
			user->Message(Chat::Red, "Error: No item in the second slot of sealer.");
		}
		return;
	}

	if (
		RuleB(Inventory, EnforceAugmentRestriction) &&
		user->IsAugmentRestricted(tobe_auged->GetItemType(), auged_with->GetAugmentRestriction())
	) {
		user->MessageString(Chat::Red, AUGMENT_RESTRICTED);
		return;
	}

	if (
		!RuleB(Inventory, AllowMultipleOfSameAugment) &&
		tobe_auged->ContainsAugmentByID(auged_with->GetID())
	) {
		user->Message(Chat::Red, "Error: Cannot put multiple of the same augment in an item.");
		return;
	}

	bool delete_items = false;

	EQ::ItemInstance *item_one_to_push = nullptr, *item_two_to_push = nullptr;

	if (in_augment->augment_slot == -1) { // Adding augment
		if (
			((slot = tobe_auged->AvailableAugmentSlot(auged_with->GetAugmentType())) != -1) &&
			tobe_auged->AvailableWearSlot(auged_with->GetItem()->Slots)
		) {
			tobe_auged->PutAugment(slot, *auged_with);

			EQ::ItemInstance *aug = tobe_auged->GetAugment(slot);
			if(aug) {
				std::vector<std::any> args;
				args.push_back(aug);

				if (parse->ItemHasQuestSub(tobe_auged, EVENT_AUGMENT_ITEM)) {
					parse->EventItem(EVENT_AUGMENT_ITEM, user, tobe_auged, nullptr, "", slot, &args);
				}

				args.assign(1, tobe_auged);

				if (parse->ItemHasQuestSub(aug, EVENT_AUGMENT_INSERT)) {
					parse->EventItem(EVENT_AUGMENT_INSERT, user, aug, nullptr, "", slot, &args);
				}

				args.push_back(aug);

				if (parse->PlayerHasQuestSub(EVENT_AUGMENT_INSERT_CLIENT)) {
					const auto& export_string = fmt::format(
						"{} {} {} {}",
						tobe_auged->GetID(),
						-1,
						aug->GetID(),
						slot
					);

					parse->EventPlayer(EVENT_AUGMENT_INSERT_CLIENT, user, export_string, 0, &args);
				}
			}

			item_one_to_push = tobe_auged->Clone();
			delete_items = true;
		} else {
			user->Message(Chat::Red, "Error: No available slot for augment.");
		}
	} else {
		EQ::ItemInstance *old_aug = nullptr;
		bool is_solvent = auged_with->GetItem()->ItemType == EQ::item::ItemTypeAugmentationSolvent;
		if (!is_solvent && auged_with->GetItem()->ItemType != EQ::item::ItemTypeAugmentationDistiller) {
			LogError("Player tried to remove an augment without a solvent or distiller");
			user->Message(Chat::Red, "Error: Missing an augmentation solvent or distiller for removing this augment.");

			return;
		}

		EQ::ItemInstance *aug = tobe_auged->GetAugment(in_augment->augment_slot);
		if (aug) {
			if (!is_solvent && auged_with->GetItem()->ID != aug->GetItem()->AugDistiller) {
				LogError("Player tried to safely remove an augment with the wrong distiller (item [{}] vs expected [{}])", auged_with->GetItem()->ID, aug->GetItem()->AugDistiller);
				user->Message(Chat::Red, "Error: Wrong augmentation distiller for safely removing this augment.");
				return;
			}
			std::vector<std::any> args;
			args.push_back(aug);

			if (parse->ItemHasQuestSub(tobe_auged, EVENT_UNAUGMENT_ITEM)) {
				parse->EventItem(EVENT_UNAUGMENT_ITEM, user, tobe_auged, nullptr, "", slot, &args);
			}

			args.assign(1, tobe_auged);
			args.push_back(&is_solvent);

			if (parse->ItemHasQuestSub(aug, EVENT_AUGMENT_REMOVE)) {
				parse->EventItem(EVENT_AUGMENT_REMOVE, user, aug, nullptr, "", slot, &args);
			}
		}

		if (is_solvent) {
			tobe_auged->DeleteAugment(in_augment->augment_slot);
		} else {
			old_aug = tobe_auged->RemoveAugment(in_augment->augment_slot);
		}

		item_one_to_push = tobe_auged->Clone();
		if (old_aug) {
			item_two_to_push = old_aug->Clone();
		}

		delete_items = true;
	}

	if (delete_items) {
		if (worldo) {
			container->Clear();
			auto outapp = new EQApplicationPacket(OP_ClearObject, sizeof(ClearObject_Struct));
			ClearObject_Struct *cos = (ClearObject_Struct *)outapp->pBuffer;
			cos->Clear = 1;
			user->QueuePacket(outapp);
			safe_delete(outapp);
			database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
		} else { // Delete items in our inventory container...
			for (uint8 i = EQ::invbag::SLOT_BEGIN; i < EQ::invtype::WORLD_SIZE; i++) {
				const EQ::ItemInstance* inst = container->GetItem(i);
				if (inst) {
					user->DeleteItemInInventory(EQ::InventoryProfile::CalcSlotId(in_augment->container_slot, i), 0, true);
				}
			}

			container->Clear(); // Explicitly mark container as cleared.
		}
	}

	// Must push items after the items in inventory are deleted - necessary due to lore items...
	if (item_one_to_push) {
		user->PushItemOnCursor(*item_one_to_push, true);
	}

	if (item_two_to_push) {
		user->PushItemOnCursor(*item_two_to_push, true);
	}
}

// Perform tradeskill combine
void Object::HandleCombine(Client* user, const NewCombine_Struct* in_combine, Object *worldo)
{
 if(user && user->GetTradeskillObject() && user->GetTradeskillObject()->IsHousingRecipe301()) {
  user->Message(Chat::Red,"Keep ingredients in your inventory and use the recipe list at this table.");auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;
 }

	if (!user || !in_combine) {
		LogError("Client or NewCombine_Struct not set in Object::HandleCombine");
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	LogTradeskills(
		"container_slot [{}] guildtribute_slot [{}]",
		in_combine->container_slot,
		in_combine->guildtribute_slot
	);

	EQ::InventoryProfile &user_inv  = user->GetInv();
	PlayerProfile_Struct &user_pp   = user->GetPP();
	EQ::ItemInstance     *container = nullptr;
	EQ::ItemInstance     *inst      = nullptr;

	uint8  c_type         = 0xE8;
	uint32 some_id        = 0;
	bool   worldcontainer = false;


	if (in_combine->container_slot == EQ::invslot::SLOT_TRADESKILL_EXPERIMENT_COMBINE) {
		if(!worldo) {
			user->Message(
				Chat::Red,
				"Error: Server is not aware of the tradeskill container you are attempting to use"
			);
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
		c_type         = worldo->m_type;
		inst           = worldo->m_inst;
		worldcontainer = true;
		// if we're a world container with an item, use that too
		if (inst) {
			const EQ::ItemData *item = inst->GetItem();
			if (item) {
				some_id = item->ID;
			}
		}
	}
	else {
		inst = user_inv.GetItem(in_combine->container_slot);
		if (inst) {
			const EQ::ItemData* item = inst->GetItem();
			if (item && inst->IsType(EQ::item::ItemClassBag)) {
				c_type = item->BagType;
				some_id = item->ID;
			}
		}
	}

	if (!inst || !inst->IsType(EQ::item::ItemClassBag)) {
		user->Message(Chat::Red, "Error: Server does not recognize specified tradeskill container");
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	container = inst;
	if (container->GetItem() && container->GetItem()->BagType == EQ::item::BagTypeTransformationmold) {
		const EQ::ItemInstance *inst = container->GetItem(0);
		bool AllowAll = RuleB(Inventory, AllowAnyWeaponTransformation);
		if (inst && EQ::ItemInstance::CanTransform(inst->GetItem(), container->GetItem(), AllowAll)) {
			const EQ::ItemData *new_weapon = inst->GetItem();
			user->DeleteItemInInventory(EQ::InventoryProfile::CalcSlotId(in_combine->container_slot, 0), 0, true);
			container->Clear();
			user->SummonItem(
				new_weapon->ID,
				inst->GetCharges(),
				inst->GetAugmentItemID(0),
				inst->GetAugmentItemID(1),
				inst->GetAugmentItemID(2),
				inst->GetAugmentItemID(3),
				inst->GetAugmentItemID(4),
				inst->GetAugmentItemID(5),
				inst->IsAttuned(),
				EQ::invslot::slotCursor,
				container->GetItem()->Icon,
				Strings::ToInt(container->GetItem()->IDFile + 2)
			);

			user->MessageString(Chat::LightBlue, TRANSFORM_COMPLETE, inst->GetItem()->Name);
			if (RuleB(Inventory, DeleteTransformationMold)) {
				user->DeleteItemInInventory(in_combine->container_slot, 0, true);
			}
		}
		else if (inst) {
			user->MessageString(Chat::LightBlue, TRANSFORM_FAILED, inst->GetItem()->Name);
		}
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);

		LogTradeskills(
			"inst_item [{}] container_item [{}]",
			inst->GetItem()->ID,
			container->GetItem()->ID
		);

		return;
	}

	if (container->GetItem() && container->GetItem()->BagType == EQ::item::BagTypeDetransformationmold) {
		LogTradeskillsDetail("Check 1");

		const EQ::ItemInstance* inst = container->GetItem(0);
		if (inst && inst->GetOrnamentationIcon()) {
			const EQ::ItemData* new_weapon = inst->GetItem();
			user->DeleteItemInInventory(EQ::InventoryProfile::CalcSlotId(in_combine->container_slot, 0), 0, true);
			container->Clear();
			user->SummonItem(new_weapon->ID, inst->GetCharges(), inst->GetAugmentItemID(0), inst->GetAugmentItemID(1), inst->GetAugmentItemID(2), inst->GetAugmentItemID(3), inst->GetAugmentItemID(4), inst->GetAugmentItemID(5), inst->IsAttuned(), EQ::invslot::slotCursor, 0, 0);
			user->MessageString(Chat::LightBlue, TRANSFORM_COMPLETE, inst->GetItem()->Name);
		}
		else if (inst) {
			user->MessageString(Chat::LightBlue, DETRANSFORM_FAILED, inst->GetItem()->Name);
		}
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	DBTradeskillRecipe_Struct spec;
	bool is_augmented = false;

	if (parse->PlayerHasQuestSub(EVENT_COMBINE)) {
		if (parse->EventPlayer(EVENT_COMBINE, user, std::to_string(in_combine->container_slot), 0) == 1) {
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (!content_db.GetTradeRecipe(container, c_type, some_id, user, &spec, &is_augmented)) {

		LogTradeskillsDetail("Check 2");

		if (!is_augmented) {
			user->MessageString(Chat::Emote, TRADESKILL_NOCOMBINE);
		} else {
			user->Message(Chat::Emote, "You must remove augments from all component items before you can attempt this combine.");
		}

		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	// Finite pre-2016 research path: actual actor and actual opened native kit.
	if (ResearchTablet335::Selected(spec.recipe_id)) {
		const auto* kit = !worldcontainer && container->IsType(EQ::item::ItemClassBag)
			? container->GetItem() : nullptr;
		if (!kit || !ResearchTablet335::Allows(spec.recipe_id, spec.tradeskill,
			user->GetClass(), kit->ID, kit->BagType, kit->BagSlots, kit->BagSize)) {
			user->Message(Chat::Red, "Your class and research kit must match this recipe.");
			auto* denied = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(denied);
			safe_delete(denied);
			return;
		}
	}

	// Native actor restriction for the exact RRC169 source-qualified formulas.
	// Container identity comes from the actual opened native bag, not a packet.
	if (!GnomishTailoring35::Allows(spec.recipe_id, spec.tradeskill, c_type,
		container->GetItem() ? container->GetItem()->ID : 0, user->GetPP().race)) {
		user->Message(Chat::Red, "Only gnomes can make this cultural armor.");
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	// Exact cultural recipe actor and actual owned world station, before consumption.
	if (const auto* requirement = CulturalArmorBulk::Find(spec.recipe_id)) {
		auto* station = worldcontainer ? worldo : nullptr;
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station != user->GetTradeskillObject() || station->user != user ||
			!station->m_inst || !station->m_inst->IsType(EQ::item::ItemClassBag) ||
			!CulturalArmorBulk::Allows(*requirement, spec.tradeskill, user->GetPP().race,
				zone->GetZoneID(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This cultural armor requires the correct race and forge.");
			auto* denied = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(denied);
			safe_delete(denied);
			return;
		}
	}

	if (const auto* requirement = CulturalTailoringBulk::Find(spec.recipe_id)) {
		const auto* kit = !worldcontainer && container ? container->GetItem() : nullptr;
		if (!kit || !CulturalTailoringBulk::Allows(*requirement, spec.tradeskill,
			user->GetPP().race, kit->ID, kit->BagType, kit->BagSlots, kit->BagSize)) {
			user->Message(Chat::Red, "This cultural armor requires the correct race and sewing kit.");
			auto* denied = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(denied);
			safe_delete(denied);
			return;
		}
	}

	// Exact source world-only loom routes: an inventory bag is not authority.
	if (const auto* requirement = WorldLoom13::Find(spec.recipe_id)) {
		auto* station = worldcontainer ? worldo : nullptr;
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station != user->GetTradeskillObject() || station->user != user ||
			!station->m_inst || !station->m_inst->IsType(EQ::item::ItemClassBag) ||
			!WorldLoom13::Allows(*requirement, spec.tradeskill, zone->GetZoneID(),
				zone->GetInstanceVersion(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This recipe requires the loom in the Plane of Knowledge.");
			auto* denied = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(denied);
			safe_delete(denied);
			return;
		}
	}

	// Exact source Tanaan Forge routes: an inventory bag is not authority.
	if (const auto* requirement = TanaanForge28::Find(spec.recipe_id)) {
		auto* station = worldcontainer ? worldo : nullptr;
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station != user->GetTradeskillObject() || station->user != user ||
			!station->m_inst || !station->m_inst->IsType(EQ::item::ItemClassBag) ||
			!TanaanForge28::Allows(*requirement, spec.tradeskill, zone->GetZoneID(),
				zone->GetInstanceVersion(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This recipe requires a Tanaan forge in the Plane of Knowledge.");
			auto* denied = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(denied);
			safe_delete(denied);
			return;
		}
	}



 if(HousingRecipe301::Selected(spec.recipe_id)) {
  user->Message(Chat::Red,"This recipe needs a placed Jewelry Making Table and its recipe list.");auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;
 }
 // Capture the actual matched physical input tree before mutable validation hooks.
 // No clone/replacement/consumption; compare identities and all nested charges/slots.
 // Keep the original finite307/309 selector bound across mutable validation.
 const auto final307_recipe=(spec.recipe_id==3102867 || spec.recipe_id==3102868 || spec.recipe_id==3102882)?spec.recipe_id:0;
 const auto recharge_recipe=RechargeRecipe8::Find(spec.recipe_id)?spec.recipe_id:0;
 const auto recharge_parent_slot=in_combine->container_slot;
 std::unique_ptr<Final300InputTree> recharge_inputs;
 std::unique_ptr<Final300InputTree> final300_inputs;
 const auto final300_owner=user->InventorySessionReleaseTicket();
 const auto final300_character=user->CharacterID();
 const auto final300_account=user->AccountID();
 auto* const final300_world=worldcontainer?worldo:nullptr;
 const auto final300_world_id=final300_world?final300_world->GetDBID():0;
 if(recharge_recipe) {
  if(worldcontainer || !container || !final300_owner || !final300_owner->HeldFor(*user)) {
   auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;
  }
  try { recharge_inputs=std::make_unique<Final300InputTree>(Final300InputTree{Final300InputTree::Capture(*container)}); }
  catch(...) {auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 }
 if(FinalRecipe300::Selected(spec.recipe_id)) {
  if(!container || !final300_owner || !final300_owner->HeldFor(*user)) {
   auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;
  }
  try { final300_inputs=std::make_unique<Final300InputTree>(Final300InputTree{Final300InputTree::Capture(*container)}); }
  catch(...) {auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 }

 const bool ogre303_selected=spec.recipe_id>=3102788&&spec.recipe_id<=3102794;
 std::unique_ptr<Final300InputTree> ogre303_inputs;
 if(ogre303_selected) {
  if(!container||!final300_owner||!final300_owner->HeldFor(*user)){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
  try{ogre303_inputs=std::make_unique<Final300InputTree>(Final300InputTree{Final300InputTree::Capture(*container)});}
  catch(...){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 }
 const auto ogre303_valid=[&](){
  if(!ogre303_selected)return true;
  if(worldcontainer||!ogre303_inputs||!final300_owner||!final300_owner->HeldFor(*user)||user->CharacterID()!=final300_character||user->AccountID()!=final300_account||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  const auto* requirement=CulturalTailoringBulk::Find(spec.recipe_id);
  const auto* current=user->GetInv().GetItem(in_combine->container_slot);
  const auto* data=current&&current->IsClassBag()?current->GetItem():nullptr;
  if(!requirement||!data||current!=container||!EQ::ValueWithin(in_combine->container_slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END)||!CulturalTailoringBulk::Allows(*requirement,spec.tradeskill,user->GetPP().race,data->ID,data->BagType,data->BagSlots,data->BagSize))return false;
  return Final300InputTree::Same(ogre303_inputs->root,current);
 };
 if(!ogre303_valid()){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 const auto* final302_requirement=FinalRecipe302::Find(spec.recipe_id);
 std::unique_ptr<Final300InputTree> final302_inputs;
 if(final302_requirement) {
  if(!container||!final300_owner||!final300_owner->HeldFor(*user)){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
  try{final302_inputs=std::make_unique<Final300InputTree>(Final300InputTree{Final300InputTree::Capture(*container)});}
  catch(...){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 }
 const auto final302_valid=[&](){
  if(!final302_requirement)return true;
  const auto& r=*final302_requirement;
  if(!final302_inputs||!final300_owner||!final300_owner->HeldFor(*user)||user->CharacterID()!=final300_character||user->AccountID()!=final300_account||!Final302Output(spec,r)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  const EQ::ItemInstance* current=nullptr;
  if(r.kit>100) {
   if(worldcontainer||!Final302Kit(*user,r,in_combine->container_slot,container))return false;
   current=user->GetInv().GetItem(in_combine->container_slot);
  }else {
   auto* station=user->GetTradeskillObject();
   if(!worldcontainer||!station||station!=worldo||station!=final300_world||station->user!=user||!station->GetDBID()||station->GetDBID()!=final300_world_id||station->GetType()!=18||!station->m_inst||station->m_inst!=container||!station->m_inst->IsClassBag()||station->GetItemID()!=0)return false;
   current=station->m_inst;
  }
  return Final300InputTree::Same(final302_inputs->root,current);
 };
 if(!final302_valid()){user->Message(Chat::Red,"This recipe requires its correct container and original ingredients.");auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 const auto recharge_valid=[&](){
  if(!recharge_recipe && !RechargeRecipe8::Find(spec.recipe_id))return true;
  if(!recharge_recipe || spec.recipe_id!=recharge_recipe || worldcontainer || !recharge_inputs ||
   !final300_owner || !final300_owner->HeldFor(*user) || user->CharacterID()!=final300_character || user->AccountID()!=final300_account || user->GetPP().race!=12)return false;
  if(in_combine->container_slot!=recharge_parent_slot || !EQ::ValueWithin(recharge_parent_slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END))return false;
  const auto* current=user->GetInv().GetItem(recharge_parent_slot);
  const auto* data=current&&current->IsClassBag()?current->GetItem():nullptr;
  return current==container && data && data->ID==17902 && data->BagType==10 && data->BagSlots==8 && data->BagSize==4 &&
   Final300InputTree::Same(recharge_inputs->root,current);
 };
 if(!recharge_valid()){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 const auto final300_valid=[&](){
  if(final307_recipe && spec.recipe_id!=final307_recipe)return false;
  if(!FinalRecipe300::Selected(spec.recipe_id))return true;
  if(!final300_inputs || !final300_owner || !final300_owner->HeldFor(*user) ||
   user->CharacterID()!=final300_character || user->AccountID()!=final300_account)return false;
  if(!Final300OutputValid(spec)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv())||
   !FinalRecipe300::RaceAllowed(spec.recipe_id,user->GetPP().race))return false;
  // Resolve current native custody before touching the retained original pointer.
  const EQ::ItemInstance* current=nullptr;
  if(FinalRecipe300::Font(spec.recipe_id)) {
   if(worldcontainer || !Final300Kit(*user,in_combine->container_slot,container) || !Final300Font(*user,spec.recipe_id))return false;
   current=user->GetInv().GetItem(in_combine->container_slot);
  }else if(spec.recipe_id==3102867) {
   if(worldcontainer || !EQ::ValueWithin(in_combine->container_slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END))return false;
   current=user->GetInv().GetItem(in_combine->container_slot);
   const auto* data=current&&current->IsClassBag()?current->GetItem():nullptr;
   if(current!=container || !data || data->ID!=17910 || data->BagType!=18 || data->BagSlots!=8 || data->BagSize!=3)return false;
  }else {
   auto* station=user->GetTradeskillObject();
   if(!worldcontainer || !station || station!=worldo || station!=final300_world ||
    station->user!=user || station->GetDBID()!=final300_world_id || !station->m_inst ||
    station->m_inst!=container || !station->m_inst->IsType(EQ::item::ItemClassBag))return false;
   const auto* data=station->m_inst->GetItem();
   if(!data || !FinalRecipe300::WorldAllowed(spec.recipe_id,zone->GetZoneID(),zone->GetInstanceVersion(),station->GetDBID(),station->GetType(),data->ID))return false;
   current=station->m_inst;
  }
  return Final300InputTree::Same(final300_inputs->root,current);
 };

 if(!final300_valid()){user->Message(Chat::Red,"This recipe requires its correct station, location, and maker.");auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
 const auto* charged_requirement=ChargedRecipe32::Find(spec.recipe_id);
 ChargedPacking32Plan charged_packing;
 const auto charged_station_valid=[&](){
  if(!charged_requirement)return true;
  if(!Charged32OutputValid(spec)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  if(charged_requirement->units)return !worldcontainer&&user->GetInv().GetItem(in_combine->container_slot)==container;
  if(charged_requirement->kit==17){auto* actual=user->GetTradeskillObject();return worldcontainer&&actual&&actual==worldo&&actual->user==user&&actual->GetType()==17&&actual->m_inst&&actual->m_inst==container;}
  const auto* actual=user->GetInv().GetItem(in_combine->container_slot);const auto* data=actual&&actual->IsType(EQ::item::ItemClassBag)?actual->GetItem():nullptr;
  return !worldcontainer&&actual==container&&data&&data->ID==charged_requirement->kit&&data->BagType==10&&data->BagSlots>=charged_requirement->cells&&data->BagSize>=charged_requirement->maxsize&&user->GetPP().race==Race::Gnome;
 };
 if(!charged_station_valid()||(charged_requirement&&charged_requirement->units&&!Charged32PreparePacking(*user,*charged_requirement,in_combine->container_slot,true,charged_packing))){
  user->Message(Chat::Red,"This recipe needs its correct container and ingredients.");auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;
 }

	// Character hasn't learnt the recipe yet.
	// must_learn:
	// bit 1 (0x01): recipe can't be experimented
	// bit 2 (0x02): can try to experiment but not useable for auto-combine until learnt
	// bit 5 (0x10): no learn message, use unlisted flag to prevent it showing up on search
	// bit 6 (0x20): unlisted recipe flag
	if ((spec.must_learn & 0xF) == 1 && !spec.has_learnt) {

		LogTradeskillsDetail("Check 3");

		// Made up message for the client. Just giving a DNC is the other option.
		user->Message(Chat::LightBlue, "You need to learn how to combine these first.");
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	// Character does not have the required skill.
	if(spec.skill_needed > 0 && user->GetSkill(spec.tradeskill) < spec.skill_needed ) {

		LogTradeskillsDetail("Check 4");

		// Notify client.
		user->Message(Chat::LightBlue, "You are not skilled enough.");
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	//changing from a switch to string of if's since we don't need to iterate through all of the skills in the SkillType enum
	if (spec.tradeskill == EQ::skills::SkillAlchemy) {
		if (user_pp.class_ != Class::Shaman) {
			user->Message(Chat::Red, "This tradeskill can only be performed by a shaman.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
		else if (user_pp.level < MIN_LEVEL_ALCHEMY) {
			user->Message(Chat::Red, "You cannot perform alchemy until you reach level %i.", MIN_LEVEL_ALCHEMY);
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}
	else if (spec.tradeskill == EQ::skills::SkillTinkering) {
		if (user_pp.race != Race::Gnome) {
			user->Message(Chat::Red, "Only gnomes can tinker.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}
	else if (spec.tradeskill == EQ::skills::SkillMakePoison) {
		if (user_pp.class_ != Class::Rogue) {
			user->Message(Chat::Red, "Only rogues can mix poisons.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	// Check if Combine would result in Lore conflict
	if (!TrophyCertificate55::Find(spec.recipe_id) && user->CheckTradeskillLoreConflict(spec.recipe_id)) {
		auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	// final check for any additional quest requirements .. "check_zone" in this case - exported as variable [validate_type]
	if (parse->PlayerHasQuestSub(EVENT_COMBINE_VALIDATE)) {
		if (parse->EventPlayer(EVENT_COMBINE_VALIDATE, user, fmt::format("check_zone {}", zone->GetZoneID()), spec.recipe_id) != 0) {
			user->Message(
				Chat::Emote,
				"You cannot make this combine because the location requirement has not been met."
			);
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}


 if(!recharge_valid()||!ogre303_valid()||!final302_valid()||!final300_valid()||!charged_station_valid()||(charged_requirement&&charged_requirement->units&&!Charged32PackingStillOriginal(*user,*charged_requirement,charged_packing))){auto* denied=new EQApplicationPacket(OP_TradeSkillCombine,0);user->QueuePacket(denied);safe_delete(denied);return;}
	// Send acknowledgement packets to client
	auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
	user->QueuePacket(outapp);
	safe_delete(outapp);

 std::unique_ptr<RechargeRecipe8::Proof> recharge_proof;
 if(RechargeRecipe8::Find(spec.recipe_id)) {
  recharge_proof=RechargeRecipe8::Custody::Manual(*user,spec,worldcontainer?worldo:nullptr,container,in_combine->container_slot);
  if(!recharge_proof)return;
 }
 std::unique_ptr<TrophyCertificate55::Proof> trophy_proof;
 if(TrophyCertificate55::Find(spec.recipe_id)) {
  trophy_proof=TrophyCertificate55::Custody::Manual(*user,spec,worldcontainer?worldo:nullptr,container,in_combine->container_slot);
  if(!trophy_proof)return;
 }
 if(!trophy_proof && !recharge_proof){
	//now clean out the containers.
	if(worldcontainer){
		container->Clear();
		outapp = new EQApplicationPacket(OP_ClearObject, sizeof(ClearObject_Struct));
		ClearObject_Struct *cos = (ClearObject_Struct *)outapp->pBuffer;
		cos->Clear = 1;
		user->QueuePacket(outapp);
		safe_delete(outapp);
		database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
	} else{
		for (uint8 i = EQ::invbag::SLOT_BEGIN; i < EQ::invtype::WORLD_SIZE; i++) {
			const EQ::ItemInstance* inst = container->GetItem(i);
			if (inst) {
				user->DeleteItemInInventory(EQ::InventoryProfile::CalcSlotId(in_combine->container_slot, i), 0, true);
			}
		}
		container->Clear();
	}
	//do the check and send results...

 if(charged_requirement&&charged_requirement->units){
  // Inputs are already gone. Remove this exact emptied no-fail bottle BEFORE
  // output/task callbacks, and suppress the stock delayed second removal.
  if(user->GetInv().GetItem(charged_packing.kit_slot)!=charged_packing.kit||!charged_packing.kit->GetContents()->empty())return;
  user->DeleteItemInInventory(charged_packing.kit_slot,0,true);spec.replace_container=false;
 }
 }
 if(trophy_proof || recharge_proof)user->CalcBonuses();
 bool success=recharge_proof?RechargeRecipe8::Custody::Complete(*user,std::move(recharge_proof),spec):
  trophy_proof?TrophyCertificate55::Custody::Complete(*user,std::move(trophy_proof),spec):user->TradeskillExecute(&spec);

	// Learn new recipe message
	// Update Made count
	if (success) {
		if (!spec.has_learnt && ((spec.must_learn&0x10) != 0x10)) {
			user->MessageString(Chat::LightBlue, TRADESKILL_LEARN_RECIPE, spec.name.c_str());
		}
		database.UpdateRecipeMadecount(spec.recipe_id, user->CharacterID(), spec.madecount+1);
	}

	// Replace the container on success if required.
	//

	if (success && spec.replace_container) {
		if (worldcontainer) {
			//should report this error, but we dont have the recipe ID, so its not very useful
			LogError("Replace container combine executed in a world container");
		}
		else {
			user->DeleteItemInInventory(in_combine->container_slot, 0, true);
		}
	}

	if (success) {
		if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::COMBINE_SUCCESS)) {
			auto e = PlayerEvent::CombineEvent{
				.recipe_id = spec.recipe_id,
				.recipe_name = spec.name,
				.made_count = spec.madecount,
				.tradeskill_id = (uint32) spec.tradeskill
			};
			RecordPlayerEventLogWithClient(user, PlayerEvent::COMBINE_SUCCESS, e);
		}

		if (parse->PlayerHasQuestSub(EVENT_COMBINE_SUCCESS)) {
			parse->EventPlayer(EVENT_COMBINE_SUCCESS, user, spec.name, spec.recipe_id);
		}
	}
	else {
		if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::COMBINE_FAILURE)) {
			auto e = PlayerEvent::CombineEvent{
				.recipe_id = spec.recipe_id,
				.recipe_name = spec.name,
				.made_count = spec.madecount,
				.tradeskill_id = (uint32) spec.tradeskill
			};
			RecordPlayerEventLogWithClient(user, PlayerEvent::COMBINE_FAILURE, e);
		}

		if (parse->PlayerHasQuestSub(EVENT_COMBINE_FAILURE)) {
			parse->EventPlayer(EVENT_COMBINE_FAILURE, user, spec.name, spec.recipe_id);
		}
	}
}

void Object::HandleAutoCombine(Client* user, const RecipeAutoCombine_Struct* rac) {

	//get our packet ready, gotta send one no matter what...
	auto outapp = new EQApplicationPacket(OP_RecipeAutoCombine, sizeof(RecipeAutoCombine_Struct));
	RecipeAutoCombine_Struct *outp = (RecipeAutoCombine_Struct *)outapp->pBuffer;
	outp->object_type = rac->object_type;
	outp->some_id = rac->some_id;
	outp->unknown1 = rac->unknown1;
	outp->recipe_id = rac->recipe_id;
	outp->reply_code = 0xFFFFFFF5;	//default fail.


 if(auto* station=user->GetTradeskillObject();station&&station->IsHousingRecipe301()&&!HousingRecipe301::Selected(rac->recipe_id)){user->QueuePacket(outapp);safe_delete(outapp);return;}

	//ask the database for the recipe to make sure it exists...
	DBTradeskillRecipe_Struct spec;
	if (!content_db.GetTradeRecipe(rac->recipe_id, rac->object_type, rac->some_id, user, &spec)) {
		LogError("Unknown recipe for HandleAutoCombine: [{}]\n", rac->recipe_id);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

 // Reject the original recharge selector before mutable validation callbacks.
 // The later gate also rejects an ordinary selector changed into a recharge.
 if(RechargeRecipe8::Find(spec.recipe_id)) {
  user->Message(Chat::Red,"Use an opened Toolbox to recharge this device.");user->QueuePacket(outapp);safe_delete(outapp);return;
 }

	if (ResearchTablet335::Selected(spec.recipe_id)) {
		// Packet recipe/container identifiers confer no ownership or maker-class authority.
		const auto kit_slot = user->GetInv().HasItem(ResearchTablet335::RequiredKit(spec.recipe_id), 1, invWherePersonal);
		const auto* native_kit = user->GetInv().GetItem(kit_slot);
		const auto* kit = native_kit && native_kit->IsType(EQ::item::ItemClassBag)
			? native_kit->GetItem() : nullptr;
		if (!kit || rac->some_id != kit->ID || rac->object_type != kit->BagType ||
			!ResearchTablet335::Allows(spec.recipe_id, spec.tradeskill, user->GetClass(),
				kit->ID, kit->BagType, kit->BagSlots, kit->BagSize)) {
			user->Message(Chat::Red, "Your class and research kit must match this recipe.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (GnomishTailoring35::Selected(spec.recipe_id)) {
		// The auto request names an item ID. Require the actual matching kit in
		// this actor's native personal inventory before trusting that request.
		const auto kit_slot = user->GetInv().HasItem(92877, 1, invWherePersonal);
		const auto* kit = user->GetInv().GetItem(kit_slot);
		const auto* kit_data = kit && kit->IsType(EQ::item::ItemClassBag) ? kit->GetItem() : nullptr;
		if (!kit_data || rac->some_id != kit_data->ID || rac->object_type != kit_data->BagType ||
			!GnomishTailoring35::Allows(spec.recipe_id, spec.tradeskill, kit_data->BagType,
				kit_data->ID, user->GetPP().race)) {
			user->Message(Chat::Red, "Only gnomes with a Clockwork Sewing Kit can make this cultural armor.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (const auto* requirement = CulturalArmorBulk::Find(spec.recipe_id)) {
		// Packet IDs confer no forge authority: use this client's owned open object.
		auto* station = user->GetTradeskillObject();
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station->user != user || !station->m_inst ||
			!station->m_inst->IsType(EQ::item::ItemClassBag) ||
			rac->object_type != station->GetType() ||
			!CulturalArmorBulk::Allows(*requirement, spec.tradeskill, user->GetPP().race,
				zone->GetZoneID(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This cultural armor requires the correct race and forge.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (const auto* requirement = CulturalTailoringBulk::Find(spec.recipe_id)) {
		// Packet item IDs are insufficient: require the actual native personal kit.
		const auto slot = user->GetInv().HasItem(requirement->kit_id, 1, invWherePersonal);
		const auto* instance = user->GetInv().GetItem(slot);
		const auto* kit = instance && instance->IsType(EQ::item::ItemClassBag) ? instance->GetItem() : nullptr;
		if (!kit || rac->some_id != kit->ID || rac->object_type != kit->BagType ||
			!CulturalTailoringBulk::Allows(*requirement, spec.tradeskill,
				user->GetPP().race, kit->ID, kit->BagType, kit->BagSlots, kit->BagSize)) {
			user->Message(Chat::Red, "This cultural armor requires the correct race and sewing kit.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (const auto* requirement = WorldLoom13::Find(spec.recipe_id)) {
		// Use the actual owned open native world object, never packet IDs alone.
		auto* station = user->GetTradeskillObject();
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station->user != user || !station->m_inst ||
			!station->m_inst->IsType(EQ::item::ItemClassBag) ||
			rac->object_type != station->GetType() ||
			!WorldLoom13::Allows(*requirement, spec.tradeskill, zone->GetZoneID(),
				zone->GetInstanceVersion(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This recipe requires the loom in the Plane of Knowledge.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

	if (const auto* requirement = TanaanForge28::Find(spec.recipe_id)) {
		// Use the actual owned open native world object, never packet IDs alone.
		auto* station = user->GetTradeskillObject();
		const auto* item = station && station->m_inst ? station->m_inst->GetItem() : nullptr;
		if (!station || station->user != user || !station->m_inst ||
			!station->m_inst->IsType(EQ::item::ItemClassBag) ||
			rac->object_type != station->GetType() ||
			!TanaanForge28::Allows(*requirement, spec.tradeskill, zone->GetZoneID(),
				zone->GetInstanceVersion(), station->GetDBID(), station->GetType(),
				item ? item->ID : 0, EQ::invtype::WORLD_SIZE)) {
			user->Message(Chat::Red, "This recipe requires a Tanaan forge in the Plane of Knowledge.");
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}



 const bool ogre303_selected=spec.recipe_id>=3102788&&spec.recipe_id<=3102794;
 const auto ogre303_valid=[&](){
  if(!ogre303_selected)return true;
  auto ticket=user->InventorySessionReleaseTicket();if(!ticket||!ticket->HeldFor(*user)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  const auto* requirement=CulturalTailoringBulk::Find(spec.recipe_id);
  const auto slot=user->GetInv().HasItem(92880,1,invWherePersonal);const auto* current=user->GetInv().GetItem(slot);const auto* data=current&&current->IsClassBag()?current->GetItem():nullptr;
  return requirement&&data&&EQ::ValueWithin(slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END)&&rac->some_id==92880&&rac->object_type==16&&CulturalTailoringBulk::Allows(*requirement,spec.tradeskill,user->GetPP().race,data->ID,data->BagType,data->BagSlots,data->BagSize);
 };
 if(!ogre303_valid()){user->QueuePacket(outapp);safe_delete(outapp);return;}
 const auto* final302_requirement=FinalRecipe302::Find(spec.recipe_id);
 const auto final302_valid=[&](){
  if(!final302_requirement)return true;
  const auto& r=*final302_requirement;
  if(!Final302Output(spec,r)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  auto ticket=user->InventorySessionReleaseTicket();if(!ticket||!ticket->HeldFor(*user))return false;
  if(r.kit>100) {
   const auto slot=user->GetInv().HasItem(r.kit,1,invWherePersonal);
   return rac->some_id==r.kit&&rac->object_type==r.type&&Final302Kit(*user,r,slot,user->GetInv().GetItem(slot));
  }
  auto* station=user->GetTradeskillObject();
  return station&&station->user==user&&station->GetDBID()&&station->GetType()==18&&rac->object_type==18&&station->m_inst&&station->m_inst->IsClassBag()&&station->GetItemID()==0;
 };
 if(!final302_valid()){user->Message(Chat::Red,"This recipe requires its correct container.");user->QueuePacket(outapp);safe_delete(outapp);return;}
 // Bind only the new Northman309 route to its original source/session before callbacks.
 const auto final309_recipe=spec.recipe_id==3102882?spec.recipe_id:0;
 const auto final309_owner=user->InventorySessionReleaseTicket();
 const auto final309_character=user->CharacterID();
 const auto final309_account=user->AccountID();
 auto* const final309_world=user->GetTradeskillObject();
 const auto final309_world_id=final309_world?final309_world->GetDBID():0;
 const auto* const final309_container=final309_world?final309_world->m_inst:nullptr;
 const auto final309_serial=final309_container?final309_container->GetSerialNumber():0;
 const auto final300_valid=[&](){
  if(spec.recipe_id==3102882 || final309_recipe) {
   if(!final309_recipe || spec.recipe_id!=final309_recipe || !final309_owner ||
    !final309_owner->HeldFor(*user) || user->CharacterID()!=final309_character || user->AccountID()!=final309_account)return false;
   auto* current=user->GetTradeskillObject();
   if(!current || current!=final309_world || current->GetDBID()!=final309_world_id || current->user!=user ||
    !current->m_inst || current->m_inst!=final309_container || current->m_inst->GetSerialNumber()!=final309_serial)return false;
  }
  if(!FinalRecipe300::Selected(spec.recipe_id))return true;
  if(!Final300OutputValid(spec)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv())||
   !FinalRecipe300::RaceAllowed(spec.recipe_id,user->GetPP().race))return false;
  if(FinalRecipe300::NeverLearn(spec.recipe_id))return false;
  if(FinalRecipe300::Font(spec.recipe_id)) {
   const auto slot=user->GetInv().HasItem(17187,1,invWherePersonal);
   return rac->some_id==17187 && rac->object_type==20 && Final300Kit(*user,slot,user->GetInv().GetItem(slot)) && Final300Font(*user,spec.recipe_id);
  }
  auto* station=user->GetTradeskillObject();
  const auto* data=station&&station->m_inst?station->m_inst->GetItem():nullptr;
  return station && station->user==user && station->m_inst && station->m_inst->IsType(EQ::item::ItemClassBag) && data &&
   rac->object_type==station->GetType() &&
   FinalRecipe300::WorldAllowed(spec.recipe_id,zone->GetZoneID(),zone->GetInstanceVersion(),station->GetDBID(),station->GetType(),data->ID);
 };
 if(!final300_valid()){user->Message(Chat::Red,"This recipe requires its correct station, location, and maker.");user->QueuePacket(outapp);safe_delete(outapp);return;}
 const auto* charged_requirement=ChargedRecipe32::Find(spec.recipe_id);
 ChargedPacking32Plan charged_packing;
 const auto charged_station_valid=[&](){
  if(!charged_requirement)return true;
  if(!Charged32OutputValid(spec)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&user->GetInv()))return false;
  if(charged_requirement->units)return true;
  if(charged_requirement->kit==17){auto* station=user->GetTradeskillObject();return station&&station->user==user&&station->m_inst&&station->m_inst->IsType(EQ::item::ItemClassBag)&&station->GetType()==17&&rac->object_type==17;}
  const auto slot=user->GetInv().HasItem(charged_requirement->kit,1,invWherePersonal);const auto* native=user->GetInv().GetItem(slot);const auto* data=native&&native->IsType(EQ::item::ItemClassBag)?native->GetItem():nullptr;
  return data&&data->ID==charged_requirement->kit&&data->BagType==10&&data->BagSlots>=charged_requirement->cells&&data->BagSize>=charged_requirement->maxsize&&rac->some_id==data->ID&&rac->object_type==data->BagType&&user->GetPP().race==Race::Gnome;
 };
 if(!charged_station_valid()||(charged_requirement&&charged_requirement->units&&
  (rac->some_id!=17149||rac->object_type!=12||!Charged32PreparePacking(*user,*charged_requirement,user->GetInv().HasItem(17149,1,invWherePersonal),false,charged_packing)))){
  user->Message(Chat::Red,"This recipe needs its correct container and ingredients.");user->QueuePacket(outapp);safe_delete(outapp);return;
 }

	// Character hasn't learnt the recipe yet.
	// This shouldn't happen.
	if ((spec.must_learn&0xf) && !spec.has_learnt) {
		// Made up message for the client. Just giving a DNC is the other option.
		user->Message(Chat::LightBlue, "You need to learn how to combine these first.");
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	// Character does not have the required skill.
	if (spec.skill_needed > 0 && user->GetSkill(spec.tradeskill) < spec.skill_needed) {
		// Notify client.
		user->Message(Chat::Red, "You are not skilled enough.");
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	if (spec.tradeskill == EQ::skills::SkillAlchemy) {
		if (user->GetClass() != Class::Shaman) {
			user->Message(Chat::Red, "This tradeskill can only be performed by a shaman.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
		else if (user->GetLevel() < MIN_LEVEL_ALCHEMY) {
			user->Message(Chat::Red, "You cannot perform alchemy until you reach level %i.", MIN_LEVEL_ALCHEMY);
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}
	else if (spec.tradeskill == EQ::skills::SkillTinkering) {
		if (user->GetPP().race != Race::Gnome) {
			user->Message(Chat::Red, "Only gnomes can tinker.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}
	else if (spec.tradeskill == EQ::skills::SkillMakePoison) {
		if (user->GetClass() != Class::Rogue) {
			user->Message(Chat::Red, "Only rogues can mix poisons.");
			auto outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			return;
		}
	}

    //pull the list of components
	const auto query = fmt::format("SELECT item_id, componentcount "
                                    "FROM tradeskill_recipe_entries "
                                    "WHERE componentcount > 0 AND recipe_id = {}",
                                    rac->recipe_id);
    auto results = content_db.QueryDatabase(query);
	if (!results.Success()) {
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	if(results.RowCount() < 1) {
		LogError("Error in HandleAutoCombine: no components returned");
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	if(results.RowCount() > 10) {
		LogError("Error in HandleAutoCombine: too many components returned ([{}])", results.RowCount());
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}

	uint32 items[10];
	memset(items, 0, sizeof(items));
	uint8 counts[10];
	memset(counts, 0, sizeof(counts));

	//search for all the items in their inventory
	EQ::InventoryProfile& user_inv = user->GetInv();
	uint8 count = 0;
	uint8 needcount = 0;

	std::list<int> MissingItems;

    uint8 needItemIndex = 0;
	for (auto row = results.begin(); row != results.end(); ++row, ++needItemIndex) {
		uint32 item = (uint32)Strings::ToInt(row[0]);
		uint8 num = (uint8) Strings::ToInt(row[1]);

		needcount += num;

		//because a HasItem on items with num > 1 only returns the
		//last-most slot... the results of this are useless to us
		//when we go to delete them because we cannot assume it is in a single stack.
		if (user_inv.HasItem(item, num, invWherePersonal) != INVALID_INDEX)
			count += num;
		else
			MissingItems.push_back(item);

		//dont start deleting anything until we have found it all.
		items[needItemIndex] = item;
		counts[needItemIndex] = num;
	}

	//make sure we found it all...
	if(count != needcount)
	{
		user->QueuePacket(outapp);

		safe_delete(outapp);

		user->MessageString(Chat::Skills, TRADESKILL_MISSING_COMPONENTS);

		for (auto it = MissingItems.begin(); it != MissingItems.end(); ++it) {
			const EQ::ItemData* item = database.GetItem(*it);

			if(item)
				user->MessageString(Chat::Skills, TRADESKILL_MISSING_ITEM, item->Name);
		}

		return;
	}

	// Check if Combine would result in Lore conflict
	if (!TrophyCertificate55::Find(spec.recipe_id) && user->CheckTradeskillLoreConflict(rac->recipe_id)) {
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}


 if(RechargeRecipe8::Find(spec.recipe_id)) {
  user->Message(Chat::Red,"Use an opened Toolbox to recharge this device.");user->QueuePacket(outapp);safe_delete(outapp);return;
 }
 if(!ogre303_valid()||!final302_valid()||!final300_valid()||!charged_station_valid()||(charged_requirement&&charged_requirement->units&&(results.RowCount()!=1||items[0]!=charged_requirement->input||counts[0]!=charged_requirement->units||!Charged32PackingStillOriginal(*user,*charged_requirement,charged_packing)))){user->QueuePacket(outapp);safe_delete(outapp);return;}
 std::unique_ptr<HousingRecipe301::Use> housing301_use;
 if(HousingRecipe301::Selected(spec.recipe_id)) {
  auto* station=user->GetTradeskillObject();const auto* origin=station?station->HousingRecipe301Origin():nullptr;
  if(!station||!origin||station->user!=user||station->GetType()!=20||rac->object_type!=20||
   !station->m_inst||!station->m_inst->IsClassBag()||station->m_inst->GetID()!=54250||!station->m_inst->GetContents()->empty()||
   uint32_t(spec.tradeskill)!=68||spec.quest||spec.replace_container||spec.onsuccess.size()!=1||
   spec.onsuccess.begin()->first!=HousingRecipe301::Output(spec.recipe_id)||spec.onsuccess.begin()->second!=1||
   !(housing301_use=HousingRecipe301::Use::Acquire(*user,*origin))) {user->QueuePacket(outapp);safe_delete(outapp);return;}
 }

	//now we know they have everything...

 std::unique_ptr<TrophyCertificate55::Proof> trophy_proof;
 if(TrophyCertificate55::Find(spec.recipe_id)) {
  trophy_proof=TrophyCertificate55::Custody::Auto(*user,spec,rac->object_type,rac->some_id);
  if(!trophy_proof){user->QueuePacket(outapp);safe_delete(outapp);return;}
 }
 if(!trophy_proof){
 // Admit this exact source-validated combine once, before any ingredient debit.
 // An authority failure here consumes nothing. After admission, finish the
 // ordinary native combine even if the separate READ connection is lost.
 if(housing301_use&&!housing301_use->AdmitDebit()){user->QueuePacket(outapp);safe_delete(outapp);return;}
	//remove all the items from the players inventory, with updates...
	int16 slot;
	for(uint8 r = 0; r < results.RowCount(); r++) {
		if(items[r] == 0 || counts[r] == 0)
			continue;	//skip empties, could prolly break here

		//we have to loop here to delete 1 at a time in case its in multiple stacks.
		for(uint8 k = 0; k < counts[r]; k++) {
			if(charged_requirement&&charged_requirement->units){
    if(r!=0||k>=charged_packing.count||items[r]!=charged_requirement->input){user->QueuePacket(outapp);safe_delete(outapp);return;}
    slot=charged_packing.takes[k].slot;
   }else slot = user_inv.HasItem(items[r], 1, invWherePersonal);
			if (slot == INVALID_INDEX) {
				//WTF... I just checked this above, but just to be sure...
				//we cant undo the previous deletes without a lot of work.
				//so just call it quits, this shouldent ever happen anyways.
				user->QueuePacket(outapp);
				safe_delete(outapp);
				return;
			}

			const EQ::ItemInstance* inst = user_inv.GetItem(slot);

			if (inst && !inst->IsStackable())
				user->DeleteItemInInventory(slot, 0, true);
			else
				user->DeleteItemInInventory(slot, 1, true);
		}
	}


 if(charged_requirement&&charged_requirement->units){
  if(user_inv.GetItem(charged_packing.kit_slot)!=charged_packing.kit||!charged_packing.kit->GetContents()->empty()){user->QueuePacket(outapp);safe_delete(outapp);return;}
  user->DeleteItemInInventory(charged_packing.kit_slot,0,true);spec.replace_container=false;
 }
 }
 housing301_use.reset(); // release source-only locks before output/task/quest hooks
	//otherwise, we found it all...
	outp->reply_code = 0x00000000;	//success for finding it...
	user->QueuePacket(outapp);
	safe_delete(outapp);


	//now actually try to make something...

 if(trophy_proof)user->CalcBonuses();
 bool success=trophy_proof?TrophyCertificate55::Custody::Complete(*user,std::move(trophy_proof),spec):user->TradeskillExecute(&spec);

	if (success) {
		if (!spec.has_learnt && ((spec.must_learn & 0x10) != 0x10)) {
			user->MessageString(Chat::LightBlue, TRADESKILL_LEARN_RECIPE, spec.name.c_str());
		}
		database.UpdateRecipeMadecount(spec.recipe_id, user->CharacterID(), spec.madecount+1);
	}


	//TODO: find in-pack containers in inventory, make sure they are really
	//there, and then use that slot to handle replace_container too.
	if(success && spec.replace_container) {
//		user->DeleteItemInInventory(in_combine->container_slot, 0, true);
	}

	if (success) {
		if (parse->PlayerHasQuestSub(EVENT_COMBINE_SUCCESS)) {
			parse->EventPlayer(EVENT_COMBINE_SUCCESS, user, spec.name, spec.recipe_id);
		}
	} else {
		if (parse->PlayerHasQuestSub(EVENT_COMBINE_FAILURE)) {
			parse->EventPlayer(EVENT_COMBINE_FAILURE, user, spec.name, spec.recipe_id);
		}
	}
}

void Client::SendTradeskillSearchResults(
	const std::string &query,
	unsigned long objtype,
	unsigned long someid
)
{
	auto results = content_db.QueryDatabase(query);
	if (!results.Success()) {
		return;
	}

	if (results.RowCount() < 1) {
		return;
	}


	auto character_learned_recipe_list = CharRecipeListRepository::GetWhere(
		database,
		fmt::format("char_id = {}", CharacterID())
	);

	for (auto row = results.begin(); row != results.end(); ++row) {
		if (row == nullptr || row[0] == nullptr || row[1] == nullptr || row[2] == nullptr || row[3] == nullptr ||
			row[4] == nullptr || row[5] == nullptr) {
			continue;
		}

		uint32     recipe_id  = (uint32) Strings::ToInt(row[0]);
		const char *name      = row[1];
		uint32     trivial    = (uint32) Strings::ToInt(row[2]);
		uint32     comp_count = (uint32) Strings::ToInt(row[3]);
		uint32     tradeskill = (uint16) Strings::ToInt(row[4]);
		uint32     must_learn = (uint16) Strings::ToInt(row[5]);


		// Skip the recipes that exceed the threshold in skill difference
		// Recipes that have either been made before or were
		// explicitly learned are excempt from that limit
		auto character_learned_recipe = CharRecipeListRepository::GetCharRecipeListEntry(
			character_learned_recipe_list,
			recipe_id
		);

		if (RuleB(Skills, UseLimitTradeskillSearchSkillDiff) &&
			((int32) trivial - (int32) GetSkill((EQ::skills::SkillType) tradeskill)) >
			RuleI(Skills, MaxTradeskillSearchSkillDiff)) {

			LogTradeskills("Checking limit recipe_id [{}] name [{}]", recipe_id, name);

			if (character_learned_recipe.madecount == 0) {
				continue;
			}
		}

		//Skip recipes that must be learned
		if ((must_learn & 0xf) && !character_learned_recipe.recipe_id) {
			continue;
		}


		auto               outapp = new EQApplicationPacket(OP_RecipeReply, sizeof(RecipeReply_Struct));
		RecipeReply_Struct *reply = (RecipeReply_Struct *) outapp->pBuffer;

		reply->object_type     = objtype;
		reply->some_id         = someid;
		reply->component_count = comp_count;
		reply->recipe_id       = recipe_id;
		reply->trivial         = trivial;
		strn0cpy(reply->recipe_name, name, sizeof(reply->recipe_name));
		FastQueuePacket(&outapp);
	}
}

void Client::SendTradeskillDetails(uint32 recipe_id) {

	std::string query = StringFormat("SELECT tre.item_id,tre.componentcount,i.icon,i.Name "
                                    "FROM tradeskill_recipe_entries AS tre "
                                    "LEFT JOIN items AS i ON tre.item_id = i.id "
                                    "WHERE tre.componentcount > 0 AND tre.recipe_id = %u",
                                    recipe_id);
    auto results = content_db.QueryDatabase(query);
	if (!results.Success()) {
		return;
	}

	if(results.RowCount() < 1) {
		LogError("Error in SendTradeskillDetails: no components returned");
		return;
	}

	if(results.RowCount() > 10) {
		LogError("Error in SendTradeskillDetails: too many components returned ([{}])", results.RowCount());
		return;
	}

	//biggest this packet can ever be:
	// 64 * 10 + 8 * 10 + 4 + 4 * 10 = 764
	auto buf = new char[775]; // dynamic so we can just give it to EQApplicationPacket
	uint8 r,k;

	uint32 *header = (uint32 *) buf;
	//Hell if I know why this is in the wrong byte order....
	*header = htonl(recipe_id);

	char *startblock = buf;
	startblock += sizeof(uint32);

	uint32 *ffff_start = (uint32 *) startblock;
	//fill in the FFFF's as if there were 0 items
	for(r = 0; r < 10; r++) { // world:item container size related?
		*ffff_start = 0xFFFFFFFF;
		ffff_start++;
	}
	char * datastart = (char *) ffff_start;
	char * cblock = (char *) ffff_start;

	uint32 *itemptr;
	uint32 *iconptr;
	uint32 len;
	uint32 datalen = 0;
	uint8 count = 0;

	for(auto row = results.begin(); row != results.end(); ++row) {

		//watch for references to items which are not in the
		//items table, which the left join will make nullptr...
		if(row[2] == nullptr || row[3] == nullptr)
			continue;

		uint32 item = (uint32)Strings::ToInt(row[0]);
		uint8 num = (uint8) Strings::ToInt(row[1]);
		uint32 icon = (uint32) Strings::ToInt(row[2]);

		const char *name = row[3];
		len = strlen(name);
		if(len > 63)
			len = 63;

		//Hell if I know why these are in the wrong byte order....
		item = htonl(item);
		icon = htonl(icon);

		//if we get more than 10 items, just start skipping them...
		for(k = 0; k < num && count < 10; k++) { // world:item container size related?
			itemptr = (uint32 *) cblock;
			cblock += sizeof(uint32);
			datalen += sizeof(uint32);
			iconptr = (uint32 *) cblock;
			cblock += sizeof(uint32);
			datalen += sizeof(uint32);

			*itemptr = item;
			*iconptr = icon;
			strncpy(cblock, name, len);

			cblock[len] = '\0';	//just making sure.
			cblock += len + 1;	//get the null
			datalen += len + 1;	//get the null
			count++;
		}

	}

	//now move the item data over top of the FFFFs
	uint8 dist = sizeof(uint32) * (10 - count);
	startblock += dist;
	memmove(startblock, datastart, datalen);

	uint32 total = sizeof(uint32) + dist + datalen;

	// The TOB client reads 4 bytes of trivial after the 10 component slots.
	// Fetch the trivial from the recipe table and append it so the client doesn't read garbage.
	uint32 trivial = 0;
	std::string trivial_query = StringFormat(
		"SELECT trivial FROM tradeskill_recipe WHERE id = %u LIMIT 1", recipe_id);
	auto trivial_results = content_db.QueryDatabase(trivial_query);
	if (trivial_results.Success() && trivial_results.RowCount() > 0) {
		auto trow = trivial_results.begin();
		if (trow[0]) {
			trivial = (uint32)Strings::ToInt(trow[0]);
		}
	}
	uint32 trivial_net = htonl(trivial);
	memcpy(buf + total, &trivial_net, sizeof(uint32));
	total += sizeof(uint32);

	auto outapp = new EQApplicationPacket(OP_RecipeDetails);
	outapp->size = total;
	outapp->pBuffer = (uchar*) buf;
	QueuePacket(outapp);
	DumpPacket(outapp);
	safe_delete(outapp);
}

//returns true on success
bool Client::TradeskillExecute(DBTradeskillRecipe_Struct *spec) {
	if (!spec) {
		return false;
	}

 const bool recharge_selected=RechargeRecipe8::Find(spec->recipe_id)!=nullptr;
 if(recharge_selected&&!RechargeRecipe8::Custody::Claim(*this,*spec))return false;
 const bool trophy_selected=TrophyCertificate55::Find(spec->recipe_id)!=nullptr;
 if(trophy_selected&&!TrophyCertificate55::Custody::Claim(*this,*spec))return false;
	uint16 user_skill = GetSkill(spec->tradeskill);

	if (RuleI(Skills, TradeSkillClamp) != 0 && user_skill > RuleI(Skills, TradeSkillClamp)) {
		user_skill = RuleI(Skills, TradeSkillClamp);
	}

	float chance = 0.0;
	float skillup_modifier = 0.0;
	int16 thirdstat = 0;
	int16 stat_modifier = 15;
	uint16 success_modifier = 0;

	// Rework based on the info on eqtraders.com
	// http://mboards.eqtraders.com/eq/showthread.php?t=22246
	// 09/10/2006 v0.1 (eq4me)
	// 09/11/2006 v0.2 (eq4me)
	// Todo:
	// Implementing AAs
	// Success modifiers based on recipes
	// Skillup modifiers based on the rarity of the ingredients

	// Some tradeskills are more eqal then others. ;-)
	// If you want to customize the stage1 success rate do it here.
	// Remember: skillup_modifier is (float). Lower is better
	switch(spec->tradeskill) {
	case EQ::skills::SkillFletching:
		skillup_modifier = RuleR(Character, TradeskillUpFletching);
		break;
	case EQ::skills::SkillAlchemy:
		skillup_modifier = RuleR(Character, TradeskillUpAlchemy);
		break;
	case EQ::skills::SkillJewelryMaking:
		skillup_modifier = RuleR(Character, TradeskillUpJewelcrafting);
		break;
	case EQ::skills::SkillPottery:
		skillup_modifier = RuleR(Character, TradeskillUpPottery);
		break;
	case EQ::skills::SkillBaking:
		skillup_modifier = RuleR(Character, TradeskillUpBaking);
		break;
	case EQ::skills::SkillBrewing:
		skillup_modifier = RuleR(Character, TradeskillUpBrewing);
		break;
	case EQ::skills::SkillBlacksmithing:
		skillup_modifier = RuleR(Character, TradeskillUpBlacksmithing);
		break;
	case EQ::skills::SkillResearch:
		skillup_modifier = RuleR(Character, TradeskillUpResearch);
		break;
	case EQ::skills::SkillMakePoison:
		skillup_modifier = RuleR(Character, TradeskillUpMakePoison);
		break;
	case EQ::skills::SkillTinkering:
		skillup_modifier = RuleR(Character, TradeskillUpTinkering);
		break;
	case EQ::skills::SkillTailoring:
		skillup_modifier = RuleR(Character, TradeskillUpTailoring);
		break;
	default:
		skillup_modifier = 2;
		break;
	}

	// Some tradeskills take the higher of one additional stat beside INT and WIS
	// to determine the skillup rate. Additionally these tradeskills do not have an
	// -15 modifier on their statbonus.
	if (spec->tradeskill == EQ::skills::SkillFletching || spec->tradeskill == EQ::skills::SkillMakePoison) {
		thirdstat = GetDEX();
		stat_modifier = 0;
	}
	else if (spec->tradeskill == EQ::skills::SkillBlacksmithing) {
		thirdstat = GetSTR();
		stat_modifier = 0;
	}

	int16 higher_from_int_wis = (GetINT() > GetWIS()) ? GetINT() : GetWIS();
	int16 bonusstat = (higher_from_int_wis > thirdstat) ? higher_from_int_wis : thirdstat;

	std::vector< std::pair<uint32,uint8> >::iterator itr;


	//calculate the base success chance
	// For trivials over 68 the chance is (skill - 0.75*trivial) +51.5
	// For trivial up to 68 the chance is (skill - trivial) + 66
	if (spec->trivial >= 68) {
		chance = (user_skill - (0.75*spec->trivial)) + 51.5;
	} else {
		chance = (user_skill - spec->trivial) + 66;
	}

	int16 over_trivial = (int16)GetRawSkill(spec->tradeskill) - (int16)spec->trivial;

	//handle caps
	if(spec->nofail) {
		chance = 100;	//cannot fail.
		LogTradeskills("This combine cannot fail");
	} else if(over_trivial >= 0) {
		// At reaching trivial the chance goes to 95% going up an additional
		// percent for every 40 skillpoints above the trivial.
		// The success rate is not modified through stats.
		// Mastery AAs are unaccounted for so far.
		// chance_AA = chance + ((100 - chance) * mastery_modifier)
		// But the 95% limit with an additional 1% for every 40 skill points
		// above critical still stands.
		// Mastery modifier is: 10%/25%/50% for rank one/two/three
		chance = 95.0f + (float(user_skill - spec->trivial) / 40.0f);

		if (RuleB(Skills, TrivialTradeskillCombinesNoFail)) {
			chance = 100;
		}

		MessageString(Chat::Emote, TRADESKILL_TRIVIAL);
	} else if(chance < 5) {
		// Minimum chance is always 5
		chance = 5;
	} else if(chance > 95) {
		//cap is 95, shouldent reach this before trivial, but just in case.
		chance = 95;
	}

	LogTradeskills("Current skill: [{}] , Trivial: [{}] , Success chance: [{}] percent", user_skill , spec->trivial , chance);
	LogTradeskills("Bonusstat: [{}] , INT: [{}] , WIS: [{}] , DEX: [{}] , STR: [{}]", bonusstat , GetINT() , GetWIS() , GetDEX() , GetSTR());

	float res = zone->random.Real(0, 99);
	int aa_chance = 0;

	aa_chance = spellbonuses.ReduceTradeskillFail[spec->tradeskill] + itembonuses.ReduceTradeskillFail[spec->tradeskill] + aabonuses.ReduceTradeskillFail[spec->tradeskill];

	const EQ::ItemData* item = nullptr;

	if (
		(
			spec->tradeskill == EQ::skills::SkillRemoveTraps ||
			GetGM() ||
			(chance > res)
		) ||
		zone->random.Roll(aa_chance)
	) {
		if (GetGM()) {
			Message(Chat::White, "Your GM flag gives you a 100%% chance to succeed in combining this tradeskill.");
		}

		success_modifier = 1;

		if (over_trivial < 0) {
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, spec->tradeskill);
		}

		MessageString(Chat::LightBlue, TRADESKILL_SUCCEED, spec->name.c_str());

		LogTradeskills("Tradeskill success");

		itr = spec->onsuccess.begin();
		while(itr != spec->onsuccess.end() && !spec->quest) {

			item = database.GetItem(itr->first);
			if (item) {
				CheckItemDiscoverability(itr->first);
				if(!trophy_selected && !recharge_selected) SummonItem(itr->first, FinalRecipe300::SummonCount(spec->recipe_id, itr->first, ChargedRecipe32::SummonCount(spec->recipe_id, itr->first, itr->second)));
				if (GetGroup()) {
					entity_list.MessageGroup(this, true, Chat::Skills, "%s has successfully fashioned %s!", GetName(), item->Name);
				}
			}
			else {
				Log(
					Logs::General,
					Logs::Tradeskills,
					StringFormat(
						"Failure (null item pointer [id: %u, qty: %u]) :: recipe_id:%i tskillid:%i trivial:%i chance:%4.2f  in zoneid:%i instid:%i",
						itr->first,
						itr->second,
						spec->recipe_id,
						spec->tradeskill,
						spec->trivial,
						chance,
						GetZoneID(),
						GetInstanceID()
					).c_str()
				);
			}

			if (RuleB(TaskSystem, EnableTaskSystem)) {
				UpdateTasksForItem(TaskActivityType::TradeSkill, itr->first, itr->second);
			}

			++itr;
		}
		UpdateAchievementForTradeskill(spec->recipe_id);
		return(true);
	}
	/* Tradeskill Fail */
	else {
		success_modifier = 2; // Halves the chance

		if(over_trivial < 0)
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, spec->tradeskill);

		MessageString(Chat::Emote,TRADESKILL_FAILED);

		LogTradeskills("Tradeskill failed");
			if (GetGroup())
		{
			entity_list.MessageGroup(this, true, Chat::Skills,"%s was unsuccessful in %s tradeskill attempt.",GetName(),GetGender() == Gender::Male ? "his" : GetGender() == Gender::Female ? "her" : "its");

		}

		itr = spec->onfail.begin();
		while(itr != spec->onfail.end()) {
			//should we check these arguments?
			SummonItem(itr->first, itr->second);
			++itr;
		}

		/* Salvage Item rolls */

		// Rolls on each item, is possible to return everything
		int SalvageChance = aabonuses.SalvageChance + itembonuses.SalvageChance + spellbonuses.SalvageChance;
		// Skip check if not a normal TS or if a quest recipe these should be nofail, but check amyways
		if(SalvageChance && spec->tradeskill != 75 && !spec->quest) {
			itr = spec->salvage.begin();
			uint8 sc = 0;
			while(itr != spec->salvage.end()) {
				for (sc = 0; sc < itr->second; sc++) {
					if (zone->random.Roll(SalvageChance)) {
						SummonItem(itr->first, 1);
					}
				}
				++itr;
			}
		}

	}
	return(false);
}

void Client::CheckIncreaseTradeskill(int16 bonusstat, int16 stat_modifier, float skillup_modifier, uint16 success_modifier, EQ::skills::SkillType tradeskill)
{
	uint16 current_raw_skill = GetRawSkill(tradeskill);

	if(!CanIncreaseTradeskill(tradeskill))
		return;	//not allowed to go higher.
	uint16 maxskill = MaxSkill(tradeskill);

	float min_skill_up_chance = RuleR(Character, TradeskillUpMinChance);
	min_skill_up_chance = std::max(min_skill_up_chance, 2.5f);

	float chance_stage2 = 0;

	//A successfull combine doubles the stage1 chance for an skillup
	//Some tradeskill are harder than others. See above for more.
	float chance_stage1 = (bonusstat - stat_modifier) / (skillup_modifier * success_modifier);
	chance_stage1 = std::max(min_skill_up_chance, chance_stage1);

	//In stage2 the only thing that matters is your current unmodified skill
	//and the Character:TradeskillUpMinChance rule.
	if (chance_stage1 > zone->random.Real(0, 99)) {
		if (current_raw_skill < 15) {
			//Always succeed
			chance_stage2 = 100;
		} else if (current_raw_skill < 175) {
			//From skill 16 to 174 your chance of success falls linearly from 92% to 13%.
			chance_stage2 = (200 - current_raw_skill) / 2;
		} else {
			//At skill 175, your chance of success falls linearly from 12.5% to 2.5% at skill 300.
			chance_stage2 = 12.5 - (.08 * (current_raw_skill - 175));
		}
		chance_stage2 = std::max(min_skill_up_chance, chance_stage2);
	}

	if (chance_stage2 > zone->random.Real(0, 99)) {
		//Only if stage1 and stage2 succeeded you get a skillup.
		SetSkill(tradeskill, current_raw_skill + 1);
		std::string export_string = fmt::format(
			"{} {} {} {}",
			tradeskill,
			current_raw_skill + 1,
			maxskill,
			1
		);
		parse->EventPlayer(EVENT_SKILL_UP, this, export_string, 0);
		if(title_manager.IsNewTradeSkillTitleAvailable(tradeskill, current_raw_skill + 1))
			NotifyNewTitlesAvailable();
	}

	LogTradeskills("skillup_modifier: [{}] , success_modifier: [{}] , stat modifier: [{}]", skillup_modifier , success_modifier , stat_modifier);
	LogTradeskills("Stage1 chance was: [{}] percent", chance_stage1);
	LogTradeskills("Stage2 chance was: [{}] percent. 0 percent means stage1 failed", chance_stage2);
}

bool ZoneDatabase::GetTradeRecipe(
	const EQ::ItemInstance* container,
	uint8 c_type,
	uint32 some_id,
	Client* c,
	DBTradeskillRecipe_Struct* spec,
	bool* is_augmented
)
{
	if (!container) {
		return false;
	}

	if (!c) {
		return false;
	}

	std::string containers; // make where clause segment for container(s)
	if (!some_id) { // world combiner so no item number
		containers = fmt::format("= {}", c_type);
	} else { // container in inventory
		containers = fmt::format("IN ({}, {})", c_type, some_id);
	}

	//Could prolly watch for stacks in this loop and handle them properly...
	//just increment sum and count accordingly
	bool        first = true;
	std::string buf2;
	uint32      count = 0;
	uint32      sum   = 0;

	for (uint8 slot_id = EQ::invbag::SLOT_BEGIN; slot_id < EQ::invbag::SLOT_COUNT; slot_id++) { // <watch> TODO: need to determine if this is bound to world/item container size
		LogTradeskills("Fetching item [{}]", slot_id);

		const auto inst = container->GetItem(slot_id);
		if (!inst) {
			continue;
		}

		if (inst->IsAugmented()) {
			*is_augmented = true;
			return false;
		}

		const auto item = database.GetItem(inst->GetItem()->ID);
		if (!item) {
			LogTradeskills("item [{}] not found!", inst->GetItem()->ID);
			continue;
		}

		if (first) {
			buf2 += fmt::format("{}", item->ID);
			first = false;
		} else {
			buf2 += fmt::format(", {}", item->ID);
		}

		sum += item->ID;
		count++;

		LogTradeskills(
			"Item in container index [{}] item [{}] found [{}]",
			slot_id,
			item->ID,
			count
		);
	}

	// no items == no recipe
	if (!count) {
		return false;
	}

	std::string query = fmt::format(
		"SELECT tre.recipe_id FROM tradeskill_recipe_entries AS tre "
		"INNER JOIN tradeskill_recipe AS tr ON (tre.recipe_id = tr.id) "
		"WHERE tr.enabled AND ((tre.item_id IN ({}) AND tre.componentcount > 0) "
		"OR (tre.item_id {} AND tre.iscontainer = 1))"
		"GROUP BY tre.recipe_id HAVING SUM(tre.componentcount) = {} "
		"AND SUM(tre.item_id * tre.componentcount) = {}",
		buf2,
		containers,
		count,
		sum
	);

	auto results = QueryDatabase(query);
	if (!results.Success()) {
		LogError("Error in search, query: [{}]", query.c_str());
		LogError("Error in search, error: [{}]", results.ErrorMessage().c_str());
		return false;
	}

	if (results.RowCount() > 1) {
		//multiple recipes, partial match... do an extra query to get it exact.
		//this happens when combining components for a smaller recipe
		//which is completely contained within another recipe
		first = true;
		uint32 index = 0;
		buf2 = "";
		for (auto row : results) {
			const uint32 recipe_id = Strings::ToUnsignedInt(row[0]);
			if (first) {
				buf2 += fmt::format("{}", recipe_id);
				first = false;
			} else {
				buf2 += fmt::format(", {}", recipe_id);
			}

			// length limit on buf2
			if (index == 214) { // Maximum number of recipe matches (19 * 215 = 4096)
				LogError(
					"Warning: Too many matches. Unable to search all recipe entries. Searched [{}] of [{}] possible entries",
					index + 1,
					results.RowCount());
				break;
			}

			++index;
		}

		query = fmt::format(
			"SELECT tre.recipe_id FROM tradeskill_recipe_entries AS tre "
			"WHERE tre.recipe_id IN ({}) GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = {} "
			"AND sum(tre.item_id * tre.componentcount) = {}",
			buf2,
			count,
			sum
		);
		results = QueryDatabase(query);
		if (!results.Success()) {
			LogError("Re-query: [{}]", query.c_str());
			LogError("Error: [{}]", results.ErrorMessage().c_str());
			return false;
		}
	}

	if (results.RowCount() < 1) {
		return false;
	}

	if (results.RowCount() > 1) { //The recipe is not unique, so we need to compare the container were using.
		uint32 container_item_id = 0;

		if (some_id) { // Standard container
			container_item_id = some_id;
		} else if (c_type) { // World container
			container_item_id = c_type;
		} else { // Invalid container
			return false;
		}

		query = fmt::format(
			"SELECT tre.recipe_id FROM tradeskill_recipe_entries AS tre WHERE tre.recipe_id IN ({}) AND tre.item_id = {}",
			buf2,
			container_item_id
		);

		results = QueryDatabase(query);
		if (!results.Success()) {
			LogError("Re-query: [{}]", query);
			LogError("Error: [{}]", results.ErrorMessage());
			return false;
		}

		if (!results.RowCount()) { //Recipe contents matched more than 1 recipe, but not in this container
			LogError("Combine error: Incorrect container is being used!");
			return false;
		}

		if (results.RowCount() > 1) { //Recipe contents matched more than 1 recipe in this container
			LogError(
				"Combine error: Recipe is not unique! [{}] matches found for container [{}]. Continuing with first recipe match",
				results.RowCount(),
				container_item_id
			);
		}
	}

	auto row = results.begin();
	const uint32 recipe_id = Strings::ToUnsignedInt(row[0]);

	//Right here we verify that we actually have ALL of the tradeskill components..
	//instead of part which is possible with experimentation.
	//This is here because something's up with the query above.. it needs to be rethought out
	query = fmt::format(
		"SELECT item_id, componentcount FROM tradeskill_recipe_entries WHERE componentcount > 0 AND recipe_id = {}",
		recipe_id
	);

	results = QueryDatabase(query);
	if (!results.Success() || !results.RowCount()) {
		return GetTradeRecipe(recipe_id, c_type, some_id, c, spec);
	}

	for (auto row : results) {
		int component_count = 0;

		for (uint8 slot_id = EQ::invbag::SLOT_BEGIN; slot_id < EQ::invtype::WORLD_SIZE; slot_id++) {
			const auto inst = container->GetItem(slot_id);
			if (!inst) {
				continue;
			}

			if (inst->IsAugmented()) {
				*is_augmented = true;
				return false;
			}

			const auto item = database.GetItem(inst->GetItem()->ID);
			if (!item) {
				continue;
			}

			if (item->ID == Strings::ToUnsignedInt(row[0])) {
				component_count++;
			}

			LogTradeskills(
				"Component count loop [{}] item [{}] recipe component_count [{}]",
				component_count,
				item->ID,
				Strings::ToInt(row[1])
			);
		}

		if (component_count != Strings::ToInt(row[1])) {
			return false;
		}
	}

	return GetTradeRecipe(recipe_id, c_type, some_id, c, spec);
}

bool ZoneDatabase::GetTradeRecipe(
	uint32 recipe_id,
	uint8 c_type,
	uint32 some_id,
	Client* c,
	DBTradeskillRecipe_Struct* spec
)
{
	if (!c) {
		return false;
	}

	std::string container_where_filter;
	if (!some_id) { // world combiner so no item number
		container_where_filter = fmt::format("= {}", c_type);
	} else { // container in inventory
		container_where_filter = fmt::format("IN ({}, {})", c_type, some_id);
	}

	std::string query = fmt::format(
		SQL(
			SELECT
			tradeskill_recipe.id,
			tradeskill_recipe.tradeskill,
			tradeskill_recipe.skillneeded,
			tradeskill_recipe.trivial,
			tradeskill_recipe.nofail,
			tradeskill_recipe.replace_container,
			tradeskill_recipe.name,
			tradeskill_recipe.must_learn,
			tradeskill_recipe.quest
			FROM
				tradeskill_recipe
				INNER JOIN tradeskill_recipe_entries ON tradeskill_recipe.id = tradeskill_recipe_entries.recipe_id
			WHERE
				tradeskill_recipe.id = {}
				AND tradeskill_recipe_entries.item_id {}
				AND tradeskill_recipe.enabled
				GROUP BY
				tradeskill_recipe.id
		),
		recipe_id,
		container_where_filter
	);
	auto results = QueryDatabase(query);
	if (!results.Success()) {
		LogError("Error, query: [{}]", query.c_str());
		LogError("Error: [{}]", results.ErrorMessage().c_str());
		return false;
	}

	if (!results.RowCount()) {
		return false;
	}

	auto row = results.begin();

	spec->tradeskill        = static_cast<EQ::skills::SkillType>(Strings::ToUnsignedInt(row[1]));
	spec->skill_needed      = Strings::ToInt(row[2]);
	spec->trivial           = Strings::ToUnsignedInt(row[3]);
	spec->nofail            = Strings::ToBool(row[4]);
	spec->replace_container = Strings::ToBool(row[5]);
	spec->name              = row[6];
	spec->must_learn        = Strings::ToUnsignedInt(row[7]);
	spec->quest             = Strings::ToBool(row[8]);
	spec->has_learnt        = false;
	spec->madecount         = 0;
	spec->recipe_id         = recipe_id;

	auto r = CharRecipeListRepository::GetWhere(
		database,
		fmt::format(
			"char_id = {} and recipe_id = {}",
			c->CharacterID(),
			recipe_id
		)
	);

	if (!r.empty() && r[0].recipe_id) { //If this exists we learned it
		LogTradeskills("made_count [{}]", r[0].madecount);

		spec->has_learnt = true;
		spec->madecount  = static_cast<uint32>(r[0].madecount);
	}

	//Pull the on-success items...
	query = fmt::format(
		"SELECT item_id, successcount FROM tradeskill_recipe_entries WHERE successcount > 0 AND recipe_id = {}",
		recipe_id
	);
	results = QueryDatabase(query);
	if (!results.Success()) {
		return false;
	}

	if (!results.RowCount() && !spec->quest) {
		LogError("Error in success: no success items returned");
		return false;
	}

	spec->onsuccess.clear();
	for(auto row : results) {
		const uint32 item_id       = Strings::ToUnsignedInt(row[0]);
		const uint8  success_count = Strings::ToUnsignedInt(row[1]);
		spec->onsuccess.emplace_back(std::pair<uint32, uint8>(item_id, success_count));
	}

	spec->onfail.clear();

	//Pull the on-fail items...
	query = fmt::format(
		"SELECT item_id, failcount FROM tradeskill_recipe_entries WHERE failcount > 0 AND recipe_id = {}",
		recipe_id
	);
	results = QueryDatabase(query);
	if (results.Success()) {
		for (auto row : results) {
			const uint32 item_id    = Strings::ToUnsignedInt(row[0]);
			const uint8  fail_count = Strings::ToUnsignedInt(row[1]);
			spec->onfail.emplace_back(std::pair<uint32, uint8>(item_id, fail_count));
		}
	}

	spec->salvage.clear();

	// Don't bother with the query if TS is nofail
	if (spec->nofail) {
		return true;
	}

	// Pull the salvage list
	query = fmt::format(
		"SELECT item_id, salvagecount FROM tradeskill_recipe_entries WHERE salvagecount > 0 AND recipe_id = {}",
		recipe_id
	);

	results = QueryDatabase(query);
	if (results.Success()) {
		for (auto row : results) {
			const uint32 item_id    = Strings::ToUnsignedInt(row[0]);
			const uint8  salvage_count = Strings::ToUnsignedInt(row[1]);
			spec->salvage.emplace_back(std::pair<uint32, uint8>(item_id, salvage_count));
		}
	}

	return true;
}

void ZoneDatabase::UpdateRecipeMadecount(uint32 recipe_id, uint32 char_id, uint32 madeCount)
{
 if(FinalRecipe300::NeverLearn(recipe_id))return;
	std::string query = StringFormat("INSERT INTO char_recipe_list "
                                    "SET recipe_id = %u, char_id = %u, madecount = %u "
                                    "ON DUPLICATE KEY UPDATE madecount = %u;",
                                    recipe_id, char_id, madeCount, madeCount);
    QueryDatabase(query);
}

void Client::LearnRecipe(uint32 recipe_id)
{
 if(FinalRecipe300::NeverLearn(recipe_id))return;
	std::string query = fmt::format(
		SQL(
			select
			char_id,
			recipe_id,
			madecount
				from
				char_recipe_list
				where
				char_id = {}
				and recipe_id = {}
			LIMIT 1
		),
		CharacterID(),
		recipe_id
	);

	auto results = database.QueryDatabase(query);
	if (!results.Success()) {
		return;
	}

	auto tradeskill_recipe = TradeskillRecipeRepository::FindOne(content_db, recipe_id);
	if (tradeskill_recipe.id == 0) {
		LogError("Invalid recipe [{}]", recipe_id);
		return;
	}

	LogTradeskills(
		"recipe_id [{}] name [{}] learned [{}]",
		recipe_id,
		tradeskill_recipe.name,
		results.RowCount()
	);

	auto row = results.begin();
	if (results.RowCount() > 0) {
		return;
	}

	MessageString(Chat::LightBlue, TRADESKILL_LEARN_RECIPE, tradeskill_recipe.name.c_str());

	database.QueryDatabase(
		fmt::format(
			"REPLACE INTO char_recipe_list (recipe_id, char_id, madecount) VALUES ({}, {}, 0)",
			recipe_id,
			CharacterID()
		)
	);
}

std::vector<uint32> ZoneDatabase::GetRecipeComponentItemIDs(RecipeCountType count_type, uint32 recipe_id)
{
	std::vector<uint32> l;

	const auto& tr = TradeskillRecipeRepository::FindOne(content_db, recipe_id);
	if (!tr.id) {
		return l;
	}

	std::string c;
	switch (count_type) {
		case RecipeCountType::Success:
			c = "successcount";
			break;
		case RecipeCountType::Fail:
			c = "failcount";
			break;
		case RecipeCountType::Component:
			c = "componentcount";
			break;
		case RecipeCountType::Salvage:
			c = "salvagecount";
			break;
		case RecipeCountType::Container:
			c = "iscontainer";
			break;
	}

	const auto& tre = TradeskillRecipeEntriesRepository::GetWhere(
		content_db,
		fmt::format(
			"recipe_id = {} AND {} >= 1 ORDER BY id ASC",
			recipe_id,
			c
		)
	);
	if (tre.empty()) {
		return l;
	}

	for (const auto& e : tre) {
		l.emplace_back(e.item_id);
	}

	return l;
}

int8 ZoneDatabase::GetRecipeComponentCount(RecipeCountType count_type, uint32 recipe_id, uint32 item_id)
{
	const auto& tr = TradeskillRecipeRepository::FindOne(content_db, recipe_id);
	if (!tr.id) {
		return -1;
	}

	const auto& tre = TradeskillRecipeEntriesRepository::GetWhere(
		content_db,
		fmt::format(
			"recipe_id = {} AND item_id = {} ORDER BY id ASC LIMIT 1",
			recipe_id,
			item_id
		)
	);
	if (tre.empty()) {
		return -1;
	}

	switch (count_type) {
		case RecipeCountType::Success:
			return tre[0].successcount;
		case RecipeCountType::Fail:
			return tre[0].failcount;
		case RecipeCountType::Component:
			return tre[0].componentcount;
		case RecipeCountType::Salvage:
			return tre[0].salvagecount;
		default:
			return -1;
	}
}

bool Client::CanIncreaseTradeskill(EQ::skills::SkillType tradeskill) {
	uint32 rawskill = GetRawSkill(tradeskill);
	uint16 maxskill = MaxSkill(tradeskill);

	if (rawskill >= maxskill) //Max skill sanity check
		return false;

	uint8 Baking = (GetRawSkill(EQ::skills::SkillBaking) > 200) ? 1 : 0;
	uint8 Smithing = (GetRawSkill(EQ::skills::SkillBlacksmithing) > 200) ? 1 : 0;
	uint8 Brewing = (GetRawSkill(EQ::skills::SkillBrewing) > 200) ? 1 : 0;
	uint8 Fletching = (GetRawSkill(EQ::skills::SkillFletching) > 200) ? 1 : 0;
	uint8 Jewelry = (GetRawSkill(EQ::skills::SkillJewelryMaking) > 200) ? 1 : 0;
	uint8 Pottery = (GetRawSkill(EQ::skills::SkillPottery) > 200) ? 1 : 0;
	uint8 Tailoring = (GetRawSkill(EQ::skills::SkillTailoring) > 200) ? 1 : 0;
	uint8 SkillTotal = Baking + Smithing + Brewing + Fletching + Jewelry + Pottery + Tailoring; //Tradeskills above 200
	//New Tanaan AA: Each level allows an additional tradeskill above 200 (first one is free)
	uint8 aaLevel = spellbonuses.TradeSkillMastery + itembonuses.TradeSkillMastery + aabonuses.TradeSkillMastery;

	switch (tradeskill) {
	case EQ::skills::SkillBaking:
	case EQ::skills::SkillBlacksmithing:
	case EQ::skills::SkillBrewing:
	case EQ::skills::SkillFletching:
	case EQ::skills::SkillJewelryMaking:
	case EQ::skills::SkillPottery:
	case EQ::skills::SkillTailoring:
		if (aaLevel == 6)
			break; //Maxed AA
		if (SkillTotal == 0)
			break; //First tradeskill freebie
		if ((SkillTotal == (aaLevel + 1)) && (rawskill > 200))
			break; //One of the tradeskills already allowed to go over 200
		if ((SkillTotal >= (aaLevel + 1)) && (rawskill >= 200))
			return false; //One or more tradeskills already at or beyond limit
			break;
	default:
		break; //Other skills unchecked and ability to increase assumed true
	}
	return true;
}

bool ZoneDatabase::EnableRecipe(uint32 recipe_id)
{
	std::string query = StringFormat("UPDATE tradeskill_recipe SET enabled = 1 "
                                    "WHERE id = %u;", recipe_id);
    auto results = QueryDatabase(query);
	if (!results.Success())

	return results.RowsAffected() > 0;

	return false;
}

bool ZoneDatabase::DisableRecipe(uint32 recipe_id)
{
	std::string query = StringFormat("UPDATE tradeskill_recipe SET enabled = 0 "
                                    "WHERE id = %u;", recipe_id);
    auto results = QueryDatabase(query);
	if (!results.Success())

	return results.RowsAffected() > 0;

	return false;
}

bool Client::CheckTradeskillLoreConflict(int32 recipe_id)
{
	auto recipe_entries = TradeskillRecipeEntriesRepository::GetWhere(
		content_db,
		fmt::format(
			"recipe_id = {} ORDER BY componentcount DESC",
			recipe_id
		)
	);
	if (recipe_entries.empty()) {
		return false;
	}

	// validate which items from the recipe we will call CheckLoreConflict on
	for (const auto &tre : recipe_entries) {
		if (tre.item_id) {
			auto tre_inst = database.GetItem(tre.item_id);

			// To compare items we iterate against each item in the recipe that have a loregroup.
			for (auto &tre_update_item : recipe_entries) {
				bool fi_is_valid = tre_update_item.item_id && tre_inst && tre_inst->LoreGroup != 0;

				if (fi_is_valid) {
					auto tre_update_item_inst = database.GetItem(tre_update_item.item_id);
					bool ei_is_valid = tre_update_item_inst && tre_update_item_inst->LoreGroup != 0;

					if (ei_is_valid) {
						bool unique_lore_group_match = tre_inst->LoreGroup > 0 && tre_inst->LoreGroup == tre_update_item_inst->LoreGroup;
						bool component_count_is_valid = tre_update_item.componentcount == 0 && tre.componentcount > 0;

						// If the recipe item is a component, and matches a unique lore group (> 0) or the item_id matches another entry in the recipe
						// zero out the item_id, this will prevent us from doing a lore check inadvertently where
						// the item is a component, and returned on success, fail, salvage.
						// or uses an item that is part of a unique loregroup that returns an item of the same unique loregroup
						if (ei_is_valid && (tre_update_item.item_id == tre.item_id || unique_lore_group_match) && component_count_is_valid) {
							tre_update_item.item_id = 0;
						}
					}
				}
			}

			if (tre_inst) {
				if (tre_inst->LoreGroup == 0 || tre.componentcount > 0 || tre.iscontainer) {
					continue;
				}

				if (CheckLoreConflict(tre_inst)) {
					EQ::SayLinkEngine linker;
					linker.SetLinkType(EQ::saylink::SayLinkItemData);
					linker.SetItemData(tre_inst);
					auto item_link = linker.GenerateLink();
					MessageString(Chat::Red, TRADESKILL_COMBINE_LORE, item_link.c_str());
					return true;
				}
			}
		}
	}

	return false;
}

void Client::ScribeRecipes(uint32_t item_id) const
{
	if (item_id == 0)
	{
		return;
	}

	auto recipes = TradeskillRecipeRepository::GetWhere(content_db, fmt::format(
		"learned_by_item_id = {} {}", item_id, ContentFilterCriteria::apply()));

	std::vector<CharRecipeListRepository::CharRecipeList> learned;
	learned.reserve(recipes.size());

	for (const auto& recipe : recipes)
	{
		auto entry = CharRecipeListRepository::NewEntity();
		entry.char_id = static_cast<int32_t>(CharacterID());
		entry.recipe_id = recipe.id;
		learned.push_back(entry);
	}

	if (!learned.empty())
	{
		// avoid replacing madecount for recipes the client already has
		int rows = CharRecipeListRepository::InsertUpdateMany(database, learned);
		LogTradeskills("Client [{}] scribed [{}] recipes from [{}]", CharacterID(), rows, item_id);
	}
}
