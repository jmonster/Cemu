#pragma once

#include "input/api/Controller.h"
class wxWindow;

// Explicit UI action. Returns true only after the new default profile is saved.
bool ApplySwitch2KitSetup(wxWindow* parent, size_t playerIndex, const ControllerPtr& controller);
