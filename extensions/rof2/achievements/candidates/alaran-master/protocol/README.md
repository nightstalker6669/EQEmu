# Alaran Language Master: existing RoF2 language protocol

This candidate uses existing messages and APIs. It discovers no new opcode and does not establish the historical achievement credit trigger. The helper, data and proposed caller are a single installation bundle; the caller must never be copied alone. All remain uninstalled and disabled pending review.

`OP_SkillUpdate = 0x004c` in the inspected RoF2 mapping. Native `Client::SetLanguageSkill` persists the requested language and queues this message with `skillId = 100 + language_id`. Internal Alaran is 25, so its language slot is **125**, with proficiency in `value`. MacroQuest's language identifier26 is not proficiency.

Native server layout (8 bytes):

```cpp
uint32 skillId; // offset0
uint32 value;   // offset4
```

The existing RoF2 encoder produces 12 bytes:

```cpp
uint32 skillId;   // offset0
uint32 value;     // offset4
uint8 unknown08;  // offset8, encoder writes1
uint8 unknown09;  // offset9, encoder writes80
uint8 unknown10;  // offset10, encoder writes136
uint8 unknown11;  // offset11, encoder writes54
```

These four constants are existing codec behavior, not newly captured historical evidence. Use the existing encoder rather than sending the native 8-byte layout directly.

Direct SetLanguageSkill and IncreaseLanguageSkill do not emit EVENT_LANGUAGE_SKILL_UP. The chat-learning wrapper emits that quest event separately. Consequently, a language-event-only implementation misses direct quest language training. No class-skill25 event is substituted.

The candidate uses existing absolute `SetAchievementProgress(1900049,1,1900049,1,false)` only after verified qualifying interaction and actual native language>=100. The proposed Boolean1 representation is an emulator implementation choice, not a recovered Live numeric counter. The existing setter can return true for deferred updates: this is neither a durable database acknowledgement nor a client completion receipt. Generic login credit, optional-component fabrication and new packet mappings are absent.
