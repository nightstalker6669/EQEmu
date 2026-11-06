-- Exact coordinator Database181605 contract bound to DISABLED source only.
return {
    enabled = true,
    clearance_status = "COORDINATOR_BOUND_FROM_CLEAR_CONTRACT",
    tasks = {abandon=920137, fallen=920138, desperate=920139, sporali=920140},
    npc_types = {mardan=485068, entricalla=485090, haeger=482189, borgar=486067, gronkin=483056},
    world_grogald = 485001,
    instance_npc_types = {
        grazden=485900, west_guard=485901, east_guard=485902,
        kerod=485903, dek=485904, koryx=485905, kikivin=485906,
        rotak=485907, thaurnek=485908, extract_chest=485909,
        compound_chest=485910, reward_chest=485911, grogald_instance=485912,
    },
    mission_zone = 485,
    mission_version = 1,
    dynamic_zone_template = 920140,
    desperate_controller_ready = true,
    mission_controller_ready = true,
    terminal_consumer = "native_open", -- Explicit configurablelocalchoice, originalunknown.
    reward_chest_condition = 6202, -- Exact183019CURRENT clearedcondition, sourceonly.
    agent_npc_type = 485913, -- Exact183444extension.
    mardan_spawn = 133832,
    entry = {x=425, y=-1685, z=197, h=0}, -- Qualifiedlocalentrycandidate.
}
