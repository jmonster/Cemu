#include <cstdlib>
#include <iostream>
#include "input/api/SDL/Switch2KitSession.h"

// One lifecycle scenario against Cemu's production session. Only the external
// host is controlled; this does not simulate Bluetooth, SDL or a running game.
struct RecordingHost
{
	int discoveryResult = 0, pumpResult = 0, stopResult = 0;
	int discoveries = 0, pumps = 0, stops = 0, shutdowns = 0;

	int discover() { ++discoveries; return discoveryResult; }
	int pump() { ++pumps; return pumpResult; }
	int stop() { ++stops; return stopResult; }
	void shutdown() { ++shutdowns; }
};

static void Check(bool condition, const char* message)
{
	// Do not use assert: the check must still execute in NDEBUG builds.
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

int main()
{
	Switch2KitSession<RecordingHost> session;
	auto& host = session.GetHost();
	Check(!session.IsEnabled() && session.Pump() == 0 && host.pumps == 0 &&
		host.discoveries == 0, "startup must not discover or pump without consent");

	host.discoveryResult = -1;
	Check(session.Discover() == -1 && host.discoveries == 1 && !session.IsEnabled(),
		"failed discovery must propagate its error without enabling the session");
	Check(session.Pump() == 0 && host.pumps == 0, "failed startup must not enable polling");

	host.discoveryResult = 0;
	Check(session.Discover() == 0 && host.discoveries == 2 && session.IsEnabled(),
		"explicit retry must enable a successfully started session");
	host.pumpResult = -2;
	Check(session.Pump() == -2 && host.pumps == 1, "active polling must reach the host and return its result");

	host.discoveryResult = -3;
	Check(session.Discover() == -3 && host.discoveries == 3 && session.IsEnabled(),
		"failed rescan must preserve an already active session");
	Check(session.Pump() == -2 && host.pumps == 2, "existing input must survive failed rescan");

	host.stopResult = -4;
	Check(session.Stop() == -4 && host.stops == 1 && !session.IsEnabled(),
		"disconnect must fence polling even when the host reports a stop error");
	Check(session.Pump() == 0 && host.pumps == 2, "polling must not restart a stopped session");

	host.discoveryResult = 0;
	Check(session.Discover() == 0 && host.discoveries == 4 && session.IsEnabled(),
		"a stopped session must permit an explicit restart");
	Check(session.Pump() == -2 && host.pumps == 3, "restarted session must forward polling");
	session.Shutdown();
	Check(host.shutdowns == 1 && !session.IsEnabled() && session.Pump() == 0 && host.pumps == 3,
		"shutdown must reach the host and prevent subsequent polling");

	std::cout << "PASS: Switch2Kit session lifecycle\n";
	return EXIT_SUCCESS;
}
