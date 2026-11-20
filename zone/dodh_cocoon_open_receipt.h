#pragma once

class Mob;
class NPC;

namespace DodhCocoonOpen {

struct OpenIdentity {
    const NPC *target = nullptr;
    const Mob *opener = nullptr;
};

// A scoped witness for the successful native open branch. Pointers are used
// only for identity while native death dispatch is on the same call stack.
class ScopedOpen {
public:
    ScopedOpen(const NPC *target, const Mob *opener) : previous_(current_)
    {
        current_ = {target, opener};
    }

    ScopedOpen(const ScopedOpen &) = delete;
    ScopedOpen &operator=(const ScopedOpen &) = delete;
    ~ScopedOpen() { current_ = previous_; }

    static bool Matches(const NPC *target, const Mob *opener)
    {
        return target && opener && current_.target == target && current_.opener == opener;
    }

private:
    inline static thread_local OpenIdentity current_{};
    OpenIdentity previous_;
};

} // namespace DodhCocoonOpen
