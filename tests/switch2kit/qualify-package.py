#!/usr/bin/env python3
"""Qualify a built Cemu package's loader; never opens a game or starts Bluetooth."""
import argparse
import json
import os
from pathlib import Path
import plistlib
import re
import subprocess
import sys
import tarfile
import tempfile


def run(command, env=None, cwd=None):
    result = subprocess.run([str(arg) for arg in command], env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30, cwd=cwd)
    if result.returncode:
        raise RuntimeError(f'{command[0]} exited {result.returncode}: {result.stdout}')
    return result.stdout


def require(path):
    if not path.exists():
        raise RuntimeError(f'Missing package component: {path}')



def inspect_macos(exe, package):
    # Inspect every bundled Mach-O, including alternate copies reachable via an
    # allowed in-bundle LC_RPATH. Never assume Frameworks is dyld's first choice.
    magic = {bytes.fromhex(value) for value in (
        'feedface', 'feedfacf', 'cefaedfe', 'cffaedfe',
        'cafebabe', 'bebafeca', 'cafebabf', 'bfbafeca')}
    pending = [exe]
    for candidate in package.rglob('*'):
        if candidate.is_file():
            with candidate.open('rb') as source:
                if source.read(4) in magic:
                    pending.append(candidate)
    visited = set()
    while pending:
        image = pending.pop().resolve()
        if image in visited:
            continue
        visited.add(image)
        commands = run(['/usr/bin/otool', '-arch', os.uname().machine, '-l', image])
        rpaths = re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset \d+\)', commands)
        if commands.count('cmd LC_RPATH') != len(rpaths):
            raise RuntimeError(f'Unrecognized macOS runtime search path: {image}')
        for rpath in rpaths:
            if rpath.startswith(('/usr/lib/', '/System/Library/')):
                continue
            if rpath == '@loader_path':
                resolved = image.parent
            elif rpath == '@executable_path':
                resolved = exe.parent
            elif rpath.startswith('@loader_path/'):
                resolved = image.parent / rpath[len('@loader_path/'):]
            elif rpath.startswith('@executable_path/'):
                resolved = exe.parent / rpath[len('@executable_path/'):]
            else:
                raise RuntimeError(f'Non-relocatable macOS runtime search path: {rpath}')
            if not resolved.resolve().is_relative_to(package.resolve()):
                raise RuntimeError(f'macOS runtime search path escapes package: {rpath}')
        for line in run(['/usr/bin/otool', '-arch', os.uname().machine, '-L', image]).splitlines()[1:]:
            name = line.strip().split(' (compatibility version', 1)[0]
            if name.startswith(('/usr/lib/', '/System/Library/')):
                continue  # OS libraries may live only in the dyld shared cache.
            if name.startswith('@rpath/'):
                dependency = package / 'Contents/Frameworks' / name[len('@rpath/'):]
            elif name.startswith('@loader_path/'):
                dependency = image.parent / name[len('@loader_path/'):]
            elif name.startswith('@executable_path/'):
                dependency = exe.parent / name[len('@executable_path/'):]
            else:
                raise RuntimeError(f'Non-relocatable macOS dependency: {image}: {name}')
            require(dependency)
            if not dependency.resolve().is_relative_to(package.resolve()):
                raise RuntimeError(f'macOS dependency escapes package: {dependency}')
            pending.append(dependency)


