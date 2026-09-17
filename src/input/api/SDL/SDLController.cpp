#include "input/api/SDL/SDLController.h"

#include "input/api/SDL/SDLControllerProvider.h"
#ifdef HAVE_SWITCH2KIT
#include "CemuMotion.hpp"
#include "input/api/SDL/Switch2KitIdentity.h"
#endif

SDLController::SDLController(const SDL_GUID& guid, size_t guid_index)
	: base_type(fmt::format("{}_", guid_index), fmt::format("Controller {}", guid_index + 1)), m_guid_index(guid_index),
	  m_guid(guid)
{
	char tmp[64];
	SDL_GUIDToString(m_guid, tmp, std::size(tmp));
	m_uuid += tmp;
}

SDLController::SDLController(const SDL_GUID& guid, size_t guid_index, std::string_view display_name)
	: base_type(fmt::format("{}_", guid_index), display_name), m_guid_index(guid_index), m_guid(guid)
{
	char tmp[64];
	SDL_GUIDToString(m_guid, tmp, std::size(tmp));
	m_uuid += tmp;
}

#ifdef HAVE_SWITCH2KIT
SDLController::SDLController(std::string_view physical_key, std::string_view display_name, unsigned model)
	: base_type(std::string(physical_key), display_name.empty() ? "Switch2Kit controller" : display_name),
	  m_guid_index(0), m_guid{}, m_physical_key(physical_key), m_switch2Model(model)
{
	if (!CemuSwitch2Kit::ValidPhysicalKey(physical_key)) throw std::invalid_argument("Invalid Switch2Kit controller identity");
}
#endif

SDLController::~SDLController()
{
#ifdef HAVE_SWITCH2KIT
	m_motion_policy.reset();
#endif
	if (m_controller)
	{
		SDL_RumbleGamepad(m_controller, 0, 0, 0);
		SDL_CloseGamepad(m_controller);
		m_controller = nullptr;
	}
}

bool SDLController::is_connected()
{
	std::scoped_lock lock(m_controller_mutex);
	if (!m_controller)
	{
		return false;
	}

	if (!SDL_GamepadConnected(m_controller))
	{
#ifdef HAVE_SWITCH2KIT
		m_motion_policy.reset();
#endif
		SDL_CloseGamepad(m_controller);
		m_controller = nullptr;
		return false;
	}

	return true;
}

bool SDLController::connect()
{
	std::scoped_lock connectionLock(m_controller_mutex);
	if (is_connected())
		return true;

	m_has_rumble = m_has_gyro = m_has_accel = false;
	auto index = m_provider->get_index(m_guid_index, m_guid);
#ifdef HAVE_SWITCH2KIT
	const auto native_instance = m_physical_key.empty() ? 0 : SDLControllerProvider::FindSwitch2Device(m_physical_key);
	if (!m_physical_key.empty() && !native_instance) return false;
#endif
	int gamepad_count = 0;

	SDL_JoystickID *gamepad_ids = SDL_GetGamepads(&gamepad_count);

#ifdef HAVE_SWITCH2KIT
	if (native_instance)
	{
		index = -1;
		for (int i = 0; gamepad_ids && i < gamepad_count; ++i)
			if (gamepad_ids[i] == native_instance) { index = i; break; }
	}
#endif
	if (!gamepad_ids || index < 0 || index >= gamepad_count)
	{
		SDL_free(gamepad_ids);
		return false;
	}

	m_diid = gamepad_ids[index];
	SDL_free(gamepad_ids);

	m_controller = SDL_OpenGamepad(m_diid);

	if (!m_controller)
		return false;

	if (const char* name = SDL_GetGamepadName(m_controller))
		m_display_name = name;

	for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
		m_buttons[i] = SDL_GamepadHasButton(m_controller, (SDL_GamepadButton)i);
	for (size_t i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i)
		m_axis[i] = SDL_GamepadHasAxis(m_controller, (SDL_GamepadAxis)i);
#ifdef HAVE_SWITCH2KIT
	if (!m_physical_key.empty())
	{
		m_switch2Model = SDL_GetGamepadProduct(m_controller);
		m_has_accel = SDL_GamepadHasSensor(m_controller, SDL_SENSOR_ACCEL);
		m_has_gyro = SDL_GamepadHasSensor(m_controller, SDL_SENSOR_GYRO);
		motion_settings_changed();
	}
	else
#endif
	{
		if (SDL_GamepadHasSensor(m_controller, SDL_SENSOR_ACCEL))
			m_has_accel = SDL_SetGamepadSensorEnabled(m_controller, SDL_SENSOR_ACCEL, true);
		if (SDL_GamepadHasSensor(m_controller, SDL_SENSOR_GYRO))
			m_has_gyro = SDL_SetGamepadSensorEnabled(m_controller, SDL_SENSOR_GYRO, true);
	}
	m_has_rumble = SDL_GetBooleanProperty(SDL_GetGamepadProperties(m_controller),
		SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
#ifdef HAVE_SWITCH2KIT
	if (m_playerIndex)
		SDL_SetGamepadPlayerIndex(m_controller, *m_playerIndex);
#endif
	return true;
}

bool SDLController::TryRumble(float strength)
{
	std::scoped_lock lock(m_controller_mutex);
	if (!is_connected() || !m_has_rumble || !std::isfinite(strength))
		return false;
	const auto amplitude = static_cast<Uint16>(std::clamp(strength, 0.0f, 1.0f) * 0xFFFF);
	return SDL_RumbleGamepad(m_controller, amplitude, amplitude, amplitude ? 5 * 1000 : 0);
}

void SDLController::start_rumble()
{
	TryRumble(get_settings().rumble);
}

void SDLController::stop_rumble()
{
	TryRumble(0);
}

MotionSample SDLController::get_motion_sample()
{
	std::scoped_lock lock(m_controller_mutex);
	if (is_connected() && has_motion())
		return m_provider->motion_sample(m_diid);
	return {};
}

std::string SDLController::get_button_name(uint64 button) const
{
	if (const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton)button))
		return name;
	return base_type::get_button_name(button);
}

