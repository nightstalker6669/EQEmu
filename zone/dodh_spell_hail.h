#pragma once

class Client;
class NPC;

namespace DodhSpellHail {
// Returns true only when the session was fenced and dispatch must stop.
bool OnTargetedSay(Client&, NPC&, const char*) noexcept;
}
