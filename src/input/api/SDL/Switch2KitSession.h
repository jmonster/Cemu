#pragma once

#include <utility>

// Lifecycle, preference and polling methods belong to Cemu's GUI main thread
// on every native platform. The SDK host serializes identity/profile access
// from configuration threads. Pump never loads settings, changes discovery
// policy, or starts support.
template <typename Host>
class Switch2KitSession
{
public:
	static constexpr int ConfigurationError = -1000;
	static constexpr int ShutdownError = -1001;

	// Load once after SDL initialization, before queueing the main-loop startup.
	// A failed read must not be interpreted as new consent or overwrite the file.
	template <typename Load>
	void LoadAutoConnect(Load&& load)
	{
		if (std::exchange(m_loaded, true) || m_shutdown)
			return;
		bool enabled = false;
		if (!load(enabled))
			m_configError = ConfigurationError;
		else
			m_autoConnect = enabled;
	}

	int StartOnce()
	{
		if (std::exchange(m_startupConsumed, true) || m_shutdown || !m_autoConnect)
			return 0;
		return StartSupport();
	}

	int Discover()
	{
		m_startupConsumed = true;
		if (m_shutdown)
			return m_actionError = ShutdownError;
		// Find is also the explicit retry/resume action. Automatic mode does not
		// create a finite discovery window, even when Find is pressed.
		return StartSupport();
	}

	template <typename Save>
	int SetAutoConnect(bool enabled, Save&& save)
	{
		if (m_shutdown)
			return m_actionError = ShutdownError;
		// Persist first. Failed writes must not alter the choice or radio policy.
		if (!save(enabled))
			return m_configError = ConfigurationError;
		m_configError = 0;
		m_autoConnect = enabled;
		m_startupConsumed = true;
		if (enabled)
			return StartSupport();
		// Do not stop the host or detach ready controllers when opting out.
		return m_actionError = m_host.setAutomaticDiscovery(false);
	}

	int Stop()
	{
		m_startupConsumed = true;
		m_enabled = false;
		m_pumpError = 0;
		if (m_shutdown)
			return 0;
		return m_actionError = m_host.stop();
	}

	int Pump()
	{
		return m_pumpError = m_enabled ? m_host.pump() : 0;
	}

	void Shutdown()
	{
		m_startupConsumed = true;
		m_enabled = false;
		if (!std::exchange(m_shutdown, true))
			m_host.shutdown();
	}

	bool IsEnabled() const { return m_enabled; }
	bool AutoConnect() const { return m_autoConnect; }
	// Successful polling/status reads must not erase failed user actions.
	int Error() const { return m_configError ? m_configError : m_actionError ? m_actionError : m_pumpError; }
	Host& GetHost() { return m_host; }

private:
	int StartSupport()
	{
		m_actionError = m_host.setAutomaticDiscovery(m_autoConnect);
		if (m_actionError == 0)
			m_actionError = m_autoConnect ? m_host.start() : m_host.discover();
		// A failed additional discovery must not stop an already live session.
		if (m_actionError == 0)
			m_enabled = true;
		return m_actionError;
	}

	Host m_host;
	bool m_enabled = false;
	bool m_autoConnect = false;
	bool m_loaded = false;
	bool m_startupConsumed = false;
	bool m_shutdown = false;
	int m_configError = 0;
	int m_actionError = 0;
	int m_pumpError = 0;
};
