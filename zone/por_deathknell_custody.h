#pragma once

class Client;

namespace PorDeathknellCustody {
enum class Victory { Sullon, Suchun };

// The quest selects the ward/death trigger. Native code binds the actual
// client, expedition, earned receipt, inventory lease and source item.
bool Transform(Client& client, Victory victory) noexcept;
}
