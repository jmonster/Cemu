#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include "input/api/SDL/Switch2KitSession.h"
#include "input/api/SDL/Switch2KitAutoConnectConfig.h"

// Test boundaries only: no Bluetooth, input, or emulator behavior is fabricated.
struct AutoHost
{
	int starts = 0, discoveries = 0, policies = 0, pumps = 0, stops = 0, shutdowns = 0;
	int startResult = 0, discoveryResult = 0, policyResult = 0, stopResult = 0, pumpResult = 0;
	bool automatic = false;
	int setAutomaticDiscovery(bool enabled)
	{
		++policies;
		if (!policyResult) automatic = enabled;
		return policyResult;
	}
	int start() { ++starts; return startResult; }
	int discover() { ++discoveries; return discoveryResult; }
	int pump() { ++pumps; return pumpResult; }
	int stop() { ++stops; return stopResult; }
	void shutdown() { ++shutdowns; }
};
using Session = Switch2KitSession<AutoHost>;

void StartupTests()
{
	Session off;
	auto& manual = off.GetHost();
	int reads = 0;
	off.LoadAutoConnect([&](bool& value) { ++reads; value = false; return true; });
	off.LoadAutoConnect([&](bool&) { ++reads; return false; });
	assert(off.StartOnce() == 0 && off.StartOnce() == 0);
	for (int i = 0; i < 10000; ++i) assert(off.Pump() == 0);
	assert(reads == 1 && !off.AutoConnect() && !off.IsEnabled());
	assert(manual.policies == 0 && manual.starts == 0 && manual.discoveries == 0 && manual.pumps == 0);
	assert(off.Discover() == 0 && off.IsEnabled());
	assert(manual.discoveries == 1 && manual.starts == 0 && !manual.automatic);

	Session on;
	auto& host = on.GetHost();
	on.LoadAutoConnect([&](bool& value) { ++reads; value = true; return true; });
	assert(on.StartOnce() == 0 && on.StartOnce() == 0 && on.AutoConnect());
	for (int i = 0; i < 10000; ++i) assert(on.Pump() == 0);
	assert(host.policies == 1 && host.starts == 1 && host.discoveries == 0);
	assert(host.pumps == 10000 && host.automatic && reads == 2);
	assert(on.Stop() == 0 && on.AutoConnect());
	for (int i = 0; i < 10000; ++i) { on.Pump(); on.StartOnce(); }
	assert(host.starts == 1 && host.pumps == 10000 && !on.IsEnabled());
	assert(on.Discover() == 0 && host.starts == 2 && host.discoveries == 0);

	Session deferred;
	deferred.LoadAutoConnect([](bool& value) { value = true; return true; });
	assert(deferred.Stop() == 0);
	assert(deferred.StartOnce() == 0);
	assert(deferred.GetHost().starts == 0 && deferred.GetHost().policies == 0);
	assert(deferred.AutoConnect() && !deferred.IsEnabled());
	assert(deferred.Discover() == 0 && deferred.GetHost().starts == 1);

	Session explicitFirst;
	explicitFirst.LoadAutoConnect([](bool& value) { value = true; return true; });
	assert(explicitFirst.Discover() == 0);
	assert(explicitFirst.StartOnce() == 0 && explicitFirst.GetHost().starts == 1);
	std::cout << "PASS default-off, saved-consent one-shot startup, no polling renewal, explicit-stop fence\n";
}

