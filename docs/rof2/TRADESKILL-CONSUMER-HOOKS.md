# Neutral tradeskill consumer extension points

Manual and automatic combines call a preconsumption method before consuming ingredients, and successful recipe execution calls a result method. In this neutral layer the preconsumption method returns true and the result method does nothing. No achievement types, state, rule, schema or progress logic is present. Null input guards and the existing combine reply behavior are retained at these call sites.

A consumer feature supplies its own method bodies on its layer. The selected achievement proposal adopts the same call sites and supplies the real ownership reconciliation and recipe progress adapter. Do not overwrite that adapter by applying neutral default bodies afterward. These patches describe source layer composition, not a new runtime callback registration framework.

Caller placement comes from the reviewed integrated candidate, whose two bounded headless journal cases passed. This new standalone neutral public baseline has not been compiled or run; it does not establish native combine or consumption safety. A preconsumption check does not close the ownership read-to-consume/shared-bank gap. Result capture begins after the physical outcome and does not establish economic exactly-once delivery.

Source checkpoint only; no deployment, schema/content installation or runtime enablement.
