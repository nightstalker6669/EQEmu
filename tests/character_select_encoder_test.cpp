// Exercise the production encoders and packet ownership, including fortified builds.
#include "common/eqemu_config.h"
#include "common/eqemu_logsys.h"
#include "common/eq_stream_intf.h"
#include "common/patches/character_select.h"
#include "common/patches/rof2.h"
#include "common/patches/rof2_structs.h"
#include "common/patches/rof_structs.h"
#include "common/patches/uf.h"
#include "common/patches/uf_structs.h"
#include "common/patches/sod.h"
#include "common/patches/sod_structs.h"
#include "common/patches/sof.h"
#include "common/patches/sof_structs.h"
#include <iostream>
#include <stdexcept>

const EQEmuConfig *Config;

class CaptureStream final : public EQStreamInterface {
public:
    std::unique_ptr<EQApplicationPacket> packet;
    bool ack = false;
    void FastQueuePacket(EQApplicationPacket **p, bool a) override { packet.reset(*p); *p = nullptr; ack = a; }
    void QueuePacket(const EQApplicationPacket *, bool) override { throw std::runtime_error("unexpected copy queue"); }
    EQApplicationPacket *PopPacket() override { return nullptr; }
    void Close() override {}
    void ReleaseFromUse() override {}
    void RemoveData() override {}
    std::string GetRemoteAddr() const override { return {}; }
    uint32 GetRemoteIP() const override { return 0; }
    uint16 GetRemotePort() const override { return 0; }
    bool CheckState(EQStreamState) override { return true; }
    std::string Describe() const override { return "encoder regression"; }
    EQStreamState GetState() override { return ESTABLISHED; }
    void SetOpcodeManager(OpcodeManager **) override {}
    OpcodeManager *GetOpcodeManager() const override { return nullptr; }
    Stats GetStats() const override { return {}; }
    void ResetStats() override {}
    EQStreamManagerInterface *GetManager() const override { return nullptr; }
};

static void Check(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}

static EQApplicationPacket *Input(const std::vector<std::string> &names) {
    auto *p = new EQApplicationPacket(OP_SendCharInfo,
        sizeof(CharacterSelect_Struct) + names.size() * sizeof(CharacterSelectEntry_Struct));
    auto *header = reinterpret_cast<CharacterSelect_Struct *>(p->pBuffer);
    header->CharCount = names.size(); header->TotalChars = 12;
    for (size_t i = 0; i < names.size(); ++i) {
        auto &entry = header->Entries[i];
        std::memcpy(entry.Name, names[i].c_str(), names[i].size() + 1);
        entry.Class = 3 + i; entry.Race = 128 + i; entry.Level = 20 + i;
        entry.Zone = 394 + i; entry.Instance = 10 + i;
        entry.Gender = 1; entry.HairStyle = 7; entry.Beard = 4;
        entry.Equip[0].Material = 1000 + i;
        entry.Equip[8].Color = 0xdeadbeef;
        entry.DrakkinDetails = 700 + i;
        entry.LastLogin = 123456789 + i; entry.Unknown2 = 17;
    }
    return p;
}

