#pragma once

#include <cstdint>

enum Buttons2 : std::uint64_t
{
	// General
	kButton0,
	kButton1,
	kButton2,
	kButton3,
	kButton4,
	kButton5,
	kButton6,
	kButton7,
	kButton8,
	kButton9,
	kButton10,
	kButton11,
	kButton12,
	kButton13,
	kButton14,
	kButton15,
	kButton16,
	kButton17,
	kButton18,
	kButton19,
	kButton20,
	kButton21,
	kButton22,
	kButton23,
	kButton24,
	kButton25,
	kButton26,
	kButton27,
	kButton28,
	kButton29,
	kButton30,
	kButton31,

	// Trigger
	kButtonZL,
	kButtonZR,

	// DPAD
	kButtonUp,
	kButtonDown,
	kButtonLeft,
	kButtonRight,

	// positive values
	kAxisXP,
	kAxisYP,

	kRotationXP,
	kRotationYP,

	kTriggerXP,
	kTriggerYP,

	// negative values
	kAxisXN,
	kAxisYN,

	kRotationXN,
	kRotationYN,

	kTriggerXN,
	kTriggerYN,

	kButtonMAX,

	kButtonNoneAxisMAX = kButtonRight,
	kButtonAxisStart = kAxisXP,
};
