# Known gaps and evidence limits

This release implements functional local RoF2 fellowships, with the following explicit limits:

1. **Housing dependency:** paid camp inventory safety and property permissions require the exact companion housing baseline. A standalone stock-upstream version has not been separated or tested.
2. **Live gameplay policy:** radius100, deterministic maximum focus, yellow-lumber melee tiers and XP/cap/cross-level conversion are emulator policies. Separate normal and AA banks do not establish Live cross-channel conversion.
3. **Camp timer:** the client consumes a nonzero timestamp and zone for presence. The original clock origin, second timestamp and complete timer semantics are unresolved. Server epoch expiry is intentionally used as presence; server expiry governs actual effects.
4. **Action unknowns:** action+0x10 carries a native value with unresolved semantics; it is not trusted as authority. Reserved action bytes are zero/not interpreted. Native accept/decline lacks a nonce, so security binds to the offer actually delivered to the current authenticated connection.
5. **Roster unknowns:** fellowship name, player handles, offline-mode flags and exact Sync semantics are not fabricated. Version5 and Sync1 follow native constructor defaults. Member0 supplies leader controls. These bytes are documented separately from implemented fields.
6. **Vitality shape:** native consumer proves64-bit normal fields and low32-bit AA reads at16/24. High AA words and exact minimum accepted size are unproven; server emits a zero-padded32-byte packet.
7. **Content/stacking:** mismatched Standard spell signatures disable affected recipes; ordinary buff conflicts can prevent a camp lease. Ordinary stat caps and haste/damage-shield rules still apply. The package does not supply client data or guarantee an arbitrary PEQ vendor inventory.
8. **Coverage:** native gameplay sampled representative camps and three members; all 597 catalogue definitions were checked programmatically. Twelve-member capacity and failure/concurrency cases have service-level coverage, not a twelve-client physical session.
9. **Other clients/custom hooks:** native UI targets this RoF2 build. Cross-client fallback is chat text only. Custom independently durable quest effects fall outside the XP transaction contract.

No unknown opcode candidate is promoted to a confirmed mapping merely because it appears near a fellowship dispatcher case. This release maps the six proven fellowship messages and uses existing general protocols for chat, buffs, spawns and travel.
