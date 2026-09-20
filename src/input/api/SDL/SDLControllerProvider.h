#pragma once
#include <SDL3/SDL_joystick.h>
#include "input/motion/MotionHandler.h"
#include "input/api/ControllerProvider.h"

static bool operator==(const SDL_GUID& g1, const SDL_GUID& g2)
{
	return memcmp(&g1, &g2, sizeof(SDL_GUID)) == 0;
}

class SDLControllerProvider : public ControllerProviderBase
{
	friend class SDLController;
public:
	SDLControllerProvider();
	~SDLControllerProvider();

	inline static InputAPI::Type kAPIType = InputAPI::SDLController;
	InputAPI::Type api() const override { return kAPIType; }

	std::vector<std::shared_ptr<ControllerBase>> get_controllers() override;
	
	int get_index(size_t guid_index, const SDL_GUID& guid) const;

	MotionSample motion_sample(SDL_JoystickID diid);
#ifdef HAVE_SWITCH2KIT
	static int FindSwitch2Controllers();
	static int DisconnectSwitch2Controllers();
	static bool Switch2AutoConnect();
	static int SetSwitch2AutoConnect(bool enabled);
	static std::string Switch2ControllerStatus();
	static SDL_JoystickID FindSwitch2Device(std::string_view key);
	static int LoadSwitch2MotionProfile(const std::string& path, const std::string& key);
	static void RemoveSwitch2MotionProfile(const std::string& key);
	static std::string Switch2MotionStatus(const std::string& key);
	static std::optional<MotionSample> AvailableSwitch2Motion(SDL_JoystickID id);
#endif

	// Main-loop ownership on macOS and whenever native Switch2Kit is enabled.
#if BOOST_OS_MACOS || defined(HAVE_SWITCH2KIT)
	static void InitSDL();
	static void ShutdownSDL();
	static void PumpSDLEvents();
#endif

private:
	void event_thread();
	static void HandleSDLEvent(union SDL_Event& event);
#if !BOOST_OS_MACOS && !defined(HAVE_SWITCH2KIT)
	static void InitSDL();
	static void ShutdownSDL();
#endif

	// there is only one SDL instance, for this reason all of our state can be static
	inline static std::atomic_int s_initCount{0};
	inline static std::shared_mutex s_mutex;
	inline static std::atomic_bool s_running = false;
	inline static std::thread s_thread;

	struct MotionInfoTracking
	{
		uint64 lastTimestampGyro{};
		uint64 lastTimestampAccel{};
		uint64 lastTimestampIntegrate{};
		bool hasGyro{};
		bool hasAcc{};
		glm::vec3 gyro{};
		glm::vec3 acc{};
	};

	struct MotionState;
	static std::unordered_map<SDL_JoystickID, MotionState> s_motion_states;
};
