# Native Switch 2 controllers in Cemu

**This maintained Cemu fork embeds [Switch2Kit](https://github.com/jmonster/Switch2Kit) for NSO GameCube and Nintendo Switch 2 Pro controllers on macOS 15+, with experimental Linux x86-64 and Windows x64 support.**

Use the [controller-enabled builds](../README.md#quick-start), launch that Cemu and [select your controller/player slot](#connect-and-play). Ordinary upstream downloads do not embed this backend. No separate dashboard, SDL override, network bridge, virtual-controller driver or Accessibility permission is required. Joy-Con 2 halves remain separate complementary sources; motion requires explicit measured calibration.

## Applications and prerequisites

Controller-enabled application builds are now a deliberate local operation using the [source helpers](#build-from-source), not extra CI builds on every pull request. The fork retains upstream workflows and one small session smoke test. That test does not produce downloads. Older development artifacts may remain in the [Actions history](https://github.com/jmonster/Cemu/actions) until their original retention expires; their recorded native build/launch results apply only to that exact revision. Unchanged upstream build artifacts do not enable Switch2Kit.

### macOS

Use macOS 15 or newer on Apple Silicon (`arm64`) or Intel (`x86_64`). [Build from source](#build-from-source) with the native architecture, then open `bin/Cemu_release.app`. The helper embeds the controller library and runtime. Enable Bluetooth and permit Cemu under **System Settings > Privacy & Security > Bluetooth** when requested.

The application is ad-hoc signed, not notarized. Use Apple's [per-app approval procedure](https://support.apple.com/en-us/102445) only for an application you trust; do not disable Gatekeeper globally. macOS emulator performance/support limitations are separate from controller input support.

### Linux

The application target is Ubuntu 24.04 x86-64 with a graphical desktop and working OpenGL/Vulkan/audio drivers. This is an installed prefix, not a universal Linux package. Install the distribution's normal runtime libraries; on Ubuntu 24.04:

```sh
sudo apt-get update
sudo apt-get install bluez libsystemd0 libgtk-3-0t64 libpulse0 libsecret-1-0   libgcrypt20 libudev1 libgl1 libegl1 libvulkan1 libx11-xcb1
```

[Build from source](#build-from-source), then launch `build-switch2kit/install/bin/Cemu_release`. The helper's `--run` option opens it after building.

Keep the whole prefix, including `lib`, `share/Cemu` and `share/Switch2KitNotices`. The selected Swift runtime is packaged; no Swift installation or `LD_LIBRARY_PATH` override is needed to launch it. System desktop libraries and drivers remain prerequisites.

Turn Bluetooth on in your desktop settings. BlueZ must be running, the adapter powered and LE-capable, and your normal account allowed system-bus/GATT access. `bluetoothctl show` can inspect the adapter. The integration does not power it on, change D-Bus permissions, erase bonds or call BlueZ Pair/RemoveDevice. Use Cemu's Find/Sync procedure; the controller handshake is distinct from operating-system pairing. Resolve denied access through normal distribution Bluetooth policy instead of running Cemu as root.

### Windows

The experimental desktop target is Windows 11 x64 with a working Bluetooth LE adapter/driver and graphics drivers. Previous native qualification used Windows Server runners, not physical Windows 11 controllers. ARM64 and 32-bit Windows are not covered. Install the [Microsoft Visual C++ x64 runtime](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) if required.

[Build from source](#build-from-source) with the PowerShell helper, then open `bin/Cemu_release.exe`. Keep DLLs, `resources`, `gameProfiles` and `Switch2KitNotices` together. A correctly staged package includes the Swift runtime; installing Swift or adding compiler directories to PATH is not a launch requirement, though Swift is required to build it.

Enable **Settings > Bluetooth & devices > Bluetooth** and use Cemu's Find/Sync procedure. Honor legitimate system pairing/access prompts; the backend does not erase bonds or bypass pairing policy. Run as a normal user, not administrator, and do not disable SmartScreen/antivirus. A missing DLL before the GUI opens is a packaging/runtime prerequisite failure: use the complete output of the build helper and inspect the staged dependencies rather than copying only the executable.

## Connect and play

Close any other app managing the same controllers. Open **Options > Input
settings**, click **Find Switch 2 Controllers**, allow Bluetooth, and hold the
controller's **Sync** button. Scanning lasts 60 seconds.

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
reconnect in a different order. Use **Find** again after restarting Cemu. Use
**Disconnect Switch 2 Controllers** to stop this backend without erasing profiles.

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

`ENABLE_SWITCH2KIT` is OFF by default. Disabled builds do not require Swift and retain upstream platform/deployment requirements. Enabled Linux/Windows builds use native SDL3 and the desktop backend; only enabled macOS builds require a macOS 15+ app bundle. Initialize the SDK revision selected by this maintained fork, not a moving SDK branch or the SDK's separate upstream patches. Check out main and its recorded dependencies:

```sh
git clone --branch main --recurse-submodules https://github.com/jmonster/Cemu.git cemu-switch2kit
cd cemu-switch2kit
```

**macOS:** install Xcode 26+ with Swift 6.2+, finish its first-launch setup and select its Command Line Tools. Then:

```sh
brew install cmake ninja nasm automake libtool molten-vk
bash scripts/build-switch2kit.sh --run
```

The helper builds and opens `bin/Cemu_release.app` with the native architecture. The finished app has its controller/runtime dependencies and notices embedded before ad-hoc signing.

**Linux:** install Swift **6.2.1** from [the official Linux instructions](https://www.swift.org/install/linux/), then the native build dependencies:

```sh
sudo apt-get install build-essential cmake ninja-build python3 pkg-config curl zip unzip tar zstd   nasm autoconf automake libtool libtool-bin gettext freeglut3-dev libgcrypt20-dev libglm-dev   libgtk-3-dev libpulse-dev libsecret-1-dev libsystemd-dev libudev-dev libbluetooth-dev   libgl1-mesa-dev libglu1-mesa-dev libx11-xcb-dev libwayland-dev wayland-protocols   extra-cmake-modules dbus
bash scripts/build-switch2kit.sh --run
```

The helper builds through the recorded vcpkg dependencies, installs to `build-switch2kit/install` and opens `build-switch2kit/install/bin/Cemu_release`. Keep the entire install prefix.

**Windows:** use native x64 Swift **6.2.1**, Visual Studio **2022 Desktop development with C++** and its Windows SDK, CMake, Ninja, Git, Python 3 and 64-bit PowerShell 7. Follow [Swift's Windows installation guide](https://www.swift.org/install/windows/) for its toolchain. This combination was used by the retired native Windows workflow; do not combine Swift 6.2's bundled compiler with VS 2026 STL headers. The helper currently selects the latest installed Visual C++ instance, so use a build machine where that instance is the compatible VS 2022 toolchain.

```powershell
./scripts/build-switch2kit.ps1 -Run
```

It bootstraps the pinned vcpkg checkout, builds `CemuBin` with Switch2Kit enabled, stages runtime dependencies and opens `bin/Cemu_release.exe`. Build-time PATH changes stay process-local. A successful local build is not qualification of a separately distributed package.

For updates, quit Cemu, run `git pull --ff-only`, update the recorded submodules and rerun the same helper. Do not delete your settings, profiles or game data to make a new build launch.

## Qualification and troubleshooting

A missing **Find Switch 2 Controllers** button means a backend-disabled binary was launched. For absent input, verify Bluetooth power/access, Sync mode, competing connections, physical selection and the emulated type accepted by the game, then retry Find. A controller already assigned to another slot must be removed there before the quick setup shortcut can move it. Cemu requires Find again after restarting; it does not implement Dolphin's automatic-reconnection option. A changed adapter or rotating device address can change physical identity, so verify player assignments after such a change.

## CI policy

Upstream's five workflow files and its tests are retained unchanged. The only added check is **Switch2Kit session smoke**: one C++ executable, compiled and run once on Ubuntu for each pull-request update, with a two-minute job limit and a ten-second execution limit. It is also available through manual dispatch. It has no platform matrix, dependency checkout, SDK test rerun, full application build, cache or artifact upload. No extra push or scheduled run is added.

The test includes the production `Switch2KitSession.h` and controls only the external host boundary. Its single lifecycle scenario checks default-off behavior, failed discovery and explicit retry, preserving an active session after a failed rescan, and stopping polling on disconnect/shutdown. Run the same test locally from the repository root with any C++20 compiler; no submodules are needed:

```sh
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
c++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/switch2kit/smoke.cpp -o "$work/smoke"
"$work/smoke"
```

The additional mapping, identity, file-reader, source-wiring and platform-configuration harnesses, SDK reruns, and duplicate application/GUI qualification workflows have been removed, not hidden behind one aggregate test command. SDK regression testing belongs in the SDK repository. This narrow smoke check does not establish full Cemu/SDL/SDK integration, controller mappings, runtime packaging, GUI startup, platform compatibility or hardware acceptance. Before distributing a controller-enabled build, separately validate that exact build on its target platform using the normal build helpers and real hardware; routine upstream builds keep the backend disabled by default.

Record separate hardware acceptance for each model/firmware/OS/adapter and tested commit: first pairing and denied-access retry; all controls and releases; independent GameCube trigger travel/clicks; rumble start/stop; two identical controllers reconnecting in reversed order; persisted player assignments after restart; Bluetooth/adapter loss; explicit disconnect; normal shutdown; measured motion where used; and an actual gameplay session. Joy-Con 2 acceptance must include both complementary sources in one slot. No fixture profile or automated pass substitutes for those physical results.
