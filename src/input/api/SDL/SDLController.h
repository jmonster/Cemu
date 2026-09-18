#pragma once

#include "input/api/Controller.h"
#include "input/api/SDL/SDLControllerProvider.h"

#include <SDL3/SDL_gamepad.h>

#ifdef HAVE_SWITCH2KIT
namespace Switch2Kit { class CemuSensorPolicy; }
#endif

class SDLController : public Controller<SDLControllerProvider>
{
public:
	SDLController(const SDL_GUID& guid, size_t guid_index);
	SDLController(const SDL_GUID& guid, size_t guid_index, std::string_view display_name);
	
#ifdef HAVE_SWITCH2KIT
	SDLController(std::string_view physical_key, std::string_view display_name, unsigned model = 0);
	bool IsSwitch2Controller() const { return !m_physical_key.empty(); }
	std::optional<unsigned> GetSwitch2Model();
	void SetPlayerIndex(size_t index);
	int set_motion_profile(const std::string& path);
	std::string motion_status() const;
	std::optional<MotionSample> get_available_motion_sample() override;
	void save(pugi::xml_node& node) override;
	void load(const pugi::xml_node& node) override;
#endif
	~SDLController() override;
	
	std::string_view api_name() const override
	{
		static_assert(to_string(InputAPI::SDLController) == "SDLController");
		return to_string(InputAPI::SDLController);
	}
	InputAPI::Type api() const override { return InputAPI::SDLController; }

	bool is_connected() override;
	bool connect() override;
	
	bool has_motion() override { std::scoped_lock lock(m_controller_mutex); return m_has_gyro && m_has_accel; }
	bool has_rumble() override { std::scoped_lock lock(m_controller_mutex); return m_has_rumble; }
	
	// Reports command acceptance, not a physical hardware measurement.
	bool TryRumble(float strength);
	void start_rumble() override;
	void stop_rumble() override;

	MotionSample get_motion_sample() override;

	std::string get_button_name(uint64 button) const override;
	const SDL_GUID& get_guid() const { return m_guid; }

	constexpr static SDL_GUID kLeftJoyCon{ 0x03, 0x00, 0x00, 0x00, 0x7e, 0x05, 0x00, 0x00, 0x06, 0x20, 0x00, 0x00, 0x00, 0x00,0x68 ,0x00 };
	constexpr static SDL_GUID kRightJoyCon{ 0x03, 0x00, 0x00, 0x00, 0x7e, 0x05, 0x00, 0x00, 0x07, 0x20, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00 };
	constexpr static SDL_GUID kSwitchProController{ 0x03, 0x00, 0x00, 0x00, 0x7e, 0x05, 0x00, 0x00, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00 };

protected:
	ControllerState raw_state() override;

private:
	inline static SDL_GUID kEmptyGUID{};

	size_t m_guid_index;
	SDL_GUID m_guid;
#ifdef HAVE_SWITCH2KIT
	void motion_settings_changed() override;
	std::string m_physical_key;
	std::string m_motion_profile_path;
	bool m_motion_profile_error = false;
	bool m_motion_policy_error = false;
	std::optional<int> m_playerIndex;
	unsigned m_switch2Model = 0;
	std::unique_ptr<Switch2Kit::CemuSensorPolicy> m_motion_policy;
#endif
	mutable std::recursive_mutex m_controller_mutex;
	SDL_Gamepad* m_controller = nullptr;
	SDL_JoystickID m_diid = -1;

	bool m_has_gyro = false;
	bool m_has_accel = false;
	bool m_has_rumble = false;
	
	std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> m_buttons{};
	std::array<bool, SDL_GAMEPAD_AXIS_COUNT> m_axis{};
};

