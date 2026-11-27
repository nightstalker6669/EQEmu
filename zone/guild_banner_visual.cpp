#include "guild_banner_visual.h"

#include "client.h"
#include "client_version.h"
#include "entity.h"
#include "guild_banner_store.h"
#include "guild_mgr.h"
#include "npc.h"
#include "zone.h"
#include "zonedb.h"
#include "../common/guild_banner_catalogue.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <string_view>

extern ZoneGuildManager guild_mgr;

namespace {

constexpr char CasterName[] = "Guild_Banner_Aura";
constexpr float EffectRadius = 100.0f;
constexpr int RefreshTicks = 2;

struct Visual {
	GuildBanner::Banner banner;
	NPC* npc{};
	uint16_t entity{};
	std::string token;
};

Zone* owner{};
std::map<uint32_t, Visual> visuals;
std::chrono::steady_clock::time_point next_poll{};

NPC* GetVisual(const Visual& visual)
{
	auto* mob = entity_list.GetMob(visual.entity);
	if (!mob || mob != visual.npc || !mob->IsNPC() ||
		mob->GetEntityVariable("guild_banner_occurrence") != visual.token ||
		mob->CastToNPC()->GetDepop()) {
		return nullptr;
	}
	return mob->CastToNPC();
}

void Despawn(const Visual& visual)
{
	if (auto* npc = GetVisual(visual)) {
		npc->Depop(false);
	}
}

Visual Spawn(const GuildBanner::Banner& banner)
{
	auto type = std::make_unique<NPCType>();
	strn0cpy(type->name, CasterName, sizeof(type->name));
	if (const auto* guild_name = guild_mgr.GetGuildName(banner.guild_id)) {
		strn0cpy(type->lastname, guild_name, sizeof(type->lastname));
	}
	type->race = 553; // GBN banner, stock capture npc_types 700079.
	type->current_hp = type->max_hp = 13;
	type->gender = 2;
	type->class_ = 1;
	type->level = 1;
	type->size = 20;
	type->bodytype = 11;
	type->untargetable = true;
	type->skip_global_loot = true;
	type->skip_auto_scale = true;
	auto npc = std::make_unique<NPC>(
		type.get(), nullptr, glm::vec4(banner.x, banner.y, banner.z, banner.heading),
		GravityBehavior::Flying
	);
	npc->GiveNPCTypeData(type.release());
	npc->AI_Stop();
	npc->Mob::AI_Stop();
	npc->SetInvul(true);
	npc->SetTargetable(false);
	auto token = fmt::format("{}:{}", banner.guild_id, banner.revision);
	npc->SetEntityVariable("guild_banner_occurrence", token);
	entity_list.AddNPC(npc.get(), true, true);
	const auto entity = npc->GetID();
	return Visual{banner, npc.release(), entity, std::move(token)};
}

bool InRange(const Client& client, const GuildBanner::Banner& banner)
{
	const auto displacement = glm::vec3(client.GetPosition()) - glm::vec3(banner.x, banner.y, banner.z);
	return glm::dot(displacement, displacement) <= EffectRadius * EffectRadius;
}

void ReconcileAura(Client& client, const GuildBanner::Definition* definition, NPC* npc)
{
	bool keep = false;
	for (int i = 0; i < client.GetMaxTotalSlots(); ++i) {
		const auto& buff = client.GetBuffs()[i];
		if (!IsGuildBannerAuraBuff(buff.spellid, buff.caster_name, sizeof(buff.caster_name))) {
			continue;
		}
		if (definition && npc && buff.spellid == definition->spell_id &&
			buff.casterid == npc->GetID() && buff.casterlevel == definition->caster_level) {
			keep = true;
			auto& lease = client.GetBuffs()[i];
			lease.ticsremaining = RefreshTicks;
			ClientPatch::SendSingleBuffChange(&client, lease, i);
			lease.UpdateClient = false;
		} else {
			client.BuffFadeBySlot(i);
		}
	}
	if (!definition || !npc || keep || !IsValidSpell(definition->spell_id) ||
		client.FindBuff(definition->spell_id)) {
		return;
	}
	// Native overwrite slots can displace an existing buff even with the
	// overwrite flag disabled. Preserve unrelated spell effects up front.
	for (int i = 0; i < client.GetMaxTotalSlots(); ++i) {
		const auto& existing = client.GetBuffs()[i];
		if (IsValidSpell(existing.spellid) &&
			client.CheckStackConflict(existing.spellid, existing.casterlevel, definition->spell_id,
				definition->caster_level, nullptr, npc, i) != 0) {
			return;
		}
	}
	if (!npc->SpellOnTarget(definition->spell_id, &client, 0, false, 0, false,
		definition->caster_level, RefreshTicks, true)) {
		return;
	}
	for (int i = 0; i < client.GetMaxTotalSlots(); ++i) {
		auto& buff = client.GetBuffs()[i];
		if (buff.spellid == definition->spell_id && buff.casterid == npc->GetID()) {
			strn0cpy(buff.caster_name, CasterName, sizeof(buff.caster_name));
			buff.persistent_buff = false;
			ClientPatch::SendSingleBuffChange(&client, buff, i);
			buff.UpdateClient = false;
		}
	}
}

} // namespace

