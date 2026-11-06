/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/event/event_loop.h"
#include "uv.h"
#include <cstring>

namespace EQ {

thread_local EventLoop *EventLoop::m_current = nullptr;
thread_local bool EventLoop::m_thread_shutting_down = false;

EventLoop& EventLoop::Get()
{
	thread_local EventLoop inst;
	return inst;
}

EventLoop::EventLoop()
	: m_loop(std::make_unique<uv_loop_t>())
{
	memset(m_loop.get(), 0, sizeof(uv_loop_t));
	uv_loop_init(m_loop.get());
	m_current = this;
}

EventLoop::~EventLoop()
{
	m_shutting_down = true;
	m_thread_shutting_down = true;
	// TLS destructors precede static service destructors. Close owned handles
	// while their owners and this loop still exist, then finish deferred frees.
	while (!m_cleanup.empty()) {
		auto it = m_cleanup.begin();
		auto cleanup = std::move(it->second);
		m_cleanup.erase(it);
		cleanup();
	}
	while (uv_loop_alive(m_loop.get())) uv_run(m_loop.get(), UV_RUN_DEFAULT);
	uv_loop_close(m_loop.get());
	m_current = nullptr;
}

void EventLoop::RegisterCleanup(const void *owner, std::function<void()> cleanup)
{
	if (!m_shutting_down) m_cleanup[owner] = std::move(cleanup);
}

void EventLoop::RemoveCleanup(const void *owner)
{
	m_cleanup.erase(owner);
}

void EventLoop::Process()
{
	uv_run(m_loop.get(), UV_RUN_NOWAIT);
}

void EventLoop::Run()
{
	uv_run(m_loop.get(), UV_RUN_DEFAULT);
}

void EventLoop::Shutdown()
{
	uv_stop(m_loop.get());
}

} // namespace EQ
