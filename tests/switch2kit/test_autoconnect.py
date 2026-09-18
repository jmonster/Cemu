#!/usr/bin/env python3
"""Execute the production automatic-connection session and configuration policies.

Only the SDK host and atomic writer are test boundaries. No controller hardware
or full application build is represented by these tests.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--include', action='append', default=[], type=Path)
    args = parser.parse_args()
    compiler = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not compiler:
        raise SystemExit('A C++20 compiler is required')
    with tempfile.TemporaryDirectory(prefix='cemu-autoconnect-') as directory:
        work = Path(directory)
        binary = work / 'autoconnect'
        command = [compiler, '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                   '-I' + str(ROOT / 'src')]
        command += ['-I' + str(path.resolve()) for path in args.include]
        command += [str(ROOT / 'tests/switch2kit/AutoConnectTests.cpp'), '-o', str(binary)]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run(command, check=True, timeout=120)
        subprocess.run([str(binary), str(work)], check=True, timeout=30)


if __name__ == '__main__':
    main()
