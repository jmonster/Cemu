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
before applying the radio policy. Startup happens once on the main run loop after
SDL initialization, even when Input settings is never opened.

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
asynchronous stop may briefly report busy; wait a moment and retry explicitly.

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
`51e36a8223f2c4254b8d9d9f43dc10c8cdb3ff33`, which includes the additive automatic
C API and the shared `SDLHost` forwarding methods from
[Switch2Kit PR #74](https://github.com/jmonster/Switch2Kit/pull/74). Do not link an
older library that lacks `s2k_set_automatic_discovery`. The source-integration
checks verify the matching pin. This follows the opt-in lifecycle behavior in
[Dolphin PR #3](https://github.com/jmonster/dolphin/pull/3).

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

`python3 tests/switch2kit/run.py --sanitize` runs mapping, identity, session,
autoconnect, configuration and backup/rollback policies after the native build.
Use `--sdl /path/to/SDL-source` and `--sdk /path/to/Switch2Kit` for a separate
checkout of the same pinned SDK revision. The standalone automatic-connection
suite can also run with a C++20 compiler and Boost headers:

```sh
python3 tests/switch2kit/test_autoconnect.py --sanitize
CXX=g++ python3 tests/switch2kit/test_autoconnect.py --sanitize
```

These tests execute the production session/configuration policies with controlled
SDK-host and atomic-writer boundaries and real INI file reads. They cover default
off, one-shot consent, repeated polling without renewal, stop/shutdown fences,
live-session preservation, save/read failure, retry and unrelated INI entries.
The size regressions verify exact-limit round trips and preservation of both the
file and runtime choice when an update would overflow the input bound.
Source checks guard the startup hook, checkbox, SDK pin and build gates; they are
not native GUI execution. Native CI compiles the full Cemu app on Apple silicon
and Intel, checks its bundle/signature, and launches, normally quits and
relaunches the exact ZIP with build dependencies denied. The SDK job also tests
real SDL, motor packets and the Cemu motion consumer.

### Physical acceptance checklist (not established by policy tests)

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

Physical pairing, input, reconnect, multiplayer, rumble start/stop, measured
motion and gameplay still require a real Mac and controller. There is no claim
that CI tests exercise physical hardware or downloaded-app Gatekeeper approval.
