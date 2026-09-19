# Native Switch 2 controllers in Cemu

**This maintained Cemu fork embeds [Switch2Kit](https://github.com/jmonster/Switch2Kit) for NSO GameCube and Nintendo Switch 2 Pro controllers on macOS 15+, with experimental Linux x86-64 and Windows x64 support.**

Use the [controller-enabled downloads](../README.md#quick-start), launch that Cemu and [select your controller/player slot](#connect-and-play). Ordinary upstream downloads do not embed this backend. No separate dashboard, SDL override, network bridge, virtual-controller driver or Accessibility permission is required. Joy-Con 2 halves remain separate complementary sources; motion requires explicit measured calibration.

## Applications and prerequisites

Sign in to GitHub and select a successful run for `feature/switch2kit-auto-connect` while this PR is unmerged. Download the application artifact, not a source or diagnostics archive. The outer GitHub artifact ZIP contains the application ZIP/tarball. Desktop application artifacts expire after 14 days; use the source fallback when no matching successful artifact remains. These are development builds, not published production releases. A workflow configuration or a different revision's result is not qualification of the selected download.

### macOS

Use macOS 15 or newer on Apple Silicon (`arm64`) or Intel (`x86_64`). From [Native Switch2Kit](https://github.com/jmonster/Cemu/actions/workflows/native-switch2kit.yml), download **Cemu-Switch2Kit-arm64** or **Cemu-Switch2Kit-x86_64**, extract the outer and inner ZIPs, move `Cemu_release.app` to Applications and open it. The controller library and runtime are embedded. Enable Bluetooth and permit Cemu under **System Settings > Privacy & Security > Bluetooth** when requested.

The application is ad-hoc signed, not notarized. Use Apple's [per-app approval procedure](https://support.apple.com/en-us/102445) only for an application you trust; do not disable Gatekeeper globally. macOS emulator performance/support limitations are separate from controller input support.

### Linux

The application target is Ubuntu 24.04 x86-64 with a graphical desktop and working OpenGL/Vulkan/audio drivers. This is an installed prefix, not a universal Linux package. Install the distribution's normal runtime libraries; on Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install bluez libsystemd0 libgtk-3-0t64 libpulse0 libsecret-1-0   libgcrypt20 libudev1 libgl1 libegl1 libvulkan1 libx11-xcb1
```

Get **Cemu-Switch2Kit-linux-x86_64** from [the Linux application workflow](https://github.com/jmonster/Cemu/actions/workflows/switch2kit-linux.yml). Extract the outer ZIP, then:

```sh
tar -xzf Cemu-Switch2Kit-linux-x86_64.tar.gz
./Cemu-Switch2Kit-linux-x86_64/bin/Cemu_release
```

Keep the whole prefix, including `lib`, `share/Cemu` and `share/Switch2KitNotices`. The selected Swift runtime is packaged; no Swift installation or `LD_LIBRARY_PATH` override is needed to launch it. System desktop libraries and drivers remain prerequisites.

Turn Bluetooth on in your desktop settings. BlueZ must be running, the adapter powered and LE-capable, and your normal account allowed system-bus/GATT access. `bluetoothctl show` can inspect the adapter. The integration does not power it on, change D-Bus permissions, erase bonds or call BlueZ Pair/RemoveDevice. Use Cemu's Find/Sync procedure; the controller handshake is distinct from operating-system pairing. Resolve denied access through normal distribution Bluetooth policy instead of running Cemu as root.

### Windows

The experimental desktop target is Windows 11 x64 with a working Bluetooth LE adapter/driver and graphics drivers. Native CI uses Windows Server runners, not physical Windows 11 controllers. ARM64 and 32-bit Windows are not covered. Install the [Microsoft Visual C++ x64 runtime](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) if required.

Get **Cemu-Switch2Kit-windows-x86_64** from [the Windows application workflow](https://github.com/jmonster/Cemu/actions/workflows/switch2kit-windows.yml). Extract the outer ZIP and its inner `Cemu-Switch2Kit-windows-x86_64.zip`, then open `Cemu-Switch2Kit-windows-x86_64/Cemu_release.exe`. Keep DLLs, `resources`, `gameProfiles` and `Switch2KitNotices` together. The package includes the Swift runtime; installing Swift or adding compiler directories to PATH is not a launch requirement.

Enable **Settings > Bluetooth & devices > Bluetooth** and use Cemu's Find/Sync procedure. Honor legitimate system pairing/access prompts; the backend does not erase bonds or bypass pairing policy. Run as a normal user, not administrator, and do not disable SmartScreen/antivirus. A missing DLL before the GUI opens is a packaging/runtime prerequisite failure: re-extract the complete controller-enabled artifact and check the native launch result for its revision.

## Connect and play

Close any other app managing the same controllers. Open **Options > Input
settings**, click **Find Switch 2 Controllers**, allow Bluetooth, and hold the
controller's **Sync** button. With automatic connection off, scanning lasts
60 seconds. Find opens another bounded window when needed.

Choose the connected **GameCube** or **Pro Controller 2** in the new dropdown
beside **Emulated controller** on the desired controller tab. Cemu applies the
button and stick mappings automatically. An empty first slot becomes a Wii U
GamePad; other empty slots become Wii U Pro Controllers. Existing GamePad, Pro
or Classic controller types are retained. Choose a type that the game supports.

The shortcut enables rumble for a new attachment. Open the physical controller's
**Settings**, adjust **Rumble**, and click **Test rumble** for a half-second test.
GameCube rumble is on/off; Pro rumble uses its existing HD-rumble path.

Replacing a populated slot asks first and saves a **Before Switch2Kit-…** backup
in Profile > Load. Cancelling, a failed backup/save, or changing the slot during
confirmation keeps the old configuration. Refreshing or reconnecting does not
reset custom mappings. A physical controller already assigned to another slot
must be removed there before using the shortcut.

Assignments persist by physical identity, including when identical controllers
reconnect in a different order. Use **Find** again after restarting Cemu when
automatic connection is off. Use **Disconnect Switch 2 Controllers** to stop this
backend without erasing profiles.

## Automatic connection

Enable **Automatically connect Switch 2 controllers** in Input settings to start
support on future launches and keep discovery available after long controller
absences or paused gameplay. It is **off by default**. Cemu saves this consent
before applying the radio policy. Startup happens once on the GUI main run loop
after SDL initialization, even when Input settings is never opened, on every
platform with this native backend enabled.

This uses the SDK's continuous discovery policy, not a timer repeatedly calling
Find or renewing a 60-second window. While Cemu remains open, supported advertising
controllers can be discovered when Bluetooth and connection capacity permit. It
uses radio resources and is **not a remembered-device allowlist**. It does not
pair arbitrary Bluetooth devices, wake a powered-off controller, grant permission,
assign a player slot, or reapply mappings. Initial pairing still requires Sync
and permission; turn the controller on to reconnect thereafter.

Unchecking the option returns discovery to on-demand mode without detaching
ready controllers. An already admitted connection attempt may finish. To cancel
attempts and disconnect all native controllers, use **Disconnect** instead.
Disconnect is authoritative for the current run: input polling, reopening
settings, resuming a game, and a delayed startup callback cannot restart support.
Use **Find**, or turn the option off and on, to resume deliberately. The saved
choice is retained for the next application launch. After Disconnect, the SDK's
asynchronous stop may briefly report busy; retry explicitly after it finishes.

The preference is `[Settings] AutoConnect=true` or `false` in `Switch2Kit.ini`,
beside Cemu's `settings.xml`, not inside controller profiles. Missing files/keys
mean off. Updates re-read the file, preserve unrelated entries and sections, and
use Cemu's atomic writer. INI comments/formatting are not preserved. Malformed,
oversized, inaccessible or non-regular files are not silently replaced. A save
that would exceed the 64 KiB read limit is rejected before writing, so a successful
save remains readable on the next launch. A failed save leaves the previous
choice and radio policy intact; the checkbox reflects the saved choice. Repair
file access or malformed configuration and retry the option. Configuration and
policy/start errors remain visible through successful polling and status
refreshes. Find retries starting support, not saving settings.

The SDK submodule is pinned to
`1e6eac15fd4d4c93f15244eb66d79021c92d29fc`, which combines the shared `SDLHost`
automatic-discovery and policy-only start methods from
[Switch2Kit PR #74](https://github.com/jmonster/Switch2Kit/pull/74) with the merged
[desktop transport and runtime fixes](https://github.com/jmonster/Switch2Kit/pull/75).
Do not substitute the older automatic-connection pin, which lacks those desktop
fixes, or a library missing `s2k_set_automatic_discovery`. Source-integration
checks verify the matching pin. The SDK companion and this combined application's
own current-revision native checks remain merge gates; earlier separate branch
results do not qualify this combination.

## Controller differences and motion

GameCube and Pro mappings preserve printed Nintendo A/B/X/Y labels. On GameCube,
C supplies Minus and Capture supplies the GamePad microphone button. Its physical
sticks do not have click buttons; games requiring L-stick/R-stick clicks need
those actions mapped to spare buttons or an additional keyboard source. Stick
travel and trigger axes remain available in manual mappings.

The Pro controller maps +/−, both stick clicks, shoulders, triggers and D-pad.
Capture supplies the GamePad microphone button and C supplies Show Screen. GL/GR
and other spare inputs remain available for custom bindings.

For Joy-Con 2, pair both halves, choose the emulated controller type, and use
**+ > SDLController** to add each half to the same slot. Each receives its own
complementary mappings; adding one does not overwrite the other's assignments.
This uses Cemu's multiple-source model, not a system-wide virtual paired device.
The quick dropdown is for GameCube and Pro controllers. Wiimote mappings remain
manual. A controller does not replace the Wii U touchscreen; use Cemu's normal
mouse/touchscreen facilities when a game requires them.

## Motion

The in-process sensor path includes GamePad/KPAD motion integration, but **motion
is not automatically calibrated**. In the physical controller's Settings, choose
a measured, device-matching `.s2kmotion` profile and enable **Use motion**, then
click OK. The status explains missing, disabled, disconnected or stale input.
An invalid profile leaves the previous selection intact. Profile import/removal
is an explicit action and the path is saved with the controller configuration.

Do not use test-fixture profiles for gameplay. Calibration must match the real
device and its sensor setup; the SDK documents the measurement procedure in
[the pinned SDK motion-profile guide](../dependencies/Switch2Kit/docs/switch2kit/motion-profiles.md). Without valid calibration the
backend does not invent sensor values or integrate across a disconnect or gap.

## Build from source

`ENABLE_SWITCH2KIT` is OFF by default. Disabled builds do not require Swift and retain upstream platform/deployment requirements. Enabled Linux/Windows builds use native SDL3 and the desktop backend; only enabled macOS builds require a macOS 15+ app bundle. Initialize the SDK revision selected by this maintained fork, not a moving SDK branch or the SDK's separate upstream patches. While the PR is unmerged:

```sh
git clone --branch feature/switch2kit-auto-connect --recurse-submodules https://github.com/jmonster/Cemu.git cemu-switch2kit
cd cemu-switch2kit
```

**macOS:** install Xcode 26+ with Swift 6.2+, finish its first-launch setup and select its Command Line Tools. Then:

```sh
brew install cmake ninja nasm automake libtool molten-vk
bash scripts/build-switch2kit.sh --run
```

The helper builds and opens `bin/Cemu_release.app` with the native architecture. The finished app has its controller/runtime dependencies and notices embedded before ad-hoc signing.

**Linux:** install Swift **6.2.1** from [the official Linux instructions](https://www.swift.org/install/linux/), then the native dependencies used by the Ubuntu workflow:

```sh
sudo apt-get install build-essential cmake ninja-build python3 pkg-config curl zip unzip tar zstd   nasm autoconf automake libtool libtool-bin gettext freeglut3-dev libgcrypt20-dev libglm-dev   libgtk-3-dev libpulse-dev libsecret-1-dev libsystemd-dev libudev-dev libbluetooth-dev   libgl1-mesa-dev libglu1-mesa-dev libx11-xcb-dev libwayland-dev wayland-protocols   extra-cmake-modules dbus
bash scripts/build-switch2kit.sh --run
```

The helper builds through the recorded vcpkg dependencies, installs to `build-switch2kit/install` and opens `build-switch2kit/install/bin/Cemu_release`. Keep the entire install prefix. CI additionally installs Xvfb/Openbox/wmctrl for isolated graphical testing; an ordinary desktop does not need those test tools.

**Windows:** use native x64 Swift **6.2.1**, Visual Studio **2022 Desktop development with C++** and its Windows SDK, CMake, Ninja, Git, Python 3 and 64-bit PowerShell 7. Follow [Swift's Windows installation guide](https://www.swift.org/install/windows/) for its toolchain. This combination matches Cemu's Windows job; do not combine Swift 6.2's bundled compiler with VS 2026 STL headers. The helper currently selects the latest installed Visual C++ instance, so use a build machine where that instance is the compatible VS 2022 toolchain.

```powershell
./scripts/build-switch2kit.ps1 -Run
```

It bootstraps the pinned vcpkg checkout, builds `CemuBin` with Switch2Kit enabled, stages runtime dependencies and opens `bin/Cemu_release.exe`. Build-time PATH changes stay process-local; extracted packages are tested without the compiler PATH.

For updates, quit Cemu, run `git pull --ff-only`, update the recorded submodules and rerun the same helper. Do not delete your settings, profiles or game data to make a new build launch.

## Qualification and troubleshooting

A missing **Find Switch 2 Controllers** button means a backend-disabled binary was launched. For absent input, verify Bluetooth power/access, Sync mode, competing connections, physical selection and the emulated type accepted by the game, then retry Find. A controller already assigned to another slot must be removed there before the quick setup shortcut can move it. Cemu requires Find again after restarting when automatic connection is off; with it enabled, saved consent starts continuous discovery on the next launch. A changed adapter or rotating device address can change physical identity, so verify player assignments after such a change.

Desktop CI builds the complete application, archives it, extracts that exact archive into a new location, checks that the GUI loads its packaged controller/Swift libraries, requests normal quit and relaunches with a private profile. It deliberately seeds noninteractive test settings; pristine first-use dialogs, downloaded-app security approval and physical hardware are not tested. No existing user configuration is erased. The artifact is qualified only after these jobs pass for its exact revision.

`python3 tests/switch2kit/run.py --sanitize` retains executable mapping, identity, lifecycle, rollback, automatic-connection and configuration regressions against controlled host/storage boundaries. Use `--sdl /path/to/SDL-source` and `--sdk /path/to/Switch2Kit` for a separate checkout of the same pinned SDK revision. `python3 tests/switch2kit/test_host_file.py --sdk dependencies/Switch2Kit --sanitize` exercises the real bounded file reader; native MSVC coverage is retained. SDK tests cover protocol values, real C/SDL consumers, calibrated sample handling, rumble bounds, cancellation, runtime relocation and required notices. Source-contract checks supplement those tests, not prose length or English wording restrictions.

The standalone automatic-connection suite needs a C++20 compiler and Boost headers:

```sh
python3 tests/switch2kit/test_autoconnect.py --sanitize
CXX=g++ python3 tests/switch2kit/test_autoconnect.py --sanitize
```

These tests execute the production session/configuration policies with controlled
SDK-host and atomic-writer boundaries and real INI file reads. They cover default
off, one-shot consent, polling without renewal, stop/shutdown fences, live-session
preservation, save/read failure, retry and unrelated INI entries. Size regressions
verify exact-limit round trips and preservation of both the file and runtime choice
when an update would exceed the input bound. Source checks guard the startup hook,
checkbox, SDK pin and native/backend-disabled gates; they are not GUI or physical
Bluetooth execution. The desktop lifecycle and Debug/Release adapter CRT configure
regressions remain in the same runner.

### Automatic-connection hardware acceptance

1. With a fresh configuration, verify automatic connection is off. Enable it,
   pair a controller, close Input settings, and relaunch Cemu without opening
   settings. Verify input, releases, sticks and rumble remain usable.
2. Power off a controller for longer than 60 seconds, then turn it on. Repeat
   several cycles with a game paused and with settings closed. Also test
   Bluetooth off/on and recovery after permission is granted.
3. Reconnect two controllers in reverse order and verify saved player identity,
   custom mappings and motion-profile selection do not change.
4. Disable automatic connection while playing: existing input should remain.
   After disconnecting the controller, use Find for manual discovery.
5. Use Disconnect with automatic connection enabled, then wake controllers,
   reopen settings and resume gameplay. Support must remain stopped until Find,
   explicit re-enabling, or a later launch with saved consent.
6. Check unreadable/malformed settings and failed saves on a real installation:
   the checkbox and error status must accurately report the retained choice.

Record separate hardware acceptance for each model/firmware/OS/adapter and tested commit: first pairing and denied-access retry; all controls and releases; independent GameCube trigger travel/clicks; rumble start/stop; two identical controllers reconnecting in reversed order; persisted player assignments after restart; Bluetooth/adapter loss; explicit disconnect; normal shutdown; measured motion where used; and an actual gameplay session. Joy-Con 2 acceptance must include both complementary sources in one slot. No fixture profile or automated pass substitutes for those physical results.
