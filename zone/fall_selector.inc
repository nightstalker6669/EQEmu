namespace {

struct FallSelectorView {
    uint32_t task_id = Rof2MonsterTaskSelector::FallOfIllsalin;
    uint64_t occurrence = 0;
    uint32_t character_id = 0;
    bool selection_locked = false;
    std::array<uint32_t, 3> catalogue_opaque_fields{};
    std::array<uint32_t, 2> result_opaque_fields{};
    std::vector<std::shared_ptr<const SpiritShroud::Template>> profiles;
    std::vector<Rof2MonsterTaskWire::Role> roles;
};

void RequestFallRosterRefresh(uint64_t occurrence)
{
    for (const auto& entry : entity_list.GetClientList()) {
        auto* member = entry.second;
        if (!member || member->ClientVersion() != EQ::versions::ClientVersion::RoF2 ||
            member->GetSharedTaskId() <= 0 || static_cast<uint64_t>(member->GetSharedTaskId()) != occurrence) {
            continue;
        }
        const auto* state = member->GetTaskState();
        if (!state || state->GetActiveSharedTask().task_id != Rof2MonsterTaskSelector::FallOfIllsalin) {
            continue;
        }
        ServerPacket packet(ServerOP_SharedTaskRequestMemberlist, sizeof(ServerSharedTaskRequestMemberlist_Struct));
        auto* request = reinterpret_cast<ServerSharedTaskRequestMemberlist_Struct*>(packet.pBuffer);
        request->source_character_id = member->CharacterID();
        request->task_id = Rof2MonsterTaskSelector::FallOfIllsalin;
        worldserver.SendPacket(&packet);
    }
}

class FallSelectorProvider {
public:
    explicit FallSelectorProvider(Client& client) : client_(client) {}

    std::optional<FallSelectorView> CurrentEligibleView()
    {
        const auto* state = client_.GetTaskState();
        const auto run = client_.GetSharedTaskId();
        if (client_.ClientVersion() != EQ::versions::ClientVersion::RoF2 || run <= 0 ||
            !state || state->GetActiveSharedTask().task_id != Rof2MonsterTaskSelector::FallOfIllsalin) {
            return std::nullopt;
        }
        FallSelectorView view;
        view.occurrence = static_cast<uint64_t>(run);
        view.character_id = client_.CharacterID();
        view.profiles = client_.FallShroudProfiles(view.occurrence);
        if (view.profiles.size() != 5) {
            return std::nullopt;
        }
        for (const auto& profile : view.profiles) {
            // No historical minimum or per-role cap was recovered. The
            // isolated stage uses the provider's current eligibility only.
            view.roles.push_back({profile->id, 0, 0, 0, true, profile->name});
        }
        return view;
    }

    struct Details {
        std::array<uint32_t, 4> prefix;
        std::string text;
    };

    std::optional<Details> GetTemplateDetails(const FallSelectorView& view, uint32_t id)
    {
        const auto profile = std::find_if(view.profiles.begin(), view.profiles.end(),
            [id](const auto& p) { return p->id == id; });
        if (profile == view.profiles.end() || !zone) {
            return std::nullopt;
        }
        const auto abilities = SpiritShroud::SelectionAbilities(**profile,
            [](uint32_t rank) { return zone->GetAlternateAdvancementRank(rank); });
        const auto bytes = SpiritShroud::Details(**profile, &abilities);
        constexpr std::size_t ordinary_prefix_size = 8;
        if (bytes.size() <= ordinary_prefix_size + 1 || bytes.back() != 0) {
            return std::nullopt;
        }
        return Details{{0, 0, id, 0},
            std::string(bytes.begin() + ordinary_prefix_size, bytes.end() - 1)};
    }

    Rof2MonsterTaskWire::SelectionResult SelectRole(const FallSelectorView& view, uint32_t id)
    {
        using Result = Rof2MonsterTaskWire::SelectionResult;
        if (client_.FallShroudSelectedTemplate(view.occurrence) == id) {
            return Result::AlreadySelected;
        }
        if (!client_.SelectFallShroudRole(view.occurrence, id)) {
            return Result::Ineligible;
        }
        selected_occurrence_ = view.occurrence;
        return Result::Selected;
    }

    void RefreshRosterAfterSelection() const
    {
        if (selected_occurrence_) {
            RequestFallRosterRefresh(selected_occurrence_);
        }
    }

private:
    Client& client_;
    uint64_t selected_occurrence_ = 0;
};

void SendFallSelectorPacket(Client& client, uint16_t wire, const Rof2MonsterTaskWire::Bytes& body)
{
    using namespace Rof2MonsterTaskWire;
    EmuOpcode opcode;
    switch (wire) {
    case CatalogueOpcode: opcode = OP_FallMonsterCatalogue; break;
    case TemplateDetailsOpcode: opcode = OP_FallMonsterDetails; break;
    case SelectionResultOpcode: opcode = OP_FallMonsterSelectionResult; break;
    default: return;
    }
    EQApplicationPacket packet(opcode, static_cast<uint32_t>(body.size()));
    std::copy(body.begin(), body.end(), packet.pBuffer);
    client.QueuePacket(&packet);
}

} // namespace

void Client::SendFallOfIllsalinCatalogue()
{
    FallSelectorProvider provider(*this);
    Rof2MonsterTaskSelector::SendCatalogue(provider, [&](uint16_t wire, const auto& body) {
        SendFallSelectorPacket(*this, wire, body);
    });
}

void Client::Handle_OP_FallMonsterSelect(const EQApplicationPacket* packet)
{
    if (ClientVersion() != EQ::versions::ClientVersion::RoF2 || !packet) {
        return;
    }
    using namespace Rof2MonsterTaskWire;
    uint16_t wire;
    switch (packet->GetOpcode()) {
    case OP_FallMonsterCatalogue: wire = CatalogueOpcode; break;
    case OP_FallMonsterDetails: wire = TemplateDetailsOpcode; break;
    case OP_FallMonsterSelectRole: wire = SelectRoleOpcode; break;
    default: return;
    }
    FallSelectorProvider provider(*this);
    Rof2MonsterTaskSelector::Dispatch(wire, packet->pBuffer, packet->size, provider,
        [&](uint16_t response, const auto& body) { SendFallSelectorPacket(*this, response, body); });
    provider.RefreshRosterAfterSelection();
}
