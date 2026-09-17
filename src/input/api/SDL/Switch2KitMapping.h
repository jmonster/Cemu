#pragma once

#include <optional>
#include <utility>
#include <vector>
#include <Switch2KitC.h>
#include <SDL3/SDL_gamepad.h>
#include "input/api/ControllerButtons.h"

namespace CemuSwitch2Kit
{
using Mapping = std::vector<std::pair<std::uint64_t, std::uint64_t>>;

// The SDK preserves physical face-button positions for the asymmetrical GameCube
// layout. Translate once here to Wii U's printed Nintendo labels. Split Joy-Con
// mappings intentionally omit the other half so Cemu can combine two sources.
template <typename Pad>
Mapping GamepadMapping(unsigned model)
{
	const bool gc = model == S2K_GAMECUBE;
	const bool left = model != S2K_JOYCON_RIGHT;
	const bool right = model != S2K_JOYCON_LEFT;
	Mapping mapping;
	if (right)
	{
		mapping = {
			{Pad::kButtonId_A, gc ? SDL_GAMEPAD_BUTTON_SOUTH : SDL_GAMEPAD_BUTTON_EAST},
			{Pad::kButtonId_B, gc ? SDL_GAMEPAD_BUTTON_WEST : SDL_GAMEPAD_BUTTON_SOUTH},
			{Pad::kButtonId_X, gc ? SDL_GAMEPAD_BUTTON_EAST : SDL_GAMEPAD_BUTTON_NORTH},
			{Pad::kButtonId_Y, gc ? SDL_GAMEPAD_BUTTON_NORTH : SDL_GAMEPAD_BUTTON_WEST},
			{Pad::kButtonId_R, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
			{Pad::kButtonId_ZR, kTriggerYP},
			{Pad::kButtonId_Plus, SDL_GAMEPAD_BUTTON_START},
			{Pad::kButtonId_Home, SDL_GAMEPAD_BUTTON_GUIDE},
			{Pad::kButtonId_StickR_Up, kRotationYN},
			{Pad::kButtonId_StickR_Down, kRotationYP},
			{Pad::kButtonId_StickR_Left, kRotationXN},
			{Pad::kButtonId_StickR_Right, kRotationXP},
		};
	}
	if (left)
	{
		const Mapping leftMapping = {
			{Pad::kButtonId_L, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
			{Pad::kButtonId_ZL, kTriggerXP},
			{Pad::kButtonId_Minus, gc ? SDL_GAMEPAD_BUTTON_MISC2 : SDL_GAMEPAD_BUTTON_BACK},
			{Pad::kButtonId_Up, SDL_GAMEPAD_BUTTON_DPAD_UP},
			{Pad::kButtonId_Down, SDL_GAMEPAD_BUTTON_DPAD_DOWN},
			{Pad::kButtonId_Left, SDL_GAMEPAD_BUTTON_DPAD_LEFT},
			{Pad::kButtonId_Right, SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
			{Pad::kButtonId_StickL_Up, kAxisYN},
			{Pad::kButtonId_StickL_Down, kAxisYP},
			{Pad::kButtonId_StickL_Left, kAxisXN},
			{Pad::kButtonId_StickL_Right, kAxisXP},
		};
		mapping.insert(mapping.end(), leftMapping.begin(), leftMapping.end());
	}
	if constexpr (requires { Pad::kButtonId_StickL; Pad::kButtonId_StickR; })
	{
		if (!gc && left)
			mapping.emplace_back(Pad::kButtonId_StickL, SDL_GAMEPAD_BUTTON_LEFT_STICK);
		if (!gc && right)
			mapping.emplace_back(Pad::kButtonId_StickR, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
	}
	if constexpr (requires { Pad::kButtonId_Mic; Pad::kButtonId_Screen; })
	{
		if (left)
			mapping.emplace_back(Pad::kButtonId_Mic, SDL_GAMEPAD_BUTTON_MISC1);
		if (!gc && right)
			mapping.emplace_back(Pad::kButtonId_Screen, SDL_GAMEPAD_BUTTON_MISC2);
	}
	return mapping;
}

inline bool IsSupportedModel(unsigned model)
{
	return model == S2K_GAMECUBE || model == S2K_PRO ||
		model == S2K_JOYCON_LEFT || model == S2K_JOYCON_RIGHT;
}
} // namespace CemuSwitch2Kit
