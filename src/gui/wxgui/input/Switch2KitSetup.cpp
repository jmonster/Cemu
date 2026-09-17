#include "wxgui/input/Switch2KitSetup.h"
#include "wxgui/input/Switch2KitSetupTransaction.h"

#include <wx/filename.h>
#include <wx/msgdlg.h>
#include "config/ActiveSettings.h"
#include "input/ControllerFactory.h"
#include "input/InputManager.h"
#include "input/api/SDL/SDLController.h"
#include "input/api/SDL/Switch2KitMapping.h"

namespace
{
	bool AssignedElsewhere(size_t playerIndex, const ControllerPtr& controller)
	{
		for (size_t i = 0; i < InputManager::kMaxController; ++i)
		{
			const auto pad = InputManager::instance().get_controller(i);
			if (i == playerIndex || !pad)
				continue;
			for (const auto& source : pad->get_controllers())
			{
				if (*source == *controller)
					return true;
			}
		}
		return false;
	}
}

bool ApplySwitch2KitSetup(wxWindow* parent, size_t playerIndex, const ControllerPtr& controller)
{
	auto& manager = InputManager::instance();
	const auto native = std::dynamic_pointer_cast<SDLController>(controller);
	if (playerIndex >= InputManager::kMaxController || !native || !native->IsSwitch2Controller())
		return false;
	if (!native->connect())
	{
		wxMessageBox(_("Connect the controller with Find Switch 2 Controllers first."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}
	const auto model = native->GetSwitch2Model();
	if (!model || (*model != S2K_GAMECUBE && *model != S2K_PRO))
		return false;
	const auto before = manager.get_controller(playerIndex);
	if (manager.is_gameprofile_set(playerIndex))
	{
		wxMessageBox(_("A game profile controls this slot. Stop the game before using quick setup."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}
	const auto type = before ? before->type() :
		(playerIndex == 0 ? EmulatedController::VPAD : EmulatedController::Pro);
	if (type == EmulatedController::Wiimote)
	{
		wxMessageBox(_("Select Wii U GamePad, Wii U Pro Controller or Classic Controller for automatic mappings. Wiimote mappings remain available manually."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}
	if (AssignedElsewhere(playerIndex, native))
	{
		wxMessageBox(_("This controller is assigned to another slot. Remove it there first."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}
	const auto snapshot = manager.ControllerConfigSnapshot(playerIndex);
	if (before && !before->get_controllers().empty() && wxMessageBox(
		_("Replace this slot's controller assignments and button mappings? A backup profile will be saved first. Use Profile > Load to restore it."),
		_("Apply recommended mapping?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, parent) != wxYES)
		return false;

	// A modal question can dispatch controller disconnects or configuration events.
	if (!native->is_connected() || manager.get_controller(playerIndex) != before || manager.ControllerConfigSnapshot(playerIndex) != snapshot ||
		manager.is_gameprofile_set(playerIndex) || AssignedElsewhere(playerIndex, native))
	{
		wxMessageBox(_("The controller or slot changed. Connect it and try again."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}
	const auto [vpads, wpads] = manager.get_controller_count();
	if (!before && ((type == EmulatedController::VPAD && vpads >= InputManager::kMaxVPADControllers) ||
		(type != EmulatedController::VPAD && wpads >= InputManager::kMaxWPADControllers)))
	{
		wxMessageBox(_("No emulated controller slots of this type are available. Disable another slot first."), _("Switch 2 Controllers"), wxOK | wxICON_INFORMATION, parent);
		return false;
	}

	try
	{
		// Construct and validate the replacement before touching the active slot.
		auto replacement = ControllerFactory::CreateEmulatedController(playerIndex, type);
		if (before)
		{
			pugi::xml_document properties;
			auto node = properties.append_child("emulated_controller");
			before->save(node);
			replacement->load(node);
		}
		replacement->add_controller(native);
		if (!replacement->set_default_mapping(native))
			return false;

		const auto previousSettings = native->get_settings();
		// Only a new attachment gets useful defaults. Existing strength, deadzones
		// and motion choices are user preferences, not part of a button preset.
		bool existingSource = false;
		if (before)
			for (const auto& source : before->get_controllers())
				existingSource |= source == native;
		const bool committed = CemuSwitch2Kit::CommitSetup(
			[&] {
				if (before)
				{
					const auto directory = ActiveSettings::GetConfigPath("controllerProfiles");
					fs::create_directories(directory);
					// Reserve a unique stem through wx's exclusive temporary-file helper.
					const auto reserved = wxFileName::CreateTempFileName(wxString::FromUTF8(_pathToUtf8(directory / "Before Switch2Kit-")));
					if (reserved.empty())
						throw std::runtime_error("Could not reserve backup profile");
					const auto reservation = _utf8ToPath(reserved.utf8_string());
					const auto name = _pathToUtf8(reservation.filename()) + ".xml";
					bool backedUp = false;
					try { backedUp = manager.save(playerIndex, name, false); }
					catch (...) { std::error_code ec; fs::remove(reservation, ec); throw; }
					std::error_code ec;
					fs::remove(reservation, ec);
					if (!backedUp)
						return false;
				}

				return true;
			},
			[&] {
				if (!existingSource)
					native->set_rumble(1.0f);
				manager.set_controller(replacement);
			},
			[&] { return manager.save(playerIndex); },
			[&] {
				native->set_settings(previousSettings);
				if (before)
					manager.set_controller(before);
				else
					manager.delete_controller(playerIndex);
			});
		if (!committed)
			throw std::runtime_error("Could not save controller settings");
		return true;
	}
	catch (const std::exception&)
	{
		wxMessageBox(_("Controller settings or their backup could not be saved. The previous slot configuration was retained."), _("Switch 2 Controllers"), wxOK | wxICON_ERROR, parent);
		return false;
	}
}
