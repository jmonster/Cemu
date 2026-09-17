#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include "ControllerEnums.h"
#include "input/api/SDL/Switch2KitMapping.h"
#include "input/api/SDL/Switch2KitIdentity.h"
#include "input/api/SDL/Switch2KitSession.h"
#include "wxgui/input/Switch2KitSetupTransaction.h"

// Bluetooth is intentionally absent here. These execute the production policies
// with the actual Cemu/SDL/SDK enum values and controlled host/storage boundaries.
struct Host
{
	int result = 0, discoveries = 0, pumps = 0, stops = 0, shutdowns = 0;
	int discover() { ++discoveries; return result; }
	int pump() { ++pumps; return 0; }
	int stop() { ++stops; return 0; }
	void shutdown() { ++shutdowns; }
};

void SessionTests()
{
	Switch2KitSession<Host> session;
	auto& host = session.GetHost();
	session.Pump();
	assert(!session.IsEnabled() && host.pumps == 0);
	host.result = -1;
	assert(session.Discover() == -1);
	session.Pump();
	assert(!session.IsEnabled() && host.pumps == 0);
	host.result = 0;
	assert(session.Discover() == 0);
	session.Pump();
	assert(session.IsEnabled() && host.pumps == 1);
	// A failed additional scan must not tear down an already connected session.
	host.result = -2;
	assert(session.Discover() == -2 && session.IsEnabled());
	assert(session.Stop() == 0);
	for (int i = 0; i < 1000; ++i) session.Pump();
	assert(!session.IsEnabled() && host.pumps == 1 && host.stops == 1);
	assert(session.Discover() == -2 && !session.IsEnabled());
	host.result = 0;
	assert(session.Discover() == 0);
	session.Pump();
	assert(host.pumps == 2);
	session.Shutdown();
	session.Pump();
	assert(!session.IsEnabled() && host.pumps == 2 && host.shutdowns == 1);
	std::cout << "PASS startup consent, failed-start retry, disconnect fence and restart\n";
}

template <typename Pad>
void MappingTests()
{
	using namespace CemuSwitch2Kit;
	for (unsigned model : {S2K_PRO, S2K_GAMECUBE, S2K_JOYCON_LEFT, S2K_JOYCON_RIGHT})
	{
		auto entries = GamepadMapping<Pad>(model);
		std::map<std::uint64_t, std::uint64_t> mapping(entries.begin(), entries.end());
		assert(mapping.size() == entries.size());
		for (auto [button, input] : entries)
		{
			assert(button > Pad::kButtonId_None && button < Pad::kButtonId_Max);
			assert(input < kButtonMAX);
		}
		if (model != S2K_JOYCON_RIGHT)
		{
			assert(mapping.at(Pad::kButtonId_StickL_Up) == kAxisYN);
			assert(mapping.at(Pad::kButtonId_StickL_Down) == kAxisYP);
			assert(mapping.at(Pad::kButtonId_StickL_Left) == kAxisXN);
			assert(mapping.at(Pad::kButtonId_ZL) == kTriggerXP);
			assert(mapping.at(Pad::kButtonId_Minus) == (model == S2K_GAMECUBE ? SDL_GAMEPAD_BUTTON_MISC2 : SDL_GAMEPAD_BUTTON_BACK));
		}
		if (model != S2K_JOYCON_LEFT)
		{
			assert(mapping.at(Pad::kButtonId_A) == (model == S2K_GAMECUBE ? SDL_GAMEPAD_BUTTON_SOUTH : SDL_GAMEPAD_BUTTON_EAST));
			assert(mapping.at(Pad::kButtonId_B) == (model == S2K_GAMECUBE ? SDL_GAMEPAD_BUTTON_WEST : SDL_GAMEPAD_BUTTON_SOUTH));
			assert(mapping.at(Pad::kButtonId_X) == (model == S2K_GAMECUBE ? SDL_GAMEPAD_BUTTON_EAST : SDL_GAMEPAD_BUTTON_NORTH));
			assert(mapping.at(Pad::kButtonId_Y) == (model == S2K_GAMECUBE ? SDL_GAMEPAD_BUTTON_NORTH : SDL_GAMEPAD_BUTTON_WEST));
			assert(mapping.at(Pad::kButtonId_StickR_Up) == kRotationYN);
			assert(mapping.at(Pad::kButtonId_StickR_Right) == kRotationXP);
			assert(mapping.at(Pad::kButtonId_ZR) == kTriggerYP);
			assert(mapping.at(Pad::kButtonId_R) == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
		}
		if constexpr (requires { Pad::kButtonId_StickL; Pad::kButtonId_StickR; })
		{
			assert(mapping.contains(Pad::kButtonId_StickL) == (model != S2K_GAMECUBE && model != S2K_JOYCON_RIGHT));
			assert(mapping.contains(Pad::kButtonId_StickR) == (model != S2K_GAMECUBE && model != S2K_JOYCON_LEFT));
		}
	}
	auto left = GamepadMapping<Pad>(S2K_JOYCON_LEFT);
	auto right = GamepadMapping<Pad>(S2K_JOYCON_RIGHT);
	std::map<std::uint64_t, std::uint64_t> combined(left.begin(), left.end());
	combined.insert(right.begin(), right.end());
	assert(combined.size() == left.size() + right.size());
	auto pro = GamepadMapping<Pad>(S2K_PRO);
	assert((combined == std::map<std::uint64_t, std::uint64_t>(pro.begin(), pro.end())));
	assert(!IsSupportedModel(0) && !IsSupportedModel(0x2009));
}

void TransactionTests()
{
	for (int failure = 0; failure != 6; ++failure)
	{
		std::string order;
		int slot = 7;
		bool backedUp = false;
		bool result = false;
		try
		{
			result = CemuSwitch2Kit::CommitSetup(
				[&] {
					order += 'B';
					assert(slot == 7);
					if (failure == 1) return false;
					if (failure == 2) throw std::runtime_error("backup");
					backedUp = true;
					return true;
				},
				[&] {
					order += 'A';
					assert(backedUp);
					slot = 8;
					if (failure == 3) throw std::runtime_error("apply");
				},
				[&] {
					order += 'S';
					assert(slot == 8);
					if (failure == 4) return false;
					if (failure == 5) throw std::runtime_error("save");
					return true;
				},
				[&] { order += 'R'; slot = 7; });
		}
		catch (const std::runtime_error&) { assert(failure == 2); }
		if (failure == 0)
			assert(result && slot == 8 && order == "BAS");
		else
		{
			assert(!result && slot == 7);
			assert(order == (failure < 3 ? "B" : failure == 3 ? "BAR" : "BASR"));
		}
	}
	std::cout << "PASS backup-before-replace, backup refusal, save failure and exception rollback\n";
}

int main()
{
	using CemuSwitch2Kit::ValidPhysicalKey;
	assert(ValidPhysicalKey("s2k:0123456789abcdef0123456789abcdef"));
	for (const auto* invalid : {"", "s2k:", "s2k:0123456789abcdef0123456789abcdeg", "s2k:0123456789abcdef0123456789abcdeF", "0_0123456789abcdef0123456789abcdef", "s2k:0123456789abcdef0123456789abcdef0"})
		assert(!ValidPhysicalKey(invalid));
	std::cout << "PASS persistent identity validation without ordinal fallback\n";
	SessionTests();
	MappingTests<VPADController>();
	MappingTests<ProController>();
	MappingTests<ClassicController>();
	std::cout << "PASS GameCube/Pro labels, trigger/stick axes and complementary Joy-Con mappings for all three pad types\n";
	TransactionTests();
}