def inspect_windows(exe, package, dumpbin):
    pending = [exe]
    visited = set()
    local = {p.name.lower(): p for p in package.iterdir() if p.is_file()}
    system = Path(os.environ['SystemRoot']) / 'System32'
    while pending:
        image = pending.pop()
        if image in visited:
            continue
        visited.add(image)
        output = run([dumpbin, '/DEPENDENTS', image])
        names = re.findall(r'^\s+([^\s]+\.dll)\s*$', output, re.M | re.I)
        for name in names:
            if any(char in name for char in ('/', '\\', ':')):
                raise RuntimeError(f'Non-relocatable Windows import: {name}')
            key = name.lower()
            if key in local:
                pending.append(local[key])
            elif key.startswith(('swift', 'foundation', '_foundation', 'dispatch', 'blocksruntime', 'switch2kit')):
                raise RuntimeError(f'Windows runtime missing from package: {name}')
            elif key.startswith(('api-ms-win-', 'ext-ms-win-')):
                continue  # Windows API-set contracts, resolved by the OS loader.
            elif not (system / name).is_file():
                raise RuntimeError(f'Windows dependency missing from package/OS: {name}')


def inspect_linux(linkage, facade, package):
    resolved = re.search(r'libSwitch2KitC\.so => (.+?) \(', linkage)
    if 'not found' in linkage or not resolved or Path(resolved[1]).resolve() != facade.resolve():
        raise RuntimeError(f'Invalid installed loader closure: {linkage}')
    runtime_prefixes = ('libswift', 'libFoundation', 'lib_Foundation', 'libdispatch', 'libBlocksRuntime')
    system_roots = [Path(p).resolve() for p in ('/lib', '/lib64', '/usr/lib', '/usr/lib64')]
    dependencies = re.findall(r'^\s*(\S+)\s+=>\s+(.+?)\s+\(0x[0-9a-f]+\)', linkage, re.M)
    dependencies += [(Path(p).name, p) for p in re.findall(r'^\s*(/.*?)\s+\(0x[0-9a-f]+\)', linkage, re.M)]
    for name, value in dependencies:
        path = Path(value).resolve(strict=True)
        if path.is_relative_to(package.resolve()):
            continue
        if name.startswith(runtime_prefixes) or not any(path.is_relative_to(p) for p in system_roots):
            raise RuntimeError(f'Runtime dependency escapes package: {name}: {path}')


