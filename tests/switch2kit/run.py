#!/usr/bin/env python3
"""Run controller policy regressions. No Bluetooth, GUI or emulation is faked as hardware.

The small test Pad types contain verbatim enum blocks extracted from the actual
Cemu headers, not duplicated numbers. Complete macOS builds compile the mappings
against the real classes and the production wxWidgets integration.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def run():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=ROOT / 'dependencies/Switch2Kit')
    parser.add_argument('--sdl', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not compiler:
        raise SystemExit('A C++20 compiler is required')
    sdk = args.sdk.resolve()
    if not (sdk / 'Sources/Switch2KitCABI/include/Switch2KitC.h').is_file():
        raise SystemExit('Initialize the Switch2Kit submodule first')
    if args.sdl:
        sdl = args.sdl.resolve() / 'include'
    else:
        sdl = ROOT / 'build-switch2kit/vcpkg_installed'
        headers = list(sdl.glob('*/include/SDL3/SDL_gamepad.h'))
        if len(headers) != 1:
            raise SystemExit('Pass --sdl /path/to/SDL-source or build Cemu first')
        sdl = headers[0].parents[1]
    assert (sdl / 'SDL3/SDL_gamepad.h').is_file()
    with tempfile.TemporaryDirectory(prefix='cemu-s2k-tests-') as directory:
        work = Path(directory)
        enums = []
        for name in ('VPADController', 'ProController', 'ClassicController'):
            source = (ROOT / f'src/input/emulated/{name}.h').read_text()
            match = re.search(r'enum ButtonId\s*\{.*?\};', source, re.S)
            assert match, f'Missing production enum: {name}'
            enums.append(f'struct {name} {{ {match[0]} }};')
        (work / 'ControllerEnums.h').write_text('\n'.join(enums))
        binary = work / 'policies'
        command = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                   '-I' + str(work), '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'src/gui'),
                   '-I' + str(sdl), '-I' + str(sdk / 'Sources/Switch2KitCABI/include'),
                   str(ROOT / 'tests/switch2kit/PolicyTests.cpp'), '-o', str(binary)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run(command, check=True, timeout=120)
        subprocess.run([str(binary)], check=True, timeout=30)
    subprocess.run(['python3', str(ROOT / 'tests/switch2kit/test_wiring.py')], check=True)

if __name__ == '__main__':
    run()
