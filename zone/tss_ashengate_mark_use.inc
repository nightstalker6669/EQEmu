// Included by the leased native inventory writer. Debit and its exact durable
// receipt commit together; only known committed state replaces live ownership.
template <typename Validate, typename Record>
static bool SpendAshengateItem(Client& client, int16_t slot, const char* receipt_table,
    Validate validate, Record record) noexcept
{
    auto& inv = client.GetInv();
    auto ticket = client.InventorySessionReleaseTicket();
    auto lease = ticket ? ticket->SnapshotFor(client) : std::nullopt;
    if (!lease || !client.HasInventorySessionReservation() || !client.trade ||
        client.trade->state != TradeNone || client.trade->With() ||
        !FungalInventory::UnknownInventory::ProfileAllowed(&inv)) return false;

    bool unknown = false;
    bool published = false;
    std::unique_ptr<FungalInventory::UnknownInventory> hold;
    try {
        auto* item = inv.GetItem(slot);
        if (!item || item->GetSerialNumber()<=0 ||
            !item->IsClassCommon() || item->IsStackable() || item->IsEvolving() ||
            item->GetEvolveUniqueID() || !item->GetContents()->empty() ||
            std::ranges::any_of(item->GetAugmentIDs(),[](uint32_t id){return id!=0;}) ||
            !validate(*item)) return false;
        const auto who = client.CharacterID();
        const auto serial = item->GetSerialNumber();
        const auto data = item->GetCustomDataString();
        const auto charges = item->GetCharges() >= 0 ? item->GetCharges() : std::numeric_limits<int16_t>::max();
        const auto* cfg = EQEmuConfig::get();
        const auto connect = [cfg](ZoneDatabase& db) {
            return cfg && db.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,
                cfg->DatabaseDB,cfg->DatabasePort,"tss-ashengate-item") && db.TransactionBeginStrict().Success();
        };

        ZoneDatabase db;
        if (!connect(db)) return false;
        struct Rollback { ZoneDatabase& db; ~Rollback() { db.TransactionRollbackStrict(); } } rollback{db};
        LockLease(db,*lease);
        auto engines = Q(db,fmt::format(
            "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
            "AND table_name IN ('inventory','housing_inventory_sessions','housing_live_item_leases','{}')",
            receipt_table));
        if (engines.RowCount()!=1 || N(engines.begin()[0])!=4) return false;
        auto other = Q(db,fmt::format(
            "SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id={} FOR UPDATE",who));
        if (other.RowCount()>1 || (other.RowCount() &&
            (N(other.begin()[0])!=client.AccountID() || (other.begin()[1] && *other.begin()[1]))) ||
            KithicorPoisonCustody::Pending(db,who)) return false;
        const auto expected = fmt::format(
            "character_id={} AND slot_id={} AND item_id={} AND guid={} AND charges={} "
            "AND BINARY COALESCE(custom_data,'')=BINARY '{}'",who,slot,item->GetID(),serial,charges,
            db.Escape(data));
        if (Q(db,"SELECT slot_id FROM inventory WHERE "+expected+" FOR UPDATE").RowCount()!=1) return false;
        const auto pre = Snapshot(db,who);
        if (Q(db,"DELETE FROM inventory WHERE "+expected).RowsAffected()!=1) return false;
        const auto receipt_sql = record(db,*item);
        if (receipt_sql.empty() || db.TransactionStrictFailed() || !ticket->HeldFor(client) ||
            inv.GetItem(slot)!=item || !validate(*item)) return false;
        const auto post = Snapshot(db,who);

        EQApplicationPacket packet(OP_MoveItem,sizeof(MoveItem_Struct));
        auto* change = reinterpret_cast<MoveItem_Struct*>(packet.pBuffer);
        change->from_slot=slot;
        change->to_slot=0xFFFFFFFF;
        change->number_in_stack=0xFFFFFFFF;

        hold.reset(new FungalInventory::UnknownInventory);
        hold->character_=who;
        hold->inventory_=&inv;
        const auto remember = [&](const auto& self,const EQ::ItemInstance* value) -> void {
            if (!value) return;
            hold->items_.push_back(value);
            for (const auto& child : *value->GetContents()) self(self,child.second);
        };
        for (const auto* bucket : {&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})
            for (const auto& entry : *bucket) remember(remember,entry.second);
        for (auto it=inv.cursor_cbegin();it!=inv.cursor_cend();++it) remember(remember,*it);

        unknown = true;
        if (!db.TransactionCommitStrict().Success()) {
            ZoneDatabase verify;
            if (!connect(verify)) throw std::runtime_error("Ashengate item readback unavailable");
            Rollback close{verify};
            LockLease(verify,*lease);
            auto receipt = Q(verify,receipt_sql);
            const auto actual = Snapshot(verify,who);
            if (!receipt.RowCount() && actual==pre && ticket->HeldFor(client)) {
                unknown=false;
                return false;
            }
            if (receipt.RowCount()!=1 || actual!=post || !ticket->HeldFor(client))
                throw std::runtime_error("Ashengate item custody outcome differs");
        }

        std::unique_ptr<EQ::ItemInstance> spent(inv.PopItem(slot));
        if (spent.get()!=item) throw std::runtime_error("Ashengate live item changed");
        published=true;
        unknown=false;
        client.QueuePacket(&packet);
        client.RecalcWeight();
        return true;
    } catch (...) {
        if (unknown) {
            hold->Attach();
            hold.release();
            client.RejectInventorySessionAdmission();
            client.Kick("Ashengate item custody requires inventory reload");
        }
        return published;
    }
}

