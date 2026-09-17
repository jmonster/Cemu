# Switch 2 controllers on macOS

This build connects Switch 2 controllers directly inside Cemu. No standalone
Switch2Kit app, SDL override, virtual-HID driver or Accessibility permission is
needed. Bluetooth input, motor rumble and calibrated motion use the pinned
Switch2Kit library included as a submodule.

## Build and open

Use macOS 15 or newer and Xcode 26 or newer with Swift 6.2. Open Xcode once to
finish setup, and select it under Settings > Locations > Command Line Tools.
Install the build tools once:

```sh
brew install cmake ninja nasm automake libtool molten-vk
```

In your Cemu checkout:

```sh
bash scripts/build-switch2kit.sh --run
```

The script builds `bin/Cemu_release.app` and opens it. Open that app again normally
for later sessions. To update, quit Cemu, run `git pull --ff-only`, and run the
script again. The script updates submodules to the versions selected by Cemu.
It does not replace existing settings or install a system driver.

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

## Controller differences

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
`dependencies/Switch2Kit/docs/switch2kit/motion-profiles.md`. Without valid calibration the
backend does not invent sensor values or integrate across a disconnect or gap.

## Build scope and checks

`ENABLE_SWITCH2KIT` remains off by default for ordinary cross-platform builds.
Enabled builds require SDL, a macOS application bundle and deployment target 15.0
or newer. The normal disabled build keeps the upstream deployment requirements
and does not compile or link Switch2Kit. The build helper enables these options,
embeds the SDK and its distribution notices, then ad-hoc signs the completed app.
These are development builds, not notarized releases.

`python3 tests/switch2kit/run.py --sanitize` runs mapping, identity, session and
backup/rollback policies after the native build. Use `--sdl /path/to/SDL-source`
and `--sdk /path/to/Switch2Kit` for a separate source checkout. Policy tests use
real enum definitions with controlled host/storage boundaries; they do not claim
Bluetooth or GUI interaction. Native CI compiles the full Cemu app on Apple
silicon and Intel, checks its bundle/signature, and launches, normally quits and
relaunches the exact ZIP with build dependencies denied. The SDK job also tests
real SDL, motor packets and the Cemu motion consumer.

Physical pairing, input, reconnect, multiplayer, rumble start/stop, measured
motion and gameplay still require a real Mac and controller. There is no claim
that CI tests exercise physical hardware or downloaded-app Gatekeeper approval.
