# AA AllStats and Staff Block handlers

Existing AA rank effects for the two Dragons of Norrath Gift rewards use
SPA159 (AllStats), and fourteen existing Staff Block ranks use SPA405
(TwoHandBluntBlock). The AA bonus dispatcher lacked these cases, so the
configured effects reached its unaccounted-effect fallback.

The change adds the configured AllStats value to the seven normal stat fields
and the configured Staff Block value to the existing TwoHandBluntBlock field.
These follow the corresponding spell handlers and existing AA stat accumulation.
The combat consumer already applies the equipment, direction and chance rules.

The retained RoF2 client descriptions identify the Gift stat bonus and Staff
Block's two-handed blunt defense. This repairs the existing data path; it does
not establish complete May 31, 2013 coverage or change rank magnitudes.
An additional existing placeholder rank, 30150, carries AllStats and benefits
from the generic handler if granted; its historical eligibility is unresolved.

Shared-core static review confirmed the accumulation, field consumers and
preservation of the combined runtime's existing shroud handler. The feature
payload is based on develop c9c8747ac949deddb140d196d241fa4e777500c7.
Compilation, deployment and native behavior validation are separate outstanding
steps. No SQL or player-data migration is required.
