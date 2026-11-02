# EQEmu · RoF2 Spells

Shared inventory/session and native task reward infrastructure, plus the spell content repairs completed through Rain of Fear.

**[Spell content and installation](extensions/rof2/spells/README.md)** · **[Build/schema prerequisites](docs/rof2/INSTALL.md)** · **[Shared task rewards](docs/rof2/SHARED-REWARDS.md)**

This branch inherits `rof2-shared-core` commit `b3f795ea642d810d0af75dca39d6dd1f28826385` on upstream develop `c9c8747ac949deddb140d196d241fa4e777500c7`. Its generic reward engine is already integrated; no achievement manager or achievement quest content is required.

The spell overlay includes summoned-item dependencies, original RoF class vendors, Kerkaz's physical mote quests and optional, clearly labelled pet/aura estimates. It includes sourced VoA merchant stock and original VoA/HoT exchanges. Generic language, exchange policy, durable reward IDs and shutdown cleanup are inherited from the shared core (schema 9338). Read the content guide before applying it. Native mote acceptance and exact Live reconstruction remain unverified.

[Shared infrastructure boundaries](common/shared_core/README.md) · [Upstream documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3](LICENSE.md)
