#ifndef EQEMU_XORBB_BINDING_CAST_CONTEXT_H
#define EQEMU_XORBB_BINDING_CAST_CONTEXT_H
#include <cstdint>
class Mob;
namespace XorbbBinding {
// Server-created provenance for the one reviewed Ring of Binding cast. Never
// populated from a Lua item ID or the previous casting_spell_* slot members.
struct ItemCastContext {
    uint32_t character_id = 0;
    uint32_t inventory_slot = UINT32_MAX;
    int32_t item_serial = 0; // exact native ItemInstance serial type
    int32_t casting_slot = -1;
    uint16_t target_id = 0;
    uint32_t target_type = 0;
    uint32_t spawn_point_id = 0;
    const Mob* target_object = nullptr; // identity only; never dereference later
};
}
#endif
