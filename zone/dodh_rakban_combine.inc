 // Included in the existing strict inventory writer. As with a committed hail
 // item and its claim, the terminal receipt and native inventory share one tx.
 static void CombineRakban(Client& c, const DodhRakbanCombine::Offer& offer,
                           const DBTradeskillRecipe_Struct& recipe) {
  namespace Rakban = DodhRakbanCombine;
  if (!Rakban::Enabled() || !Rakban::Selected(recipe.recipe_id) || c.IsSpiritShrouded() ||
      c.CharacterID() != offer.character || c.AccountID() != offer.account ||
      static_cast<unsigned>(recipe.tradeskill) != 75 || !recipe.nofail || recipe.replace_container ||
      recipe.quest || recipe.skill_needed || recipe.trivial || recipe.must_learn != 16 ||
      recipe.onsuccess.size() != 1 || recipe.onsuccess.front().first != Rakban::Skull ||
      recipe.onsuccess.front().second != 1 || !recipe.onfail.empty() || !recipe.salvage.empty()) return;

  auto& inv = c.GetInv();
  if (!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&inv) ||
      !c.trade || c.trade->state != TradeNone || c.trade->With() ||
      inv.CursorSize() >= EQ::invbag::CURSOR_BAG_COUNT) return;

  auto ticket = c.InventorySessionReleaseTicket();
  auto lease = ticket ? ticket->SnapshotFor(c) : std::nullopt;
  if (!lease || !c.HasInventorySessionReservation()) return;

  std::unique_ptr<FungalInventory::UnknownInventory> hold;
  bool commit_unknown = false;
  try {
   // The captured offer is re-resolved after the last quest validator.
   const auto parent = inv.m_inv.find(offer.slot);
   if (parent == inv.m_inv.end() || !parent->second ||
       Rakban::Capture(c, offer.slot, *parent->second) != offer) return;

   std::unique_ptr<EQ::ItemInstance> empty_shroud(parent->second->Clone());
   if (!empty_shroud || empty_shroud->GetSerialNumber() != offer.shroud) return;
   empty_shroud->Clear();
   if (!empty_shroud->GetContents()->empty()) return;
   std::unique_ptr<EQ::ItemInstance> skull(database.CreateItem(Rakban::Skull, 1));
   if (!skull || skull->GetID() != Rakban::Skull || skull->GetSerialNumber() <= 0 ||
       skull->GetSerialNumber() == offer.shroud || skull->GetEvolveUniqueID() ||
       c.CheckLoreConflict(skull->GetItem())) return;

   ItemInstQueue staged;
   std::list<EQ::ItemInstance*> before;
   for (auto it = inv.cursor_cbegin(); it != inv.cursor_cend(); ++it) {
    auto* item = *it;
    if (!item || (item->IsClassBag() && !item->GetContents()->empty())) return;
    std::unique_ptr<EQ::ItemInstance> copy(item->Clone());
    if (!copy) return;
    before.push_back(item);
    staged.push(copy.get());
    copy.release();
   }

   auto db_owner = std::make_unique<ZoneDatabase>();
   auto& db = *db_owner;
   const auto* cfg = EQEmuConfig::get();
   const auto connect = [&](ZoneDatabase& connection) {
    return cfg && connection.Connect(cfg->DatabaseHost, cfg->DatabaseUsername, cfg->DatabasePassword,
        cfg->DatabaseDB, cfg->DatabasePort, "dodh-rakban-combine");
   };
   if (!connect(db) || !db.TransactionBeginStrict().Success()) return;
   LockLease(db, *lease);
   const auto who = offer.character;
   auto engines = Q(db,
       "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
       "AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases',"
       "'character_evolving_items','housing_evolving_item_locations','dodh_rakban_claims','dodh_rakban_combines')");
   if (engines.RowCount() != 1 || N(engines.begin()[0]) != 8) return;
   auto housing = Q(db, fmt::format(
       "SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id={} FOR UPDATE", who));
   if (housing.RowCount() > 1 || (housing.RowCount() &&
       (N(housing.begin()[0]) != offer.account || (housing.begin()[1] && *housing.begin()[1])))) return;
   if (KithicorPoisonCustody::Pending(db, who)) return;

   const auto receipt_sql = fmt::format(
       "SELECT recipe_id,shroud_serial,skull_serial FROM dodh_rakban_combines WHERE character_id={} FOR UPDATE", who);
   auto existing = Q(db, receipt_sql);
   if (existing.RowCount()) return; // One original character can never create a second skull here.
   auto claims = Q(db, fmt::format(
       "SELECT route_id,item_id,item_serial FROM dodh_rakban_claims WHERE character_id={} ORDER BY route_id FOR UPDATE", who));
   if (claims.RowCount() != 6) return;
   unsigned route = 0;
   for (auto row = claims.begin(); row != claims.end(); ++row) {
    ++route;
    const auto item = route <= 5 ? 85565 + route : Rakban::Shroud;
    const auto serial = route <= 5 ? offer.fragments[route - 1] : offer.shroud;
    if (N(row[0]) != route || N(row[1]) != item || N(row[2]) != uint64_t(serial)) return;
   }

   std::set<uint32_t> tree_slots;
   Tree(db, who, offer.slot, *parent->second, tree_slots);
   if (tree_slots.size() != 6) return;
   ValidateTrees(db, c, before, false);
   auto raw = InventoryRepository::GetLoadSnapshot(db, who, true);
   if (!raw.success) return;
   const auto skull_serial = skull->GetSerialNumber();
   for (const auto& row : raw.rows) if (row.values.guid == uint64_t(skull_serial)) return;
   const auto pre = Snapshot(db, who);

   // Initial native evolving identity, if this definition uses modern
   // evolution. DoLootChecks uses its global DB and would commit separately.
   if (skull->IsEvolving()) {
    auto evolution = CharacterEvolvingItemsRepository::NewEntity();
    evolution.character_id = who;
    evolution.item_id = Rakban::Skull;
    evolution.final_item_id = EvolvingItemsManager::Instance()->GetFinalItemID(*skull);
    if (evolution.final_item_id != Rakban::FinalSkull) return;
    evolution = CharacterEvolvingItemsRepository::InsertOne(db, evolution);
    if (!evolution.id) return;
    skull->SetEvolveUniqueID(evolution.id);
    skull->SetEvolveCharID(who);
    skull->SetEvolveItemID(Rakban::Skull);
    skull->SetEvolveFinalItemID(Rakban::FinalSkull);
   }

   staged.push(skull.get());
   auto* published_skull = skull.release();
   std::array<std::unique_ptr<EQApplicationPacket>, 5> removals;
   std::array<int16_t, 5> child_slots{};
   for (size_t i = 0; i < offer.cells.size(); ++i) {
    const auto slot = EQ::InventoryProfile::CalcSlotId(offer.slot, offer.cells[i]);
    if (slot < 0 || !tree_slots.count(slot)) return;
    child_slots[i] = slot;
    removals[i] = std::make_unique<EQApplicationPacket>(OP_MoveItem, sizeof(MoveItem_Struct));
    auto* packet = reinterpret_cast<MoveItem_Struct*>(removals[i]->pBuffer);
    packet->from_slot = slot;
    packet->to_slot = 0xffffffff;
    packet->number_in_stack = 0xffffffff;
   }
   std::unique_ptr<EQApplicationPacket> cursor_packet;
   if (before.empty()) {
    const auto bytes = published_skull->Serialize(EQ::invslot::slotCursor);
    cursor_packet = std::make_unique<EQApplicationPacket>(OP_ItemPacket, bytes.size() + sizeof(ItemPacket_Struct));
    auto* packet = reinterpret_cast<ItemPacket_Struct*>(cursor_packet->pBuffer);
    packet->PacketType = ItemPacketLimbo;
    std::memcpy(packet->SerializedItem, bytes.data(), bytes.size());
   }

   hold.reset(new FungalInventory::UnknownInventory);
   hold->character_ = who;
   hold->inventory_ = &inv;
   const auto remember = [&](const auto& self, const EQ::ItemInstance* item) -> void {
    if (!item) return;
    hold->items_.push_back(item);
    for (const auto& child : *item->GetContents()) self(self, child.second);
   };
   for (const auto* bucket : {&inv.m_worn, &inv.m_inv, &inv.m_bank, &inv.m_shbank, &inv.m_trade})
    for (const auto& entry : *bucket) remember(remember, entry.second);
   for (const auto* item : before) remember(remember, item);

   for (const auto slot : child_slots) if (!db.SaveInventory(who, nullptr, slot)) return;
   const auto output_slot = static_cast<int16_t>(before.empty()
       ? EQ::invslot::slotCursor : EQ::invbag::CURSOR_BAG_BEGIN + before.size());
   if (!db.SaveInventory(who, published_skull, output_slot)) return;
   std::set<uint32_t> after_parent;
   Tree(db, who, offer.slot, *empty_shroud, after_parent);
   ValidateTrees(db, c, std::list<EQ::ItemInstance*>(staged.cbegin(), staged.cend()), false);
   auto persisted = InventoryRepository::GetLoadSnapshot(db, who, true);
   if (!persisted.success) return;
   std::map<uint32_t, InventoryRepository::LoadFields> untouched;
   for (const auto& row : raw.rows) {
    const auto slot = row.values.slot_id;
    if (std::find(child_slots.begin(), child_slots.end(), slot) == child_slots.end())
     untouched.emplace(slot, row.fields);
   }
   for (const auto& row : persisted.rows) {
    if (row.values.slot_id == static_cast<uint32_t>(output_slot)) continue;
    auto expected = untouched.find(row.values.slot_id);
    if (expected == untouched.end() || expected->second != row.fields) return;
    untouched.erase(expected);
   }
   if (!untouched.empty()) return;
   auto receipt = Q(db, fmt::format(
       "INSERT INTO dodh_rakban_combines (character_id,recipe_id,shroud_serial,skull_serial) VALUES ({},{},{},{})",
       who, recipe.recipe_id, offer.shroud, skull_serial));
   if (receipt.RowsAffected() != 1 || db.TransactionStrictFailed() || !ticket->HeldFor(c)) return;
   const auto post = Snapshot(db, who);

   commit_unknown = true;
   const bool committed = db.TransactionCommitStrict().Success();
   db_owner.reset();
   if (!committed) {
    ZoneDatabase readback;
    if (!connect(readback) || !readback.TransactionBeginStrict().Success())
     throw std::runtime_error("Rakban combine readback unavailable");
    LockLease(readback, *lease);
    auto durable = Q(readback, receipt_sql);
    const auto actual = Snapshot(readback, who);
    if (!durable.RowCount() && actual == pre) {
     commit_unknown = false;
     return;
    }
    if (durable.RowCount() != 1 || actual != post || !ticket->HeldFor(c))
     throw std::runtime_error("Rakban combine outcome unknown");
    auto row = durable.begin();
    if (N(row[0]) != recipe.recipe_id || N(row[1]) != uint64_t(offer.shroud) ||
        N(row[2]) != uint64_t(skull_serial)) throw std::runtime_error("Rakban combine receipt differs");
   }
   commit_unknown = false;

   // Allocation-free publication of just the empty bag and new cursor queue.
   std::unique_ptr<EQ::ItemInstance> spent_bag(parent->second);
   parent->second = empty_shroud.release();
   inv.m_cursor.Swap(staged);
   for (const auto& packet : removals) c.QueuePacket(packet.get());
   if (cursor_packet) c.QueuePacket(cursor_packet.get());
   c.RecalcWeight();
   c.MessageString(Chat::LightBlue, TRADESKILL_SUCCEED, recipe.name.c_str());
   c.UpdateAchievementForTradeskill(recipe.recipe_id);
  } catch (...) {
   if (!commit_unknown) return;
   hold->Attach();
   hold.release();
   c.RejectInventorySessionAdmission();
   c.Kick("Rakban combine requires inventory reload");
  }
 }
