# CopyCharacter SQL NULL serialization

CopyCharacter previously converted SQL NULL pointers to the string `null` and quoted every value. A copied nullable field could therefore become a literal string or fail strict SQL validation. Preserve actual NULL pointers as unquoted SQL NULL, while retaining escaping and quoting for every nonNULL value, including literal `null`, `NULL` and empty strings. Explicit id, owner, name and account replacements remain nonNULL.

Only the serialization loop and row assembly in `common/database.cpp` change. No journal exclusion or orphan guard, registry, schema, migration, backup, restore or deletion behavior is added by this checkpoint.

Validation: exact public source preimage, minimal forward/reverse patch checks and static review. The public baseline has not been compiled or executed. The separate integrated five-case CopyCharacter fixture is prepared; compilation/runtime status is tracked separately and is not a PASS claim here. Strict SQL NULL/string round trip, ordinary/currency row cloning, destination journal refusal and fresh namespace load require actual execution. No native, concurrent ID allocation, authenticated lifecycle or crash durability claim is made.

This is a source publication checkpoint. No deployment or runtime/schema change is implied.
