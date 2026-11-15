// Exercise the production legacy message builder used by nearby RoF2 clients.
#include "common/eqemu_config.h"
#include "common/eqemu_logsys.h"
#include "common/eq_packet.h"
#include "common/patches/titanium.h"
#include "common/patches/titanium_structs.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

const EQEmuConfig *Config = nullptr;

static void Check(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static void CheckMessage(const std::string &name, const std::string &spell_link)
{
    Titanium::MessageComponent component;
    auto packet = component.InterruptSpellOther(nullptr, 1234, 5678, name.c_str(), spell_link.c_str());
    using Header = Titanium::structs::InterruptCast_Struct;
    static_assert(sizeof(Header) == 8, "legacy interrupt header must remain eight bytes");
    Check(packet && packet->GetOpcode() == OP_InterruptCast, "wrong interrupt opcode");
    Check(packet->size == sizeof(Header) + name.size() + 1, "wrong legacy payload length");
    const auto *header = reinterpret_cast<const Header *>(packet->pBuffer);
    Check(header->messageid == 1234 && header->spawnid == 5678, "wrong interrupt identity");
    Check(std::memcmp(header->message, name.c_str(), name.size() + 1) == 0,
        "legacy interrupt must carry the caster name and its terminator");
}

int main()
{
    EQEmuLogSys::Instance()->LoadLogSettingsDefaults();
    try {
        // ASAN caught the mercenary's long spell link overflowing this 17-byte
        // packet. Payload identity matters even when the link would fit.
        CheckMessage("Berexiko", std::string(35, 'L'));
        CheckMessage(std::string(63, 'N'), "x");
        CheckMessage("", std::string(35, 'L'));
        CheckMessage("Caster", "");
        std::cout << "InterruptSpellOther: reported mercenary/63-byte name/empty name/empty link PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