ControllerState SDLController::raw_state()
{
	ControllerState result{};
	std::scoped_lock lock(m_controller_mutex);
	if (!is_connected())
		return result;
	for (size_t i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
	{
		if (m_buttons[i] && SDL_GetGamepadButton(m_controller, (SDL_GamepadButton)i))
			result.buttons.SetButtonState(i, true);
	}

	if (m_axis[SDL_GAMEPAD_AXIS_LEFTX])
		result.axis.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_LEFTY])
		result.axis.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHTX])
		result.rotation.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHTY])
		result.rotation.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_LEFT_TRIGGER])
		result.trigger.x = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
	if (m_axis[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER])
		result.trigger.y = (float)SDL_GetGamepadAxis(m_controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;

	return result;
}

#ifdef HAVE_SWITCH2KIT
std::optional<unsigned> SDLController::GetSwitch2Model()
{
	std::scoped_lock lock(m_controller_mutex);
	if (m_physical_key.empty() || !m_switch2Model)
		return {};
	return m_switch2Model;
}

void SDLController::SetPlayerIndex(size_t index)
{
	std::scoped_lock lock(m_controller_mutex);
	if (m_physical_key.empty() || index >= 8)
		return;
	m_playerIndex = static_cast<int>(index);
	if (is_connected())
		SDL_SetGamepadPlayerIndex(m_controller, *m_playerIndex);
}

std::optional<MotionSample> SDLController::get_available_motion_sample()
{
	if (!use_motion()) return {};
	std::scoped_lock lock(m_controller_mutex);
	if (!is_connected()) return {};
	if (m_physical_key.empty()) return get_motion_sample();
	return SDLControllerProvider::AvailableSwitch2Motion(m_diid);
}
void SDLController::motion_settings_changed()
{
	const bool enabled = get_settings().motion;
	std::scoped_lock lock(m_controller_mutex);
	if (m_physical_key.empty() || !m_controller || !SDL_GamepadConnected(m_controller)) return;
	if (!m_motion_policy) m_motion_policy = std::make_unique<Switch2Kit::CemuSensorPolicy>();
	m_motion_policy_error = !m_motion_policy->update(m_controller, enabled && m_has_accel && m_has_gyro);
}
int SDLController::set_motion_profile(const std::string& path)
{
	if (m_physical_key.empty() || path.size() > 4096) return S2K_INVALID_ARGUMENT;
	const int result = path.empty() ? (SDLControllerProvider::RemoveSwitch2MotionProfile(m_physical_key), S2K_OK) :
		SDLControllerProvider::LoadSwitch2MotionProfile(path, m_physical_key);
	std::scoped_lock lock(m_controller_mutex);
	m_motion_profile_error = result != S2K_OK;
	if (result == S2K_OK) m_motion_profile_path = path;
	return result;
}
std::string SDLController::motion_status() const
{
	// Called by the settings UI; no persistent key or raw sensor data is logged.
	{
		std::scoped_lock lock(m_controller_mutex);
		if (m_motion_profile_error) return "Invalid/unreadable calibration; previous selection retained";
		if (m_motion_policy_error) return "Motion sensor enablement failed; disable and reenable Use motion";
		if (!get_settings().motion) return "Motion disabled by Use motion setting";
	}
	return SDLControllerProvider::Switch2MotionStatus(m_physical_key);
}
void SDLController::save(pugi::xml_node& node)
{
	std::scoped_lock lock(m_controller_mutex);
	if (!m_physical_key.empty())
	{
		node.remove_child("rumble");
		node.append_child("rumble").text().set(get_settings().rumble);
		node.remove_child("motion");
		node.append_child("motion").text().set(get_settings().motion);
	}
	node.remove_child("switch2kit_motion_profile");
	if (!m_physical_key.empty() && !m_motion_profile_path.empty())
		node.append_child("switch2kit_motion_profile").text().set(m_motion_profile_path.c_str());
}
void SDLController::load(const pugi::xml_node& node)
{
	if (m_physical_key.empty()) return;
	const auto* value = node.child("switch2kit_motion_profile").child_value();
	const auto length = strnlen(value, 4097);
	if (length > 4096)
	{
		std::scoped_lock lock(m_controller_mutex);
		m_motion_profile_error = true;
	}
	else if (length) set_motion_profile(std::string(value, length));
}
#endif