void PreferenceTests()
{
	Session session;
	auto& host = session.GetHost();
	int writes = 0;
	bool saved = false;
	auto save = [&](bool value) { ++writes; saved = value; return true; };
	assert(session.SetAutoConnect(true, save) == 0);
	assert(saved && session.AutoConnect() && host.starts == 1 && host.automatic);
	assert(session.StartOnce() == 0 && host.starts == 1);
	assert(session.SetAutoConnect(false, save) == 0);
	assert(!saved && !session.AutoConnect() && !host.automatic);
	assert(session.IsEnabled() && host.stops == 0 && host.starts == 1);
	session.Pump();
	assert(host.pumps == 1);
	assert(session.Discover() == 0 && host.discoveries == 1);

	const auto policies = host.policies;
	assert(session.SetAutoConnect(true, [&](bool) { ++writes; return false; }) == Session::ConfigurationError);
	assert(!session.AutoConnect() && !saved && host.policies == policies && host.starts == 1);
	for (int i = 0; i < 1000; ++i) session.Pump();
	assert(session.Error() == Session::ConfigurationError && session.IsEnabled());
	assert(session.SetAutoConnect(true, save) == 0 && session.Error() == 0);
	assert(saved && writes == 4 && session.AutoConnect());
	assert(session.Stop() == 0);
	assert(session.SetAutoConnect(true, save) == 0 && session.IsEnabled());

	Session nextLaunch;
	nextLaunch.LoadAutoConnect([&](bool& value) { value = saved; return true; });
	assert(nextLaunch.StartOnce() == 0 && nextLaunch.GetHost().starts == 1);
	std::cout << "PASS atomic-save boundary, opt-out keeps live input, explicit re-enable, next-launch consent\n";
}

void ErrorAndShutdownTests()
{
	Session session;
	auto& host = session.GetHost();
	session.LoadAutoConnect([](bool& value) { value = true; return true; });
	host.policyResult = 5;
	assert(session.StartOnce() == 5 && !session.IsEnabled());
	assert(host.starts == 0);
	session.Pump();
	assert(session.Error() == 5);
	host.policyResult = 0;
	assert(session.StartOnce() == 0 && host.starts == 0); // no automatic retry
	host.startResult = 4;
	assert(session.Discover() == 4 && !session.IsEnabled());
	session.Pump();
	assert(session.Error() == 4);
	host.startResult = 0;
	assert(session.Discover() == 0 && session.Error() == 0);
	host.policyResult = 5;
	assert(session.SetAutoConnect(false, [](bool) { return true; }) == 5);
	assert(!session.AutoConnect() && session.IsEnabled() && host.automatic);
	for (int i = 0; i < 1000; ++i) session.Pump();
	assert(session.Error() == 5); // polling cannot erase failed policy application
	host.policyResult = 0;
	assert(session.Discover() == 0 && !host.automatic && host.discoveries == 1);
	host.discoveryResult = 6;
	assert(session.Discover() == 6 && session.IsEnabled());
	session.Pump();
	assert(session.Error() == 6);
	host.stopResult = 5;
	assert(session.Stop() == 5 && !session.IsEnabled());
	const auto pumps = host.pumps;
	session.Pump(); session.StartOnce();
	assert(host.pumps == pumps && session.Error() == 5);
	session.Shutdown(); session.Shutdown();
	assert(host.shutdowns == 1 && !session.IsEnabled());
	const auto starts = host.starts;
	assert(session.Discover() == Session::ShutdownError);
	assert(session.SetAutoConnect(true, [](bool) { assert(false); return true; }) == Session::ShutdownError);
	session.StartOnce(); session.Pump();
	assert(host.starts == starts && host.pumps == pumps);

	Session beforeCallback;
	beforeCallback.LoadAutoConnect([](bool& value) { value = true; return true; });
	beforeCallback.Shutdown();
	assert(beforeCallback.StartOnce() == 0 && beforeCallback.GetHost().starts == 0);
	Session unreadable;
	unreadable.LoadAutoConnect([](bool& value) { value = true; return false; });
	assert(!unreadable.AutoConnect() && unreadable.Error() == Session::ConfigurationError);
	unreadable.StartOnce(); unreadable.Pump();
	assert(unreadable.GetHost().policies == 0 && unreadable.GetHost().starts == 0);
	assert(unreadable.SetAutoConnect(true, [](bool) { return true; }) == 0);
	assert(unreadable.AutoConnect() && unreadable.Error() == 0);
	std::cout << "PASS sticky policy/start/stop errors, retry, failed reads and shutdown-before-start\n";
}

