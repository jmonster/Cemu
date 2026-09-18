#pragma once

#include <atomic>

namespace CemuSwitch2Kit
{
// The event subscription owns this flag, not the settings window. A callback
// already in flight may finish after disconnect() or window destruction.
// Only the window's main-thread timer consumes the flag and touches widgets.
class DeviceChanges
{
public:
	void Notify() { m_pending.store(true, std::memory_order_relaxed); }
	bool Consume() { return m_pending.exchange(false, std::memory_order_relaxed); }

private:
	std::atomic<bool> m_pending{false};
};
} // namespace CemuSwitch2Kit
