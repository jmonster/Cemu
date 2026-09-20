#!/usr/bin/env python3
"""Source integration guards, not GUI, Bluetooth, or full-build acceptance."""
import argparse
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
SDK = ROOT / 'dependencies/Switch2Kit'
PIN = 'd9129e3876f0d68aa7d13395dcff8e2abb38d609'


def source(path):
    return (ROOT / path).read_text()


def between(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]


class AutoConnectWiring(unittest.TestCase):
    def test_main_loop_start_is_one_shot_and_after_sdl(self):
        provider = source('src/input/api/SDL/SDLControllerProvider.cpp')
        startup = between(provider, 'void SDLControllerProvider::InitSDL()',
                          'void SDLControllerProvider::ShutdownSDL()')
        self.assertLess(startup.index('SDL_InitSubSystem('), startup.index('LoadAutoConnect('))
        self.assertIn('#ifdef HAVE_SWITCH2KIT\n\tNativeSession().LoadAutoConnect', startup)
        self.assertIn('wxTheApp->CallAfter([] { NativeSession().StartOnce(); });', startup)
        self.assertEqual(provider.count('NativeSession().StartOnce()'), 1)
        app = source('src/gui/wxgui/CemuApp.cpp')
        self.assertLess(app.index('DeterminePaths(failedWriteAccess)'), app.index('SDLControllerProvider::InitSDL()'))

    def test_input_polling_neither_starts_nor_reads_preferences(self):
        provider = source('src/input/api/SDL/SDLControllerProvider.cpp')
        polling = between(provider, 'void SDLControllerProvider::PumpSDLEvents()',
                          'void SDLControllerProvider::HandleSDLEvent(')
        self.assertIn('NativeSession().Pump()', polling)
        for forbidden in ('StartOnce', 'Discover(', 'AutoConnect', 'GetConfigPath', 'read_ini'):
            self.assertNotIn(forbidden, polling)
        session = source('src/input/api/SDL/Switch2KitSession.h')
        polling = between(session, '\tint Pump()', '\tvoid Shutdown()')
        self.assertIn('m_enabled ? m_host.pump() : 0', polling)
        for forbidden in ('start(', 'discover(', 'setAutomaticDiscovery', 'LoadAutoConnect'):
            self.assertNotIn(forbidden, polling)

    def test_checkbox_reports_saved_choice_without_assigning_controllers(self):
        ui = source('src/gui/wxgui/input/InputSettings2.cpp')
        self.assertIn('Automatically connect Switch 2 controllers', ui)
        handler = between(ui, 'auto_connect->Bind(wxEVT_CHECKBOX', 'find->Bind(wxEVT_BUTTON')
        self.assertIn('SetSwitch2AutoConnect(event.IsChecked())', handler)
        self.assertIn('auto_connect->SetValue(SDLControllerProvider::Switch2AutoConnect())', handler)
        self.assertIn('wxMessageBox', handler)
        for forbidden in ('ApplySwitch2KitSetup', 'set_default_mapping', 'set_controller', 'load_controller'):
            self.assertNotIn(forbidden, handler)
        timer = between(ui, 'm_switch2Timer = new wxTimer(this);', 'm_switch2Timer->Start(500)')
        self.assertIn('Switch2AutoConnect()', timer)
        self.assertNotIn('FindSwitch2Controllers', timer)
        self.assertNotIn('SetSwitch2AutoConnect', timer)

    def test_persistence_is_separate_atomic_and_errors_precede_snapshot(self):
        provider = source('src/input/api/SDL/SDLControllerProvider.cpp')
        save = between(provider, 'static bool SaveSwitch2AutoConnect(',
                       'int SDLControllerProvider::FindSwitch2Controllers()')
        self.assertIn('AutoConnectConfig::Save', save)
        self.assertIn('ActiveSettings::GetConfigPath("Switch2Kit.ini")', save)
        self.assertIn('FileStream::WriteFileAtomic', save)
        self.assertNotIn('controllerProfiles', save)
        status = between(provider, 'std::string SDLControllerProvider::Switch2ControllerStatus()',
                         'SDL_JoystickID SDLControllerProvider::FindSwitch2Device(')
        self.assertLess(status.index('session.Error()'), status.index('nativeControllers().snapshot()'))

    def test_matching_sdk_exposes_policy_and_start(self):
        revision = subprocess.check_output(['git', '-C', str(SDK), 'rev-parse', 'HEAD'], text=True).strip()
        self.assertEqual(revision, PIN, 'Update the reviewed SDK pin and its tests together')
        host = (SDK / 'Integrations/Emulators/SDLHost.hpp').read_text()
        setter = between(host, 'S2KResult setAutomaticDiscovery(bool enabled)', 'S2KResult start()')
        self.assertIn('Guard lock(mutex_)', setter)
        self.assertIn('s2k_set_automatic_discovery(context_, enabled ? 1u : 0u)', setter)
        self.assertNotIn('s2k_start(', setter)
        start = between(host, 'S2KResult start()', 'S2KResult discover(')
        self.assertIn('s2k_start(context_)', start)
        self.assertNotIn('s2k_discover(', start)
        self.assertIn('s2k_set_automatic_discovery(',
                      (SDK / 'Sources/Switch2KitCABI/include/Switch2KitC.h').read_text())

    def test_native_build_and_backend_disabled_gates_are_retained(self):
        workflow = source('.github/workflows/native-switch2kit.yml')
        for gate in ('arch: arm64', 'arch: x86_64', 'tests/switch2kit/run.py --sanitize',
                     'verify-bundle.py', 'tests/emulator-launch/verify.py',
                     '-DENABLE_SWITCH2KIT=OFF', 'tests/emulator-host/verify.sh'):
            self.assertIn(gate, workflow)
        runner = source('tests/switch2kit/run.py')
        self.assertIn('test_autoconnect.py', runner)
        self.assertIn('test_autoconnect_wiring.py', runner)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=SDK)
    args = parser.parse_args()
    SDK = args.sdk.resolve()
    unittest.main(argv=[__file__])
