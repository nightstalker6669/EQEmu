-- Cragbeak: use the native spell's target type, hate filtering and distance damage.
local config = require("rof2_cragbeak_config")
local timer = "rof2_cragbeak_screech"

function event_combat(e)
    if e.joined then
        eq.set_timer(timer, config.first_cast_seconds * 1000)
    else
        eq.stop_timer(timer)
    end
end

function event_timer(e)
    if e.timer ~= timer then return end
    if not e.self:IsEngaged() then
        eq.stop_timer(timer)
        return
    end
    local target = e.self:GetTarget()
    if target.valid then
        -- A nonnull hate target is required by ST_AETargetHateList (32).
        -- CastSpell keeps the native spell's resist/cast/distance semantics.
        e.self:CastSpell(33544, target:GetID())
    end
    eq.set_timer(timer, config.repeat_seconds * 1000)
end
