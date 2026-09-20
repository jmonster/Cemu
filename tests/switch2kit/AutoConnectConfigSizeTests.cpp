#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include "input/api/SDL/Switch2KitAutoConnectConfig.h"
#include "input/api/SDL/Switch2KitSession.h"

namespace
{
std::string Read(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Write(const std::filesystem::path& path, const std::string& bytes)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	output.close();
	assert(output);
}
// Only the host boundary is controlled. The production session and INI parser
// execute unchanged; this test makes no claim about physical Bluetooth.
struct Host
{
	bool automatic = false;
	int policies = 0;
	int setAutomaticDiscovery(bool enabled) { automatic = enabled; ++policies; return 0; }
	int start() { return 0; }
	int discover() { return 0; }
};
}
int main(int argc, char** argv)
{
	assert(argc == 2);
	using CemuSwitch2Kit::AutoConnectConfig;
	using Session = Switch2KitSession<Host>;
	constexpr std::size_t limit = 65536;
	const auto path = std::filesystem::path(argv[1]) / "size-limit.ini";
	int writes = 0;
	// Deliberately simple writer: rejected output must never reach it.
	auto write = [&](const auto& target, const std::string& bytes) {
		++writes;
		Write(target, bytes);
		return true;
	};
	const std::string prefix = "[Settings]\nAutoConnect=true\nPadding=";
	const auto full = prefix + std::string(limit - prefix.size() - 1, 'x') + "\n";
	Write(path, full);
	bool enabled = false;
	assert(AutoConnectConfig::Load(path, enabled) && enabled);
	Session session;
	session.LoadAutoConnect([&](bool& value) { return AutoConnectConfig::Load(path, value); });
	assert(session.StartOnce() == 0 && session.IsEnabled());
	const auto policies = session.GetHost().policies;
	// Changing true to false grows a valid, full document by one byte.
	assert(session.SetAutoConnect(false, [&](bool value) {
		return AutoConnectConfig::Save(path, value, write);
	}) == Session::ConfigurationError);
	assert(writes == 0 && Read(path) == full);
	assert(session.AutoConnect() && session.GetHost().automatic);
	assert(session.IsEnabled() && session.GetHost().policies == policies);
	assert(AutoConnectConfig::Load(path, enabled) && enabled);

	// An exactly-at-limit document must still round-trip when it does not grow.
	assert(AutoConnectConfig::Save(path, true, write) && writes == 1);
	assert(Read(path).size() == limit && AutoConnectConfig::Load(path, enabled) && enabled);
	const auto almostFull = prefix + std::string(limit - prefix.size() - 2, 'x') + "\n";
	Write(path, almostFull);
	assert(AutoConnectConfig::Save(path, false, write) && writes == 2);
	assert(Read(path).size() == limit && AutoConnectConfig::Load(path, enabled) && !enabled);

	// Adding the missing setting/section can also push an otherwise valid INI
	// over the read bound. Preserve every byte and never call the writer.
	const std::string otherPrefix = "[Other]\nPadding=";
	const auto other = otherPrefix + std::string(limit - otherPrefix.size() - 1, 'x') + "\n";
	Write(path, other);
	assert(AutoConnectConfig::Load(path, enabled) && !enabled);
	assert(!AutoConnectConfig::Save(path, true, write));
	assert(writes == 2 && Read(path) == other);
	std::filesystem::remove(path);
	std::cout << "PASS output size bound, exact-limit round trips, file and runtime preservation\n";
}
