#!/usr/bin/env python3
"""Compile and run the SDK's actual host-file reader on the current platform."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=ROOT / 'dependencies/Switch2Kit')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    include = args.sdk.resolve() / 'Integrations/Emulators'
    if not (include / 'HostFile.hpp').is_file():
        parser.error('Initialize the Switch2Kit submodule first')
    compiler = (shutil.which('cl') if os.name == 'nt' else
                os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++'))
    if not compiler:
        parser.error('A C++20 compiler is required; on Windows initialize the MSVC environment')
    source = ROOT / 'tests/switch2kit/HostFileTests.cpp'
    with tempfile.TemporaryDirectory(prefix='cemu-host-file-') as directory:
        work = Path(directory)
        binary = work / ('host-file.exe' if os.name == 'nt' else 'host-file')
        if os.name == 'nt':
            command = [compiler, '/nologo', '/std:c++20', '/EHsc', '/W4', '/WX', '/utf-8',
                       '/I' + str(include), str(source), '/Fe:' + str(binary)]
        else:
            command = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror',
                       '-I' + str(include), str(source), '-o', str(binary)]
            if args.sanitize:
                command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run(command, cwd=work, check=True, timeout=120)
        subprocess.run([str(binary)], cwd=work, check=True, timeout=30)


if __name__ == '__main__':
    main()
