#!/usr/bin/env python3
"""Source-contract checks supplement, but never substitute for, native builds."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
def text(path): return (ROOT / path).read_text()

class Wiring(unittest.TestCase):
    def test_opt_in_bundle_and_target(self):
        cmake = text('CMakeLists.txt')
        self.assertIn('option(ENABLE_SWITCH2KIT "Use in-process Switch2Kit controllers on macOS 15+" OFF)', cmake)
        self.assertIn('NOT APPLE OR NOT ENABLE_SDL OR NOT MACOS_BUNDLE', cmake)
        self.assertIn('CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 15.0', cmake)
        self.assertIn('${CMAKE_CURRENT_SOURCE_DIR}/dependencies/Switch2Kit', cmake)
        self.assertIn('SWITCH2KIT_BLUETOOTH_USAGE', text('src/resource/MacOSXBundleInfo.plist.in'))
    def test_real_input_loop_and_consent(self):
        provider = text('src/input/api/SDL/SDLControllerProvider.cpp')
        self.assertIn('NativeSession().Discover()', provider)
        self.assertIn('NativeSession().Pump()', provider)
        self.assertLess(provider.index('NativeSession().Shutdown()'), provider.index('SDL_QuitSubSystem'))
        app = text('src/gui/wxgui/CemuApp.cpp')
        self.assertIn('SDLControllerProvider::PumpSDLEvents()', app)
        self.assertNotIn('FindSwitch2Controllers', app)
    def test_enumeration_does_not_cancel_rumble(self):
        provider = text('src/input/api/SDL/SDLControllerProvider.cpp')
        self.assertIn('SDL_GetGamepadProductForID', provider)
        ui = text('src/gui/wxgui/input/InputSettings2.cpp').split('void InputSettings2::RefreshSwitch2Controllers()')[1]
        self.assertNotIn('->connect()', ui)
        self.assertNotIn('set_default_mapping', ui)
        controller = text('src/input/api/SDL/SDLController.cpp')
        self.assertIn('SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN', controller)
    def test_backups_and_stale_dialog(self):
        setup = text('src/gui/wxgui/input/Switch2KitSetup.cpp')
        self.assertIn('ControllerConfigSnapshot(playerIndex) != snapshot', setup)
        self.assertIn('wxNO_DEFAULT', setup)
        self.assertIn('AssignedElsewhere', setup)
        self.assertIn('CommitSetup(', setup)
        self.assertIn('manager.save(playerIndex, name, false)', setup)
        self.assertIn('manager.is_gameprofile_set(playerIndex)', setup)
        manager = text('src/input/InputManager.cpp')
        self.assertIn('FileStream::WriteFileAtomic', manager)
    def test_saved_native_identity(self):
        self.assertIn('starts_with("s2k:")', text('src/input/ControllerFactory.cpp'))
        controller = text('src/input/api/SDL/SDLController.cpp')
        self.assertIn('ValidPhysicalKey', controller)
        self.assertIn('FindSwitch2Device(m_physical_key)', controller)
    def test_motion_is_never_fabricated(self):
        for path in ['src/input/emulated/VPADController.cpp', 'src/input/emulated/WPADController.cpp']:
            self.assertIn('get_motion_data()', text(path))
        provider = text('src/input/api/SDL/SDLControllerProvider.cpp')
        self.assertIn('availableSample()', provider)
        self.assertIn('loadMotionProfile', provider)
    def test_timer_ids_and_rumble_cancel(self):
        ui = text('src/gui/wxgui/input/settings/DefaultControllerSettings.cpp')
        self.assertIn('this, m_timer->GetId()', ui)
        self.assertIn('native->TryRumble(m_settings.rumble)', ui)
        self.assertIn('m_controller->stop_rumble()', ui)
        self.assertIn('m_switch2DevicesChanged.exchange(false)', text('src/gui/wxgui/input/InputSettings2.cpp'))

if __name__ == '__main__': unittest.main(verbosity=2)
