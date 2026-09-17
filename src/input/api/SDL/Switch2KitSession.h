#pragma once

// Start/Stop/Pump are called on Cemu's macOS main thread. The SDK host serializes
// identity, profile and status access from controller/configuration threads.
// Keeping the state here also prevents a failed first start from enabling polling.
template <typename Host>
class Switch2KitSession
{
public:
	int Discover()
	{
		const int result = m_host.discover();
		if (result == 0)
			m_enabled = true;
		return result;
	}

	int Stop()
	{
		m_enabled = false;
		return m_host.stop();
	}

	int Pump()
	{
		return m_enabled ? m_host.pump() : 0;
	}

	void Shutdown()
	{
		m_enabled = false;
		m_host.shutdown();
	}

	bool IsEnabled() const { return m_enabled; }
	Host& GetHost() { return m_host; }

private:
	Host m_host;
	bool m_enabled = false;
};
