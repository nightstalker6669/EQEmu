#ifndef EQEMU_GUILD_BANNER_VISUAL_H
#define EQEMU_GUILD_BANNER_VISUAL_H

#include <cstddef>
#include <cstdint>

void ProcessGuildBanners();
void ClearGuildBanners();
bool IsGuildBannerAuraBuff(int32_t spell_id, const char* caster, size_t capacity);

#endif