bool IsGuildBannerAuraBuff(int32_t spell_id, const char* caster, size_t capacity)
{
	if (!caster || capacity < sizeof(CasterName) || std::memcmp(caster, CasterName, sizeof(CasterName)) != 0) {
		return false;
	}
	return std::any_of(
		GuildBanner::UpgradedStandards.begin(), GuildBanner::UpgradedStandards.end(),
		[spell_id](const auto& definition) { return definition.spell_id == spell_id; }
	);
}

void ClearGuildBanners()
{
	for (const auto& [guild, visual] : visuals) {
		Despawn(visual);
	}
	visuals.clear();
	owner = nullptr;
	next_poll = {};
}

void ProcessGuildBanners()
{
	if (!zone || !GuildBanner::SupportedPublicZone(zone->GetZoneID()) ||
		zone->GetInstanceID() || zone->GetInstanceVersion() ||
		!zone->CanCastOutdoor() || !zone->CanDoCombat() || zone->IsPVPZone()) {
		return;
	}
	if (owner != zone) {
		ClearGuildBanners();
		owner = zone;
	}
	const auto clock = std::chrono::steady_clock::now();
	if (clock < next_poll) {
		return;
	}
	next_poll = clock + std::chrono::seconds(2);
	const auto now = static_cast<uint64_t>(std::time(nullptr));
	const auto banners = GuildBanner::FindActiveInZone(database, zone->GetZoneID(), now);
	for (auto current = visuals.begin(); current != visuals.end();) {
		const auto active = std::find_if(banners.begin(), banners.end(), [&](const auto& banner) {
			return current->first == banner.guild_id && current->second.banner.revision == banner.revision;
		});
		if (active == banners.end() || !GetVisual(current->second)) {
			Despawn(current->second);
			current = visuals.erase(current);
		} else {
			current->second.banner = *active;
			++current;
		}
	}
	for (const auto& banner : banners) {
		if (!visuals.contains(banner.guild_id)) {
			visuals.emplace(banner.guild_id, Spawn(banner));
		}
	}
	std::map<uint32_t, uint32_t> present_members;
	for (const auto& [entity, client] : entity_list.GetClientList()) {
		if (!client || !client->Connected() || !client->ClientDataLoaded()) {
			continue;
		}
		const GuildBanner::Definition* definition = nullptr;
		NPC* source = nullptr;
		const auto visual = visuals.find(client->GuildID());
		if (visual != visuals.end() && client->IsInAGuild()) {
			present_members.try_emplace(client->GuildID(), client->CharacterID());
			if (client->GetHP() > 0 && InRange(*client, visual->second.banner)) {
				const auto* candidate = GuildBanner::FindDefinition(visual->second.banner.banner_type);
				if (candidate && candidate->spell_id) {
					definition = candidate;
					source = GetVisual(visual->second);
				}
			}
		}
		ReconcileAura(*client, definition, source);
	}
	for (const auto& [guild, character] : present_members) {
		GuildBanner::RefreshBaseLifetime(database, visuals.at(guild).banner, character, now);
	}
}
