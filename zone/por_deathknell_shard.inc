// Included by the leased inventory writer so validated roots can be published
// in memory without a second database save.
static bool TransformDeathknellShard(Client& client, PorDeathknellCustody::Victory victory) noexcept
{
    using Victory = PorDeathknellCustody::Victory;
    const bool sullon = victory == Victory::Sullon;
    const auto* stage = sullon ? "sullon" : "suchun";
    const auto* name = sullon ? "Sullon Zek" : "Suchun the Blood Warden";
    const uint32_t zone_id = sullon ? 375 : 377;
    const uint32_t input_id = sullon ? 84164 : 84166;
    auto& inventory = client.GetInv();
    std::unique_ptr<FungalInventory::UnknownInventory> hold;
    bool commit_unknown = false;
    bool published = false;

    try {
        auto* expedition = zone ? zone->GetDynamicZone() : nullptr;
        const uint32_t character = client.CharacterID();
        if (!expedition || !expedition->IsValid() || expedition->IsExpired() ||
            expedition->GetType() != DynamicZoneType::Expedition || expedition->GetName() != name ||
            expedition->GetZoneID() != zone_id || expedition->GetZoneVersion() != 20 ||
            zone->GetZoneID() != zone_id || zone->GetInstanceVersion() != 20 ||
            zone->GetInstanceID() != expedition->GetInstanceID() || expedition->GetUUID().size() != 36 ||
            !expedition->HasMember(character) || entity_list.GetClientByCharID(character) != &client ||
            client.IsSpiritShrouded() || !FungalInventory::UnknownInventory::ProfileAllowed(&inventory) ||
            !client.trade || client.trade->state != TradeNone || client.trade->With() ||
            !inventory.m_trade.empty()) return false;

        auto ticket = client.InventorySessionReleaseTicket();
        auto lease = ticket ? ticket->SnapshotFor(client) : std::nullopt;
        if (!lease || !client.HasInventorySessionReservation() ||
            lease->owner.zone_id != zone_id || lease->owner.instance_id != expedition->GetInstanceID()) return false;

        EQ::ItemInstance* input = nullptr;
        int16_t slot = EQ::invslot::SLOT_INVALID;
        const auto locate = [&](int16_t candidate) {
            auto* item = inventory.GetItem(candidate);
            if (!item || item->GetID() != input_id) return true;
            if (input) return false;
            input = item;
            slot = candidate;
            return true;
        };
        for (int16_t top = EQ::invslot::GENERAL_BEGIN; top <= EQ::invslot::GENERAL_END; ++top) {
            if (!locate(top)) return false;
            auto* bag = inventory.GetItem(top);
            if (!bag || !bag->IsClassBag()) continue;
            for (const auto& entry : *bag->GetContents())
                if (!locate(EQ::InventoryProfile::CalcSlotId(top, entry.first))) return false;
        }
        if (!locate(EQ::invslot::slotCursor) || (sullon && !input)) return false;
        if (input) {
            if (input->GetSerialNumber() <= 0 || input->IsStackable() || input->IsClassBag() ||
                input->IsEvolving() || input->GetEvolveUniqueID() || !input->GetContents()->empty()) return false;
            for (const auto augment : input->GetAugmentIDs()) if (augment) return false;
        }
        const uint64_t input_guid = input ? input->GetSerialNumber() : 0;
        const bool vial = !input;
        const uint32_t output_id = sullon ? 84165 : vial ? 84242 : 84167;

        std::unique_ptr<EQ::ItemInstance> output(database.CreateItem(output_id));
        if (!output || output->GetID() != output_id || output->GetSerialNumber() <= 0 ||
            uint64_t(output->GetSerialNumber()) == input_guid || output->IsStackable() ||
            output->IsClassBag() || output->IsEvolving() || output->GetEvolveUniqueID() ||
            client.CheckLoreConflict(output->GetItem())) return false;
        const uint64_t output_guid = output->GetSerialNumber();
        auto* replacement = output.get();
        const bool cursor = vial || slot == EQ::invslot::slotCursor;
        const bool in_bag = !cursor && slot >= EQ::invbag::GENERAL_BAGS_BEGIN;
        const auto root_slot = in_bag ? EQ::InventoryProfile::CalcSlotId(slot) : slot;
        auto root = inventory.m_inv.find(root_slot);
        std::unique_ptr<EQ::ItemInstance> staged_root;
        ItemInstQueue staged_cursor;
        std::list<EQ::ItemInstance*> prior_cursor;
        if (cursor) {
            if (vial && inventory.CursorSize() >= EQ::invbag::CURSOR_BAG_COUNT) return false;
            for (auto it = inventory.cursor_cbegin(); it != inventory.cursor_cend(); ++it) {
                auto* item = *it;
                if (!item || (item->IsClassBag() && !item->GetContents()->empty())) return false;
                prior_cursor.push_back(item);
                if (item == input) {
                    staged_cursor.push(replacement);
                    output.release();
                    continue;
                }
                if (item->GetID() == input_id) return false;
                std::unique_ptr<EQ::ItemInstance> clone(item->Clone());
                if (!clone) return false;
                staged_cursor.push(clone.get());
                clone.release();
            }
            if (vial) {
                slot = prior_cursor.empty() ? EQ::invslot::slotCursor
                    : EQ::invbag::CURSOR_BAG_BEGIN + prior_cursor.size();
                staged_cursor.push(replacement);
                output.release();
            } else if (prior_cursor.empty() || prior_cursor.front() != input) {
                return false;
            }
        } else if (root == inventory.m_inv.end() || !root->second) {
            return false;
        } else if (in_bag) {
            staged_root.reset(root->second->Clone());
            if (!staged_root) return false;
            const auto index = EQ::InventoryProfile::CalcBagIdx(slot);
            staged_root->PutItem(index, *output);
            replacement = staged_root->GetItem(index);
            if (!replacement || uint64_t(replacement->GetSerialNumber()) != output_guid) return false;
        } else {
            staged_root = std::move(output);
        }

        std::unique_ptr<EQApplicationPacket> packet;
        if (!vial || prior_cursor.empty()) {
            const auto bytes = replacement->Serialize(slot);
            packet = std::make_unique<EQApplicationPacket>(OP_ItemPacket, bytes.size() + sizeof(ItemPacket_Struct));
            auto* wire = reinterpret_cast<ItemPacket_Struct*>(packet->pBuffer);
            wire->PacketType = cursor ? ItemPacketLimbo : ItemPacketTrade;
            std::memcpy(wire->SerializedItem, bytes.data(), bytes.size());
        }

        auto connection = std::make_unique<ZoneDatabase>();
        const auto* cfg = EQEmuConfig::get();
        const auto connect = [&](ZoneDatabase& db) {
            return cfg && db.Connect(cfg->DatabaseHost, cfg->DatabaseUsername, cfg->DatabasePassword,
                cfg->DatabaseDB, cfg->DatabasePort, "por-deathknell-shard") && db.TransactionBeginStrict().Success();
        };
        if (!connect(*connection)) return false;
        auto& db = *connection;
        LockLease(db, *lease);
        auto engines = Q(db,
            "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
            "AND table_name IN ('inventory','housing_inventory_sessions','housing_live_item_leases',"
            "'por_deathknell_raid_credit','dynamic_zones','dynamic_zone_members','instance_list')");
        if (engines.RowCount() != 1 || N(engines.begin()[0]) != 7) return false;
        auto housing = Q(db, fmt::format(
            "SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id={} FOR UPDATE",
            character));
        if (housing.RowCount() > 1 || (housing.RowCount() &&
            (N(housing.begin()[0]) != client.AccountID() || (housing.begin()[1] && *housing.begin()[1]))) ||
            KithicorPoisonCustody::Pending(db, character)) return false;

        const auto credit_key = fmt::format("dz_uuid='{}' AND stage='{}' AND character_id={} AND instance_id={}",
            db.Escape(expedition->GetUUID()), stage, character, expedition->GetInstanceID());
        const auto receipt_sql = "SELECT input_guid,output_guid FROM por_deathknell_raid_credit WHERE " +
            credit_key + " FOR UPDATE";
        const auto member = Q(db, fmt::format(
            "SELECT d.id FROM dynamic_zones d JOIN dynamic_zone_members m ON m.dynamic_zone_id=d.id "
            "JOIN instance_list i ON i.id=d.instance_id WHERE d.id={} AND d.uuid='{}' AND d.type={} "
            "AND BINARY d.name='{}' AND i.id={} AND i.zone={} AND i.version=20 "
            "AND (i.never_expires=1 OR i.expire_at>UNIX_TIMESTAMP()) AND m.character_id={} FOR UPDATE",
            expedition->GetID(), db.Escape(expedition->GetUUID()), uint32_t(DynamicZoneType::Expedition),
            name, expedition->GetInstanceID(), zone_id, character));
        if (member.RowCount() != 1) return false;
        auto credit = Q(db, receipt_sql);
        if (credit.RowCount() != 1 || credit.begin()[0] || credit.begin()[1]) return false;
        if (vial) {
            if (Q(db, fmt::format(
                "SELECT dz_uuid FROM por_deathknell_raid_credit WHERE character_id={} AND stage='suchun' "
                "AND (input_guid IS NOT NULL OR output_guid IS NOT NULL) LIMIT 1 FOR UPDATE",
                character)).RowCount()) return false;
            if (Q(db, fmt::format(
                "SELECT request_key FROM kithicor_poison_custody WHERE character_id={} AND recipe_kind=4 "
                "AND recipe_context IN (x'{}',x'{}') AND npc_type_id={} "
                "AND npc_spawn_id={} AND physical=1 AND abort_only=0 LIMIT 1 FOR UPDATE",
                character, Hex(PorChime::Encode(PorChime::VialExchange)),
                Hex(PorChime::Encode(PorChime::FettersExchange)), PorChime::ShrineNpc,
                PorChime::ShrineSpawn)).RowCount()) return false;
        }
        std::set<uint32_t> tree_slots;
        if (cursor) ValidateTrees(db, client, prior_cursor, false);
        else Tree(db, character, root_slot, *root->second, tree_slots);
        const auto raw = InventoryRepository::GetLoadSnapshot(db, character, true);
        if (!raw.success) return false;
        std::map<uint32_t, InventoryRepository::LoadFields> untouched;
        for (const auto& row : raw.rows) {
            if (row.values.guid == output_guid) return false;
            if (vial && (row.values.item_id == 84242 || row.values.item_id == 84167 ||
                        row.values.item_id == 52601 || row.values.slot_id == uint32_t(slot))) return false;
            if (row.values.slot_id != uint32_t(slot)) {
                if (input_guid && row.values.guid == input_guid) return false;
                untouched.emplace(row.values.slot_id, row.fields);
            }
        }

        hold.reset(new FungalInventory::UnknownInventory);
        hold->character_ = character;
        hold->inventory_ = &inventory;
        const auto remember = [&](const auto& self, const EQ::ItemInstance* item) -> void {
            if (!item) return;
            hold->items_.push_back(item);
            for (const auto& child : *item->GetContents()) self(self, child.second);
        };
        for (const auto* bucket : {&inventory.m_worn, &inventory.m_inv, &inventory.m_bank,
                                  &inventory.m_shbank, &inventory.m_trade})
            for (const auto& entry : *bucket) remember(remember, entry.second);
        for (auto it = inventory.cursor_cbegin(); it != inventory.cursor_cend(); ++it) remember(remember, *it);

        const auto pre = Snapshot(db, character);
        if (cursor) {
            auto first = staged_cursor.cbegin(), last = staged_cursor.cend();
            if (!db.SaveCursor(character, first, last)) return false;
            ValidateTrees(db, client, std::list<EQ::ItemInstance*>(staged_cursor.cbegin(), staged_cursor.cend()), false);
        } else {
            if (!db.SaveInventory(character, replacement, slot)) return false;
            tree_slots.clear();
            Tree(db, character, root_slot, *staged_root, tree_slots);
        }
        const auto persisted = InventoryRepository::GetLoadSnapshot(db, character, true);
        if (!persisted.success) return false;
        bool replaced = false;
        for (const auto& row : persisted.rows) {
            if (row.values.slot_id == uint32_t(slot)) {
                if (row.values.item_id != output_id || row.values.guid != output_guid) return false;
                replaced = true;
                continue;
            }
            const auto old = untouched.find(row.values.slot_id);
            if (old == untouched.end() || old->second != row.fields) return false;
            untouched.erase(old);
        }
        if (!replaced || !untouched.empty()) return false;
        const auto saved = Q(db, fmt::format(
            "UPDATE por_deathknell_raid_credit SET input_guid={},output_guid={} WHERE {} "
            "AND input_guid IS NULL AND output_guid IS NULL",
            vial ? "NULL" : std::to_string(input_guid), output_guid, credit_key));
        if (saved.RowsAffected() != 1 || db.TransactionStrictFailed() || !ticket->HeldFor(client)) return false;
        const auto post = Snapshot(db, character);

        commit_unknown = true;
        const bool committed = db.TransactionCommitStrict().Success();
        connection.reset();
        if (!committed) {
            ZoneDatabase readback;
            if (!connect(readback)) throw std::runtime_error("Deathknell shard outcome readback unavailable");
            LockLease(readback, *lease);
            auto receipt = Q(readback, receipt_sql);
            const auto actual = Snapshot(readback, character);
            if (receipt.RowCount() == 1 && !receipt.begin()[0] && !receipt.begin()[1] && actual == pre &&
                ticket->HeldFor(client)) {
                commit_unknown = false;
                return false;
            }
            if (receipt.RowCount() != 1 || (vial ? receipt.begin()[0] != nullptr : N(receipt.begin()[0]) != input_guid) ||
                N(receipt.begin()[1]) != output_guid || actual != post || !ticket->HeldFor(client))
                throw std::runtime_error("Deathknell shard outcome differs");
        }

        if (cursor) {
            inventory.m_cursor.Swap(staged_cursor);
        } else {
            std::unique_ptr<EQ::ItemInstance> retired(root->second);
            root->second = staged_root.release();
        }
        commit_unknown = false;
        published = true;
        if (packet) client.QueuePacket(packet.get());
        client.RecalcWeight();
        return true;
    } catch (...) {
        if (commit_unknown) {
            hold->Attach();
            hold.release();
            client.RejectInventorySessionAdmission();
            client.Kick("Deathknell shard custody requires inventory reload");
        }
        return published;
    }
}
