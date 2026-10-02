"""Linux ELF fixtures test the verifier, not a native Cemu application build."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('qualification', Path(__file__).with_name('qualify-package.py'))
qualification = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qualification)


@unittest.skipUnless(sys.platform == 'linux', 'ELF fixtures require Linux')
class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.build = self.root / 'build-switch2kit'
        self.package = self.build / 'install'
        for relative in ('bin', 'lib', 'share/Cemu/gameProfiles', 'share/Cemu/resources', 'share/Switch2KitNotices/LICENSES'):
            (self.package / relative).mkdir(parents=True)
        (self.root / 'bin').mkdir()
        for name in ('CREDITS.md', 'LICENSES/MIT-trevlars.txt', 'LICENSES/SDL-zlib.txt', 'SwiftRuntime/LICENSE.txt', 'SwiftRuntime/ICU.txt'):
            notice = self.package / 'share/Switch2KitNotices' / name
            notice.parent.mkdir(parents=True, exist_ok=True)
            notice.write_text('Fixture')
        (self.build / 'CMakeCache.txt').write_text('ENABLE_SWITCH2KIT:BOOL=ON\n')
        self.command(['git', 'init', '-q', self.root])
        self.command(['git', '-C', self.root, '-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', 'commit', '-qm', 'fixture', '--allow-empty'])
        (self.root / 'dependencies/Switch2Kit').mkdir(parents=True)
        (self.root / 'library.c').write_text('int fixture(void) { return 0; }\n')
        (self.root / 'app.c').write_text('int fixture(void); int main(void) { return fixture(); }\n')
        self.command(['cc', '-shared', '-fPIC', '-Wl,-soname,libSwitch2KitC.so', self.root / 'library.c', '-o', self.package / 'lib/libSwitch2KitC.so'])
        self.link('$ORIGIN/../lib')

    def command(self, args):
        subprocess.run([str(a) for a in args], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def link(self, rpath):
        self.command(['cc', self.root / 'app.c', '-L' + str(self.package / 'lib'), '-lSwitch2KitC', '-Wl,-rpath,' + rpath, '-o', self.package / 'bin/Cemu_release'])

    def verify(self):
        qualification.qualify(self.root, self.root / 'result')

    def test_relocated_loader_passes(self):
        self.verify()
        self.assertTrue(json.loads((self.root / 'result/qualification.json').read_text())['relocated_loader_passed'])
        self.assertTrue(self.build.exists())
        self.assertTrue((self.root / 'bin').exists())

    def test_absolute_build_rpath_fails_and_restores(self):
        self.link(str(self.package / 'lib'))
        with self.assertRaisesRegex(RuntimeError, 'Invalid installed loader closure'):
            self.verify()
        self.assertTrue(self.build.exists())
        self.assertTrue((self.root / 'bin').exists())
        self.assertEqual(json.loads((self.root / 'result/qualification.json').read_text())['status'], 'started')

    def test_application_failure_restores_originals(self):
        (self.root / 'app.c').write_text('int fixture(void); int main(void) { return fixture() + 7; }\n')
        self.link('$ORIGIN/../lib')
        with self.assertRaisesRegex(RuntimeError, 'exited 7'):
            self.verify()
        self.assertTrue(self.build.exists())
        self.assertTrue((self.root / 'bin').exists())

    def test_unlinked_application_rejected(self):
        (self.root / 'app.c').write_text('int main(void) { return 0; }\n')
        self.command(['cc', self.root / 'app.c', '-o', self.package / 'bin/Cemu_release'])
        with self.assertRaisesRegex(RuntimeError, 'does not import'):
            self.verify()

    def test_external_transitive_runtime_rejected(self):
        external = self.root / 'external-runtime'
        external.mkdir()
        (self.root / 'external.c').write_text('int external(void) { return 0; }\n')
        self.command(['cc', '-shared', '-fPIC', '-Wl,-soname,libswiftExternal.so', self.root / 'external.c', '-o', external / 'libswiftExternal.so'])
        (self.root / 'library.c').write_text('int external(void); int fixture(void) { return external(); }\n')
        self.command(['cc', '-shared', '-fPIC', '-Wl,-soname,libSwitch2KitC.so', self.root / 'library.c', '-L' + str(external), '-lswiftExternal', '-Wl,-rpath,' + str(external), '-o', self.package / 'lib/libSwitch2KitC.so'])
        self.link('$ORIGIN/../lib')
        with self.assertRaisesRegex(RuntimeError, 'Runtime dependency escapes package'):
            self.verify()
        self.assertTrue(self.build.exists())

    def test_missing_resources_rejected(self):
        (self.package / 'share/Cemu/resources').rmdir()
        with self.assertRaisesRegex(RuntimeError, 'Missing package component'):
            self.verify()

    def test_disabled_backend_rejected(self):
        (self.build / 'CMakeCache.txt').write_text('ENABLE_SWITCH2KIT:BOOL=OFF\n')
        with self.assertRaisesRegex(RuntimeError, 'ENABLE_SWITCH2KIT=ON'):
            self.verify()

    def test_missing_notice_rejected(self):
        (self.package / 'share/Switch2KitNotices/CREDITS.md').unlink()
        with self.assertRaisesRegex(RuntimeError, 'Missing package component'):
            self.verify()

    def test_repeat_failure_does_not_preserve_success(self):
        self.verify()
        (self.build / 'CMakeCache.txt').write_text('ENABLE_SWITCH2KIT:BOOL=OFF\n')
        with self.assertRaisesRegex(RuntimeError, 'ENABLE_SWITCH2KIT=ON'):
            self.verify()
        self.assertEqual(json.loads((self.root / 'result/qualification.json').read_text())['status'], 'started')

    def test_empty_runtime_notice_rejected(self):
        (self.package / 'share/Switch2KitNotices/SwiftRuntime/ICU.txt').write_text('')
        with self.assertRaisesRegex(RuntimeError, 'Invalid package notice'):
            self.verify()

    def test_missing_facade_rejected(self):
        (self.package / 'lib/libSwitch2KitC.so').unlink()
        with self.assertRaisesRegex(RuntimeError, 'exactly one installed facade'):
            self.verify()

    def test_existing_hidden_tree_is_not_overwritten(self):
        hidden = self.root / 'bin.qualification-unavailable'
        hidden.mkdir()
        with self.assertRaisesRegex(RuntimeError, 'Refusing to overwrite'):
            self.verify()
        self.assertTrue(self.build.exists())
        self.assertTrue(hidden.exists())


class StaticPlatformInspectionTests(unittest.TestCase):
    # Mocked command outputs validate scanner policy, not Apple/Windows loaders.
    def test_mac_external_rpath_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            package = Path(folder)
            exe = package / 'Contents/MacOS/Cemu_release'
            output = 'cmd LC_RPATH\n cmdsize 48\n path /opt/swift/lib (offset 12)\n'
            with patch.object(qualification, 'run', return_value=output):
                with self.assertRaisesRegex(RuntimeError, 'Non-relocatable macOS runtime search path'):
                    qualification.inspect_macos(exe, package)

    def test_mac_absolute_external_dependency_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            package = Path(folder)
            exe = package / 'Contents/MacOS/Cemu_release'
            def fake_run(command):
                if '-l' in command:
                    return 'cmd LC_RPATH\n cmdsize 48\n path @executable_path/../Frameworks (offset 12)\n'
                return 'Cemu:\n /usr/local/lib/libMoltenVK.dylib (compatibility version 1.0.0)\n'
            with patch.object(qualification, 'run', side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, 'Non-relocatable macOS dependency'):
                    qualification.inspect_macos(exe, package)

    def test_mac_alternate_bundled_image_inspected(self):
        with tempfile.TemporaryDirectory() as folder:
            package = Path(folder)
            alternate = package / 'alternate.dylib'
            alternate.write_bytes(bytes.fromhex('cffaedfe'))
            exe = package / 'Contents/MacOS/Cemu_release'
            def fake_run(command):
                if '-l' in command:
                    return ''
                if command[-1] == alternate:
                    return 'alternate:\n /opt/swift/libMissing.dylib (compatibility version 1.0.0)\n'
                return 'Cemu:\n /usr/lib/libSystem.B.dylib (compatibility version 1.0.0)\n'
            with patch.object(qualification, 'run', side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, 'Non-relocatable macOS dependency'):
                    qualification.inspect_macos(exe, package)

    def test_windows_swift_in_system_directory_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            package = Path(folder) / 'package'
            package.mkdir()
            system = Path(folder) / 'Windows/System32'
            system.mkdir(parents=True)
            (system / 'swiftCore.dll').touch()
            with patch.dict(qualification.os.environ, {'SystemRoot': str(system.parent)}):
                with patch.object(qualification, 'run', return_value='    swiftCore.dll\n'):
                    with self.assertRaisesRegex(RuntimeError, 'Windows runtime missing from package'):
                        qualification.inspect_windows(package / 'Cemu_release.exe', package, 'dumpbin')


if __name__ == '__main__':
    unittest.main()