def qualify(root, destination):
    destination.mkdir(parents=True, exist_ok=True)
    # Do not leave a previous successful result behind when a repeat check fails.
    (destination / 'qualification.json').write_text('{"status": "started"}\n')
    windows = sys.platform == 'win32'
    mac = sys.platform == 'darwin'
    build = root / ('build-switch2kit-windows' if windows else 'build-switch2kit')
    cache = (build / 'CMakeCache.txt').read_text()
    if not re.search(r'^ENABLE_SWITCH2KIT:BOOL=ON$', cache, re.M):
        raise RuntimeError('Qualification requires ENABLE_SWITCH2KIT=ON')
    source = root / 'bin' if windows else (root / 'bin/Cemu_release.app' if mac else build / 'install')
    relative_exe = Path('Cemu_release.exe' if windows else 'Contents/MacOS/Cemu_release' if mac else 'bin/Cemu_release')
    require(source / relative_exe)
    # Archive and extract to a path outside the checkout, preserving executable modes.
    with tempfile.TemporaryDirectory(prefix='cemu-package-') as temporary:
        temporary = Path(temporary)
        archive = temporary / 'package.tar.gz'
        with tarfile.open(archive, 'w:gz') as output:
            output.add(source, arcname='package')
        with tarfile.open(archive) as package:
            package.extractall(temporary / 'extracted', filter='data')
        moved = temporary / 'extracted/package'
        exe = moved / relative_exe
        notices = moved / ('Switch2KitNotices' if windows else 'Contents/Resources/Switch2KitNotices' if mac else 'share/Switch2KitNotices')
        notice_names = ['CREDITS.md', 'LICENSES/MIT-trevlars.txt', 'LICENSES/SDL-zlib.txt']
        if not mac:
            notice_names += ['SwiftRuntime/LICENSE.txt', 'SwiftRuntime/ICU.txt']
        for name in notice_names:
            notice = notices / name
            require(notice)
            if not notice.is_file() or notice.is_symlink() or notice.stat().st_size == 0:
                raise RuntimeError(f'Invalid package notice: {notice}')
        resources = moved / ('Contents/SharedSupport' if mac else '' if windows else 'share/Cemu')
        require(resources / 'gameProfiles')
        require(resources / 'resources')
        env = os.environ.copy()
        for name in list(env):
            if name.startswith(('LD_', 'DYLD_')):
                env.pop(name)
        if windows:
            facade = moved / 'Switch2KitC.dll'
            vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = run([vswhere, '-latest', '-products', '*', '-version', '[17.0,18.0)', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath']).strip()
            candidates = sorted(Path(vs).glob('VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe'))
            if not candidates:
                raise RuntimeError('VS2022 dumpbin missing')
            imports = run([candidates[-1], '/DEPENDENTS', exe])
            if 'switch2kitc.dll' not in imports.lower():
                raise RuntimeError('Cemu does not import Switch2KitC.dll')
            inspect_windows(exe, moved, candidates[-1])
            env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32') + ';' + os.environ['SystemRoot']
        elif mac:
            facade = moved / 'Contents/Frameworks/libSwitch2KitC.dylib'
            plist = plistlib.loads((moved / 'Contents/Info.plist').read_bytes())
            if not plist.get('NSBluetoothAlwaysUsageDescription') or int(plist['LSMinimumSystemVersion'].split('.')[0]) < 15:
                raise RuntimeError('Missing Bluetooth description or macOS 15 minimum')
            imports = run(['/usr/bin/otool', '-L', exe])
            if '@rpath/libSwitch2KitC.dylib' not in imports:
                raise RuntimeError('Cemu does not import the relocatable facade')
            inspect_macos(exe, moved)
            run(['/usr/bin/codesign', '--verify', '--deep', '--strict', moved])
            env['PATH'] = '/usr/bin:/bin:/usr/sbin:/sbin'
        else:
            libraries = list(moved.glob('lib*/libSwitch2KitC.so'))
            if len(libraries) != 1:
                raise RuntimeError('Expected exactly one installed facade')
            facade = libraries[0]
            imports = run(['readelf', '-d', exe])
            if not re.search(r'\(NEEDED\).*\[libSwitch2KitC\.so\]', imports):
                raise RuntimeError('Cemu does not import the relocatable facade')
            require(moved / 'share/Cemu/gameProfiles')
            require(moved / 'share/Cemu/resources')
            env['PATH'] = '/usr/bin:/bin'
        require(facade)
        # Hide both the original app and Swift/vcpkg build tree. A successful
        # loader run must not depend on absolute build paths or developer PATH.
        hidden = []
        try:
            for original in (build, root / 'bin'):
                unavailable = original.with_name(original.name + '.qualification-unavailable')
                if unavailable.exists():
                    raise RuntimeError(f'Refusing to overwrite {unavailable}')
                original.rename(unavailable)
                hidden.append((original, unavailable))
            if not windows and not mac:
                linkage = run(['/usr/bin/ldd', exe], env)
                inspect_linux(linkage, facade, moved)
            # --version exits before application/controller initialization.
            version = run([exe, '--version'], env, exe.parent)
        finally:
            for original, unavailable in reversed(hidden):
                unavailable.rename(original)
        report = {
            'status': 'passed',
            'cemu_commit': run(['git', '-C', root, 'rev-parse', 'HEAD']).strip(),
            'sdk_commit': run(['git', '-C', root / 'dependencies/Switch2Kit', 'rev-parse', 'HEAD']).strip(),
            'platform': sys.platform, 'version_output': version.strip(),
            'switch2kit_enabled': True, 'archive_extracted': True,
            'relocated_loader_passed': True, 'hardware_gameplay_validated': False,
        }
        (destination / 'qualification.json').write_text(json.dumps(report, indent=2) + '\n')
        (destination / 'imports.txt').write_text(imports)
        print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--output', type=Path, default=Path('qualification'))
    args = parser.parse_args()
    qualify(args.root.resolve(), args.output.resolve())
