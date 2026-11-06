# Knight first-ten AA delivery

This is an isolated source proposal for the ten automatic Harm Touch / Lay on Hands ranks. Publish these integration artifacts on `rof2-aa`; leave the raw branch source files unchanged. The exact patch target is the integrated source described by the preimage hashes in the manifest. It is not a standalone public-branch compile claim.

## Behavior

The native reconciler runs at the end of ordinary connect and after native SetLevel publishes the new level, for both ordinary and command level changes. It applies only to SoF and later client versions and to the character's actual Paladin or Shadowknight class. Older clients keep their existing innate ability path. No Lua wrapper, global script replacement, rule, database flag flip, or player SQL accompanies the proposal.

It authenticates the twenty known free rank records and first paid boundary, selects the highest eligible free rank at levels 1, 6, 11, 16, 21, 26, 31, 36, 41 and 46, and writes only that class's first-rank ownership row. A character already at or above the eligible count, including a paid-rank or delevelled owner, is left alone. Nonzero charges are preserved. The paid tail is neither traversed nor granted. No AA points or other rows change.

## Durable receipt and uncertainty

A private ordinary Database connection must report autocommit enabled and no transaction. Every query disables replay. An absent row is inserted only when loaded ownership is also zero; a positive loaded rank without its durable row disconnects without a write. An existing row must exactly match loaded ownership and zero charges before a conditional advance. A mismatch disconnects that client without a write. Memory and AA UI publish only after exact committed readback.

Positive free-rank progression differs from Origin's unowned-to-rank-one operation. A lost write acknowledgement could leave an older positive rank in memory. Therefore a per-client flag is set before DML and blocks SaveAA and direct AA-row deletion until committed readback and successful memory publication. On write/readback/publication failure it remains set and the client disconnects. There is no zone freeze, shared-connection contamination, write replay or generic persistence redesign. The next login loads durable ownership normally. The flag has no effect in sessions without this feature's uncertain outcome.

## Integration and installation

1. The coordinator selects the actual integrated target containing the real shroud and fellowship persistence guards. Companion provenance is pinned in the manifest; it is not a selective integration certificate or authorization to import whole features or stub guards.
2. Match the exact five preimage hashes, then integrate only the patch delta. Origin shares the configuration include and AA insertion context; Merc and other owners share headers/connect/level paths. Resolve feature overlaps explicitly and issue new postimage pins when needed. Never overwrite full integrated source files.
3. GitHub owns publication. Build owns compilation and deployment of the reviewed actual integration. No build or runtime action was performed for this proposal. No quest install is needed.
4. Retain exact preimages and current rollback. Apply the reverse patch only against exact postimages, through source/build ownership. Rollback does not delete earned or purchased AA rows.

## Evidence and validation limits

The 2007 conversion contract specifies ten automatic ranks and purchases from level 51. A February 17, 2011 emergency patch, preserved in a March 9 period post, corroborates that both were automatically granted AAs. Exact free IDs, levels and first paid boundary were verified against retained data and read-only current definitions. Generic auto-grant currently skips both abilities because their flags are disabled; enabling those flags would expose a paid tail and is not this repair.

Validation is static source review and corrected-patch applicability against exact active preimages. A temporary exact-preimage forward/reverse round trip passed for the earlier revision; that result is not attributed to the corrected patch hash. Active source is unchanged. No C++ compilation, synthetic behavior test, client action or native acceptance is claimed. Runtime paths and full integrated preimages stay in the private installation map.
