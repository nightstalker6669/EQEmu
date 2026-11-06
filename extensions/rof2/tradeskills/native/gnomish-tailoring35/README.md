# Gnomish tailoring actor source

Exact RRC169 recipe IDs3095953–3095987 only. Apply source.patch against the pinned native zone/tradeskills.cpp preimage; add zone/gnomish_tailoring35_actor.h. Manual and auto matched-recipe boundaries reject actors whose native player-profile race is not Race::Gnome before consumption/roll/output. The auto path validates a real personal-inventory Clockwork Sewing Kit against request identity. No all-Tailoring gate, schema or native mask mutation.

Source only. GitHub owns publication, Build owns compilation/deployment, coordinator owns installation and SQL ordering. DO NOT APPLY RECIPE SQL BEFORE compatible actor code is installed. No tests/build/runtime performed by this author.
