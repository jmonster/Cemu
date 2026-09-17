#pragma once

namespace CemuSwitch2Kit
{
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
