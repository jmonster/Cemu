#!/usr/bin/env python3
"""Exercise production preprocessor ownership and isolated CMake admission gates.

This does not emulate a controller or claim a native GUI/platform build. Only
includes are removed for preprocessing; the production conditional directives
and method bodies are retained. CMake imports are recorded rather than built.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SDK = ROOT / 'dependencies/Switch2Kit'


def text(path):
    return (ROOT / path).read_text()


def between(source, start, end):
    return source.split(start, 1)[1].split(end, 1)[0]


class DesktopLifecycle(unittest.TestCase):
    def test_single_sdl_owner_for_each_platform_and_feature_mode(self):
        compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
        self.assertTrue(compiler, 'A C++ preprocessor is required')
        for platform in ('MACOS', 'LINUX', 'WINDOWS'):
            for native in (False, True):
                with self.subTest(platform=platform, native=native):
                    flags = [f'-DBOOST_OS_{name}={int(name == platform)}'
                             for name in ('MACOS', 'LINUX', 'WINDOWS')]
                    if native:
                        flags.append('-DHAVE_SWITCH2KIT')

                    def preprocess(path):
                        source = re.sub(r'^\s*#\s*(include|pragma)\b[^\n]*', '',
                                        text(path), flags=re.M)
                        return subprocess.run(
                            [compiler, '-E', '-P', '-x', 'c++', *flags, '-'],
                            input=source, text=True, capture_output=True,
                            check=True, timeout=30).stdout

                    app = preprocess('src/gui/wxgui/CemuApp.cpp')
                    app_header = preprocess('src/gui/wxgui/CemuApp.h')
                    provider = preprocess('src/input/api/SDL/SDLControllerProvider.cpp')
                    header = preprocess('src/input/api/SDL/SDLControllerProvider.h')
                    main = platform == 'MACOS' or native
                    constructor = between(provider, 'SDLControllerProvider::SDLControllerProvider()',
                                          'SDLControllerProvider::~SDLControllerProvider()')
                    destructor = between(provider, 'SDLControllerProvider::~SDLControllerProvider()',
                                         'SDLControllerProvider::get_controllers()')
                    self.assertEqual('s_thread = std::thread' in constructor, not main)
                    self.assertEqual('s_thread.join()' in destructor, not main)
                    self.assertEqual('void SDLControllerProvider::PumpSDLEvents()' in provider, main)
                    self.assertEqual('SDLControllerProvider::PumpSDLEvents();' in app, main)
                    self.assertEqual('void OnSDLEventPumpTimer(' in app_header, main)
                    self.assertEqual('static void PumpSDLEvents();' in header, main)
                    # No duplicated public/private SDL lifecycle declarations.
                    self.assertEqual(header.count('static void InitSDL();'), 1)
                    self.assertEqual(header.count('static void ShutdownSDL();'), 1)
                    self.assertEqual('NativeSession().Pump()' in provider, native)
                    if main:
                        startup = between(app, 'bool CemuApp::OnInit()', 'int CemuApp::OnExit()')
                        shutdown = between(app, 'int CemuApp::OnExit()',
                                           'void CemuApp::OnSDLEventPumpTimer(')
                        self.assertLess(startup.index('SDLControllerProvider::InitSDL();'),
                                        startup.index('CemuCommonInit();'))
                        self.assertIn('m_sdlEventPumpTimer->Start(5, wxTIMER_CONTINUOUS)', startup)
                        self.assertLess(shutdown.index('m_sdlEventPumpTimer->Stop()'),
                                        shutdown.index('InputManager::instance().Shutdown()'))
                        self.assertLess(shutdown.index('InputManager::instance().Shutdown()'),
                                        shutdown.index('SDLControllerProvider::ShutdownSDL()'))
                    else:
                        self.assertNotIn('SDLControllerProvider::InitSDL();', app)
                        self.assertNotIn('SDLControllerProvider::ShutdownSDL();', app)
                        self.assertIn('SDL_WaitEvent(&event)', provider)

    def test_static_adapter_uses_the_hosts_crt_in_both_build_configurations(self):
        # Include the complete production input CMake file, not a copied setter.
        # These configure-only targets record CRT choices; the native Windows
        # application workflow separately compiles and links the real binaries.
        for enabled in (False, True):
            for configuration in ('Debug', 'Release'):
                with self.subTest(enabled=enabled, configuration=configuration):
                    with tempfile.TemporaryDirectory(prefix='cemu-crt-policy-') as directory:
                        root = Path(directory)
                        (root / 'fixture.cpp').write_text('int adapter_fixture;\n')
                        source = '''cmake_minimum_required(VERSION 3.24)
project(CemuCRTPolicy LANGUAGES CXX)
set(MSVC TRUE)
set(ENABLE_SDL ON)
function(cemu_use_precompiled_header)
endfunction()
add_library(CemuCommon INTERFACE)
add_library(CemuGui INTERFACE)
add_library(SDL3::SDL3 INTERFACE IMPORTED)
'''
                        source += f'set(ENABLE_SWITCH2KIT {"ON" if enabled else "OFF"})\n'
                        if enabled:
                            source += '''add_library(Switch2KitSDL3 STATIC fixture.cpp)
add_library(Switch2Kit::SDL3 ALIAS Switch2KitSDL3)
set_property(TARGET Switch2KitSDL3 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
'''
                        source += f'add_subdirectory("{(ROOT / "src/input").as_posix()}" input)\n'
                        source += 'file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/host-crt.txt" CONTENT "$<TARGET_GENEX_EVAL:CemuInput,$<TARGET_PROPERTY:CemuInput,MSVC_RUNTIME_LIBRARY>>")\n'
                        if enabled:
                            source += 'file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/adapter-crt.txt" CONTENT "$<TARGET_GENEX_EVAL:Switch2KitSDL3,$<TARGET_PROPERTY:Switch2KitSDL3,MSVC_RUNTIME_LIBRARY>>")\n'
                        else:
                            source += '''if(TARGET Switch2KitSDL3)
  message(FATAL_ERROR "The disabled input target must not acquire the adapter")
endif()
'''
                        (root / 'CMakeLists.txt').write_text(source)
                        result = subprocess.run(
                            ['cmake', '-S', str(root), '-B', str(root / 'build'),
                             '-G', 'Ninja', f'-DCMAKE_BUILD_TYPE={configuration}'],
                            text=True, capture_output=True, timeout=30)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        expected = 'MultiThreadedDebug' if configuration == 'Debug' else 'MultiThreaded'
                        self.assertEqual((root / 'build/host-crt.txt').read_text(), expected)
                        adapter = root / 'build/adapter-crt.txt'
                        self.assertEqual(adapter.exists(), enabled)
                        if enabled:
                            self.assertEqual(adapter.read_text(), expected)

    def test_production_cmake_gates(self):
        cmake = shutil.which('cmake')
        self.assertTrue(cmake, 'CMake is required')
        policy = 'if(ENABLE_SWITCH2KIT)' + text('CMakeLists.txt').split(
            'if(ENABLE_SWITCH2KIT)', 1)[1].split('\n# glslang', 1)[0]
        # platform, native, SDL, bundle, deployment, SDK exists, SDK reached, configure succeeds
        cases = [
            ('Darwin', False, False, False, '13.4', True, False, True),
            ('FreeBSD', False, False, False, '', False, False, True),
            ('Darwin', True, True, True, '15.0', True, True, True),
            ('Darwin', True, True, False, '15.0', True, False, False),
            ('Darwin', True, True, True, '13.4', True, False, False),
            ('Linux', True, True, False, '', True, True, True),
            ('Windows', True, True, False, '', True, True, True),
            ('Windows', True, False, False, '', True, False, False),
            ('Windows', False, False, False, '', False, False, True),
            ('Linux', True, False, False, '', True, False, False),
            ('Linux', True, True, False, '', False, False, False),
            ('FreeBSD', True, True, False, '', True, False, False),
        ]
        with tempfile.TemporaryDirectory(prefix='cemu-cmake-policy-') as directory:
            script = Path(directory) / 'policy.cmake'
            sdk = SDK
            for platform, native, sdl, bundle, version, exists, admitted, valid in cases:
                with self.subTest(platform=platform, native=native, sdl=sdl,
                                  bundle=bundle, version=version, sdk=exists):
                    values = dict(APPLE=platform == 'Darwin', WIN32=platform == 'Windows',
                                  CMAKE_SYSTEM_NAME=platform, ENABLE_SWITCH2KIT=native,
                                  ENABLE_SDL=sdl, MACOS_BUNDLE=bundle,
                                  CMAKE_OSX_DEPLOYMENT_TARGET=version,
                                  SWITCH2KIT_SOURCE_DIR=str(sdk if exists else Path(directory)/'absent'))
                    setup = ''.join(f'set({key} "{value}")\n' for key, value in values.items())
                    script.write_text(setup + '''
function(add_subdirectory)
    message(STATUS "SWITCH2KIT_ADMITTED")
endfunction()
function(add_compile_definitions)
endfunction()
function(include_directories)
endfunction()
''' + policy)
                    result = subprocess.run([cmake, '-P', str(script)], text=True,
                                            capture_output=True, timeout=30)
                    output = result.stdout + result.stderr
                    self.assertEqual(result.returncode == 0, valid, output)
                    self.assertEqual('SWITCH2KIT_ADMITTED' in output, admitted, output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=SDK)
    args = parser.parse_args()
    SDK = args.sdk.resolve()
    unittest.main(argv=[__file__], verbosity=2)