template<class Strategy, class Header, class Entry, size_t Limit>
struct Exercise : Strategy {
    static void Run(const char *label) {
        const std::vector<std::vector<std::string>> cases = {
            {}, {"Uena"}, {std::string(63, 'M')}, {"Uena", std::string(63, 'N'), "Qaaccept"},
            std::vector<std::string>(Limit + 1, "Captest")
        };
        for (const auto &names : cases) {
            auto *p = Input(names);
            auto stream = std::make_shared<CaptureStream>();
            Strategy::Encode_OP_SendCharInfo(&p, stream, true);
            Check(!p && stream->packet && stream->ack, "ownership / queue failure");
            const auto count = std::min(names.size(), Limit);
            auto *header = reinterpret_cast<const Header *>(stream->packet->pBuffer);
            Check(header->CharCount == count, "wire count mismatch");
            size_t offset = sizeof(Header);
            for (size_t i = 0; i < count; ++i) {
                const auto *base = stream->packet->pBuffer + offset;
                Check(std::memcmp(base + offsetof(Entry, Name), names[i].c_str(), names[i].size() + 1) == 0,
                    "name or NUL mismatch");
                const auto *entry = reinterpret_cast<const Entry *>(base + names[i].size());
                Check(entry->Class == 3 + i && entry->Race == 128 + i && entry->Zone == 394 + i,
                    "fields after name shifted");
                Check(entry->Beard == 4 && entry->Equip[0].Material == 1000 + i && entry->Equip[8].Color == 0xdeadbeef &&
                    entry->DrakkinDetails == 700 + i, "appearance fields shifted");
                const auto *prefix = reinterpret_cast<const Entry *>(base);
                if constexpr (offsetof(Entry, Level) < offsetof(Entry, Name)) {
                    Check(prefix->Level == 20 + i && prefix->HairStyle == 7 && prefix->Gender == 1,
                        "fields before name shifted");
                } else {
                    Check(entry->Level == 20 + i, "level shifted");
                }
                if constexpr (requires { entry->LastLogin; }) {
                    Check(entry->Instance == 10 + i && entry->LastLogin == 123456789 + i && entry->Unknown2 == 17, "trailing fields shifted");
                }
                offset += sizeof(Entry) + names[i].size();
            }
            Check(offset == stream->packet->size, "wire length mismatch");
        }
        // Every malformed input must be consumed without enqueuing a packet.
        for (int mode = 0; mode < 4; ++mode) {
            auto *p = Input({"Uena"});
            auto *header = reinterpret_cast<CharacterSelect_Struct *>(p->pBuffer);
            if (mode == 0) p->size = sizeof(CharacterSelect_Struct) - 1;
            if (mode == 1) --p->size;
            if (mode == 2) header->CharCount = UINT32_MAX;
            if (mode == 3) std::memset(header->Entries[0].Name, 'X', sizeof(header->Entries[0].Name));
            auto stream = std::make_shared<CaptureStream>();
            Strategy::Encode_OP_SendCharInfo(&p, stream, false);
            Check(!p && !stream->packet, "malformed packet accepted");
        }
        std::cout << label << ": ordinary/max63/multiple/empty/cap/malformed PASS\n";
    }
};

int main() {
    EQEmuLogSys::Instance()->LoadLogSettingsDefaults();
    try {
        Exercise<RoF2::Strategy, RoF2::structs::CharacterSelect_Struct, RoF2::structs::CharacterSelectEntry_Struct,
            RoF2::constants::CHARACTER_CREATION_LIMIT>::Run("RoF2");
        Exercise<RoF::Strategy, RoF::structs::CharacterSelect_Struct, RoF::structs::CharacterSelectEntry_Struct,
            RoF::constants::CHARACTER_CREATION_LIMIT>::Run("RoF");
        Exercise<UF::Strategy, UF::structs::CharacterSelect_Struct, UF::structs::CharacterSelectEntry_Struct,
            UF::constants::CHARACTER_CREATION_LIMIT>::Run("UF");
        Exercise<SoD::Strategy, SoD::structs::CharacterSelect_Struct, SoD::structs::CharacterSelectEntry_Struct,
            SoD::constants::CHARACTER_CREATION_LIMIT>::Run("SoD");
        Exercise<SoF::Strategy, SoF::structs::CharacterSelect_Struct, SoF::structs::CharacterSelectEntry_Struct,
            SoF::constants::CHARACTER_CREATION_LIMIT>::Run("SoF");
        auto *p = Input({"Uena"});
        std::unique_ptr<EQApplicationPacket> owner(p);
        Check(!EQ::patches::GetCharacterSelectLayout(p->pBuffer, p->size, 1, UINT32_MAX, 1), "output overflow accepted");
        Check(!EQ::patches::GetCharacterSelectLayout(p->pBuffer, p->size, 1, 1, UINT32_MAX), "entry overflow accepted");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
    return 0;
}
