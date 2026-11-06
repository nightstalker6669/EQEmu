#pragma once

#include "fire_personal_after_reward_r2.h"

class Client;
class NPC;

namespace FirePersonalOccurrenceSource {

enum class PersonalDispatchResult {
	Hold,
	// Legacy name: submission lacks a verified fresh-binding receipt. The native
	// task may nevertheless exist. Never cancel, allocate again or retry on this.
	SubmittedNotAccepted,
	// Records the actual persisted acceptance before mutable TASK_ACCEPTED.
	// Subsequent callbacks may change the task; this is not a post-callback view.
	AcceptedBeforeCallbacks
};

// Only the original physical-final-hail owner, after R2 confirms actual cleanup.
// The Shared920169->920170 edge is rejected before taking its handoff.
PersonalDispatchResult DispatchActualOriginalPersonalSuccessorOnce(
	Handoff& original_handoff,
	const Binding& actual_original_finish,
	Client& actual_client,
	NPC& actual_gotonur,
	uint32_t actual_zone,
	uint32_t actual_instance,
	uint32_t owner_selected_original_spawn
);

} // namespace FirePersonalOccurrenceSource