std::string ReadBytes(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void WriteBytes(const std::filesystem::path& path, const std::string& bytes)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	output.close();
	assert(output);
}

void ConfigurationTests(const std::filesystem::path& directory)
{
	using CemuSwitch2Kit::AutoConnectConfig;
	const auto path = directory / "Switch2Kit.ini";
	bool value = true;
	assert(AutoConnectConfig::Load(path, value) && !value && !std::filesystem::exists(path));
	int writes = 0;
	// This writer is a test boundary. Production uses FileStream::WriteFileAtomic.
	auto write = [&](const auto& target, const std::string& bytes) {
		++writes;
		auto temporary = target; temporary += ".test-tmp";
		WriteBytes(temporary, bytes);
		std::filesystem::rename(temporary, target);
		return true;
	};
	assert(AutoConnectConfig::Save(path, true, write));
	assert(AutoConnectConfig::Load(path, value) && value && writes == 1);
	WriteBytes(path, "[Settings]\nAutoConnect=1\nOther=retained\n[Identities]\ncontroller=s2k:0123456789abcdef0123456789abcdef\n");
	assert(AutoConnectConfig::Load(path, value) && value);
	// An edit made since loading must also survive the next write.
	std::ofstream(path, std::ios::app) << "external=changed\n";
	assert(AutoConnectConfig::Save(path, false, write));
	assert(AutoConnectConfig::Load(path, value) && !value);
	const auto before = ReadBytes(path);
	assert(before.find("Other=retained") != std::string::npos);
	assert(before.find("controller=s2k:0123456789abcdef0123456789abcdef") != std::string::npos);
	assert(before.find("external=changed") != std::string::npos);
	assert(!AutoConnectConfig::Save(path, true, [](const auto&, const auto&) { return false; }));
	assert(ReadBytes(path) == before);
	assert(!AutoConnectConfig::Save(path, true, [](const auto&, const auto&) -> bool { throw std::runtime_error("write"); }));
	assert(ReadBytes(path) == before);

	for (const auto& invalid : {std::string("[Settings\nAutoConnect=true\n"),
		std::string("[Settings]\nAutoConnect=maybe\n"),
		std::string("[Settings]\nAutoConnect=true\nAutoConnect=false\n"),
		std::string("[Settings]\nAutoConnect=true\n[Settings]\nOther=duplicate\n"),
		std::string("[Settings]\nAutoConnect=true\n\0", 29), std::string(65537, 'x')})
	{
		WriteBytes(path, invalid);
		value = true;
		assert(!AutoConnectConfig::Load(path, value) && value);
		assert(!AutoConnectConfig::Save(path, false, write));
		assert(ReadBytes(path) == invalid && writes == 2);
	}
	std::filesystem::remove(path);
	std::filesystem::create_directory(path);
	assert(!AutoConnectConfig::Load(path, value));
	assert(!AutoConnectConfig::Save(path, true, write));
	std::filesystem::remove(path);
	const auto target = directory / "target.ini";
	WriteBytes(target, before);
	std::filesystem::create_symlink(target, path);
	assert(!AutoConnectConfig::Load(path, value));
	assert(!AutoConnectConfig::Save(path, true, write));
	assert(ReadBytes(target) == before);
	std::filesystem::remove(path);
	std::filesystem::remove(target);
	std::filesystem::create_symlink(target, path); // dangling link is not a missing preference
	assert(!AutoConnectConfig::Load(path, value));
	assert(!AutoConnectConfig::Save(path, true, write));
	std::filesystem::remove(path);
	std::cout << "PASS real INI reads, unrelated entries, fresh-save reads, malformed/oversized/type rejection and failed-write preservation\n";
}

int main(int argc, char** argv)
{
	assert(argc == 2);
	StartupTests();
	PreferenceTests();
	ErrorAndShutdownTests();
	ConfigurationTests(argv[1]);
}
