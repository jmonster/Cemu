#pragma once

#include <cstddef>

namespace CemuSwitch2Kit
{
// InputManager normally inherits sources from the outgoing slot when the new
// slot is empty. Remove that outgoing slot first so rollback restores an empty
// prior configuration exactly, rather than inheriting the new controller.
template <typename Manager, typename Controller>
void RestoreSlot(Manager& manager, std::size_t index, const Controller& previous)
{
	manager.delete_controller(index);
	if (previous)
		manager.set_controller(previous);
}

// Save the old profile before changing the active slot. A failed atomic save or
// an exception after applying the new slot restores the in-memory configuration.
// Callers validate identity/selection again after any modal confirmation.
template <typename Backup, typename Apply, typename Save, typename Rollback>
bool CommitSetup(Backup backup, Apply apply, Save save, Rollback rollback)
{
	if (!backup())
		return false;
	try
	{
		apply();
		if (save())
			return true;
	}
	catch (...)
	{
	}
	rollback();
	return false;
}
} // namespace CemuSwitch2Kit