static bool ConsumeAshengateMark(Client& client, int16_t slot) noexcept
{
    using namespace TssAshengate;
    const auto who=client.CharacterID();
    const auto validate=[&](EQ::ItemInstance& item) {
        return OriginalClickScope::Matches(client,slot) && AshPublic::Token(item.GetCustomData(MarkToken)) &&
            item.GetCustomData(MarkOwner)==std::to_string(who);
    };
    const auto record=[&](ZoneDatabase& db,EQ::ItemInstance& item)->std::string {
        const auto token=item.GetCustomData(MarkToken);
        auto source=Q(db,fmt::format(
            "SELECT m.dz_uuid,m.instance_id,m.event_id FROM tss_ashengate_mark_loot m "
            "JOIN tss_ashengate_event_credit e ON e.dz_uuid=m.dz_uuid AND e.event_id=m.event_id "
            "AND e.character_id=m.character_id AND e.instance_id=m.instance_id AND e.earned_at>0 "
            "WHERE m.character_id={} AND m.source_token='{}' AND m.issued_at>0 FOR UPDATE",who,token));
        if (source.RowCount()!=1 || !source.begin()[0]) return {};
        const std::string uuid(source.begin()[0]);
        const auto instance=N(source.begin()[1]);
        const auto event=N(source.begin()[2]);
        if (uuid.size()!=36 || !instance || event<1 || event>6) return {};
        if (Q(db,fmt::format(
            "SELECT character_id FROM tss_ashengate_marks WHERE character_id={} FOR UPDATE",who)).RowCount())
            return {};
        if (Q(db,fmt::format(
            "INSERT INTO tss_ashengate_marks(character_id,source_token,dz_uuid,instance_id,event_id,consumed_at) "
            "VALUES ({},'{}','{}',{},{},UNIX_TIMESTAMP())",who,token,db.Escape(uuid),instance,event)).RowsAffected()!=1)
            return {};
        return fmt::format(
            "SELECT 1 FROM tss_ashengate_marks WHERE character_id={} AND source_token='{}' "
            "AND dz_uuid='{}' AND instance_id={} AND event_id={} AND consumed_at>0 FOR UPDATE",
            who,token,db.Escape(uuid),instance,event);
    };
    const bool done=SpendAshengateItem(client,slot,"tss_ashengate_marks",validate,record);
    if (done) client.Message(Chat::White,"You have received the Mark of Vergalid's Ashengate recognition.");
    return done;
}

static bool ConsumeAshengateSabotage(Client& client, int16_t slot, uint64_t accepted) noexcept
{
    const auto validate=[&](EQ::ItemInstance& item) {
        return AshPublic::SabotageSite(client) && item.GetID()==AshPublic::Explosives;
    };
    const auto record=[&](ZoneDatabase& db,EQ::ItemInstance& item)->std::string {
        if (AshGive::Accepted(db,client,AshPublic::Sabotage,true)!=accepted ||
            AshPublic::ActionCount(db,client,AshPublic::Sabotage,accepted,4)) return {};
        const auto token=AshPublic::Nonce();
        if (token.empty()) return {};
        const auto who=client.CharacterID();
        if (Q(db,fmt::format(
            "INSERT INTO tss_ashengate_public_actions(source_token,activity_id,character_id,task_id,"
            "accepted_time,placement_id,item_guid,recorded_at) VALUES ('{}',4,{},{},{},0,{},UNIX_TIMESTAMP())",
            token,who,AshPublic::Sabotage,accepted,item.GetSerialNumber())).RowsAffected()!=1) return {};
        return fmt::format(
            "SELECT 1 FROM tss_ashengate_public_actions WHERE source_token='{}' AND activity_id=4 "
            "AND character_id={} AND task_id={} AND accepted_time={} AND item_guid={} FOR UPDATE",
            token,who,AshPublic::Sabotage,accepted,item.GetSerialNumber());
    };
    return SpendAshengateItem(client,slot,"tss_ashengate_public_actions",validate,record);
}
