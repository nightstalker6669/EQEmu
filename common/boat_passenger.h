#pragma once

#include <chrono>
#include <cstdint>

namespace EQ {

// Transport passenger observations are separate from controllable rowboats.
// The caller validates the referenced entity before recording a movement packet.
class BoatPassengerState {
public:
	using Clock = std::chrono::steady_clock;
	static constexpr auto MaxAge = std::chrono::seconds(15);

	void Observe(uint16_t vehicle_id, bool valid_boat, Clock::time_point now = Clock::now())
	{
		m_vehicle_id = valid_boat ? vehicle_id : 0;
		m_observed = now;
	}

	uint16_t Get(Clock::time_point now = Clock::now()) const
	{
		return m_vehicle_id != 0 && now >= m_observed && now - m_observed <= MaxAge
			? m_vehicle_id : 0;
	}

private:
	uint16_t m_vehicle_id = 0;
	Clock::time_point m_observed{};
};

} // namespace EQ
