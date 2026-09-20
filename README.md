# Cemu - Wii U emulator

**This fork embeds [Switch2Kit](https://github.com/jmonster/Switch2Kit) for NSO GameCube and Nintendo Switch 2 Pro controllers on macOS 15+, with experimental Linux x86-64 and Windows x64 builds.**

Use this controller-enabled Cemu directly: connect the controller, choose a player slot and emulated controller, and play. No separate dashboard, network bridge, SDL override or virtual-controller driver is needed. Joy-Con 2 halves remain available as separate, complementary input sources.

## Quick start

**Controller-enabled application artifacts are no longer built for every pull request.** Use [Build from source](#build-from-source-alternative) for the checked-out revision. Older development artifacts may remain in the [Actions history](https://github.com/jmonster/Cemu/actions) until they expire; inspect the recorded revision and its original build/launch results before using one. Ordinary upstream downloads do not include this integration.

### macOS

Use macOS 15 or newer on Apple Silicon or Intel and follow the [platform build guide](docs/Switch2Kit.md#build-from-source). The helper creates `bin/Cemu_release.app`. Open that app, enable Bluetooth and allow Cemu's Bluetooth request; denied access is managed under **System Settings > Privacy & Security > Bluetooth**.

These apps are ad-hoc signed, not notarized. For a source you trust, use Apple's [per-app Open Anyway procedure](https://support.apple.com/en-us/102445); do not disable Gatekeeper globally.

### Linux

Use Ubuntu 24.04 x86-64 with a desktop session, graphics drivers, normal system-bus permissions, a powered Bluetooth LE adapter and the BlueZ service. See the [Linux prerequisites](docs/Switch2Kit.md#linux) and [source build instructions](docs/Switch2Kit.md#build-from-source). The helper installs to `build-switch2kit/install`; launch `build-switch2kit/install/bin/Cemu_release`.

Keep `bin`, `lib` and `share` together. The Swift runtime is packaged; no Swift installation or loader-path override is needed for a correctly staged application. Compatible system libraries and drivers remain required. This is not an AppImage or universal Linux binary.

### Windows

Use Windows 11 x64 for these experimental instructions, with a Bluetooth LE driver, graphics drivers and the Microsoft Visual C++ x64 runtime. Follow the [Windows build instructions](docs/Switch2Kit.md#build-from-source), then open `bin/Cemu_release.exe`. Physical Windows 11 controller support still requires hardware validation.

Keep its DLLs, `resources`, `gameProfiles` and `Switch2KitNotices` beside it. The selected Swift runtime is included; launching does not require the Swift compiler or its PATH. Run normally, not as administrator, and do not disable operating-system security to bypass errors. See the [Windows source fallback](docs/Switch2Kit.md#windows).

### Connect and play

1. Close competing controller apps/consoles. In Cemu, open **Options > Input settings**, click **Find Switch 2 Controllers**, and hold the controller's **Sync** button until its player lights sweep. Allow legitimate Bluetooth access prompts. The search lasts 60 seconds; repeat Find to retry.
2. Select the desired controller tab (for example, **Controller 1**). In the physical-controller dropdown beside **Emulated controller**, select the connected **GameCube** or **Pro Controller 2**. Cemu applies recommended mappings. An empty first slot becomes a Wii U GamePad, other empty slots become Wii U Pro Controllers; existing GamePad/Pro/Classic types are retained. Choose a type the game accepts.
3. Verify buttons, sticks and triggers in Input settings. Open the physical controller's **Settings** to adjust **Rumble** and use **Test rumble**, then open the game. NSO GameCube trigger travel and full clicks remain independent inputs; Pro triggers are digital. GameCube sticks have no click buttons, so games needing those actions require additional bindings.

Saved assignments follow the physical controller rather than its discovery order. Reopen the same application and use **Find** again on subsequent launches; Cemu does not implement Dolphin's automatic-reconnect option. **Disconnect Switch 2 Controllers** stops the backend without erasing saved profiles. Replacing a populated slot asks first and creates a **Before Switch2Kit-…** backup; reconnecting does not overwrite custom mappings.

For Joy-Con 2, discover each half with Find/Sync, choose the emulated controller type, and add both via **+ > SDLController** to the same player slot. These are separate complementary sources, not a system-wide paired virtual controller. Motion requires a measured, device-matching `.s2kmotion` profile selected in physical-controller Settings and **Use motion** enabled. Never use synthetic test profiles for gameplay. See [controller differences and motion](docs/Switch2Kit.md#controller-differences-and-motion).

### Build from source (alternative)

Follow the [platform build guide](docs/Switch2Kit.md#build-from-source). Check out this maintained fork and its pinned dependencies:

```sh
git clone --branch main --recurse-submodules https://github.com/jmonster/Cemu.git cemu-switch2kit
cd cemu-switch2kit
```

The helpers enable Switch2Kit and use the pinned submodule; do not apply the SDK's separate upstream patches. The ordinary upstream instructions below do not enable it by default. Linux/Windows support remains experimental, and the CI smoke test does not establish full native integration, physical-controller or gameplay acceptance.

## Upstream Cemu information

The information below describes Cemu generally, including builds without this fork's Switch2Kit feature. Controller-enabled builds use the platform requirements above.

[![Upstream Build Process](https://github.com/cemu-project/Cemu/actions/workflows/build.yml/badge.svg)](https://github.com/cemu-project/Cemu/actions/workflows/build.yml)
[![Discord](https://img.shields.io/discord/286429969104764928?label=Cemu&logo=discord&logoColor=FFFFFF)](https://discord.gg/5psYsup)
[![Matrix Server](https://img.shields.io/matrix/cemu:cemu.info?server_fqdn=matrix.cemu.info&label=cemu:cemu.info&logo=matrix&logoColor=FFFFFF)](https://matrix.to/#/#cemu:cemu.info)

This is the code repository of Cemu, a Wii U emulator that is able to run most Wii U games and homebrew in a playable state.
It's written in C/C++ and is being actively developed with new features and fixes.

Cemu is currently only available for 64-bit Windows, Linux & macOS devices.

### Links:
 - [Open Source Announcement](https://www.reddit.com/r/cemu/comments/wwa22c/cemu_20_announcement_linux_builds_opensource_and/)
 - [Official Website](https://cemu.info)
 - [Compatibility List/Wiki](https://wiki.cemu.info/wiki/Main_Page)
 - [Official Subreddit](https://reddit.com/r/Cemu)
 - [Official Discord](https://discord.gg/5psYsup)
 - [Official Matrix Server](https://matrix.to/#/#cemu:cemu.info)
 - [Setup Guide](https://cemu.cfw.guide)

#### Other relevant repositories:
 - [Cemu-Language](https://github.com/cemu-project/Cemu-Language)
 - [Cemu's Community Graphic Packs](https://github.com/cemu-project/cemu_graphic_packs)

## Upstream downloads (without this integration)

For this fork's Switch2Kit support, use [Quick start](#quick-start) above instead.

You can download the latest upstream Cemu releases for Windows, Linux and Mac from the [upstream GitHub Releases](https://github.com/cemu-project/Cemu/releases/). For Linux you can also find upstream Cemu on [Flathub](https://flathub.org/apps/info.cemu.Cemu).

On Windows, Cemu is available both as an installer and in a portable format, where no installation is required besides extracting it in a safe place.

The native macOS build is currently purely experimental and should not be considered stable or ready for issue-free gameplay. There are also known issues with degraded performance due to the use of MoltenVK and Rosetta for ARM Macs. We appreciate your patience while we improve Cemu for macOS.

Pre-2.0 releases can be found on Cemu's [changelog page](https://cemu.info/changelog.html).

## Build Instructions

For a controller-enabled desktop build, use [Build from source](#build-from-source-alternative) above. For ordinary upstream-style builds on Windows, Linux or macOS, view [BUILD.md](/BUILD.md).

## Issues

Issues with the emulator should be filed using [GitHub Issues](https://github.com/cemu-project/Cemu/issues).  
The old bug tracker can be found at [bugs.cemu.info](https://bugs.cemu.info) and still contains relevant issues and feature suggestions.

## Contributing

If you want to contribute you can take a look at our [contribution guidelines](/CONTRIBUTING.md).

## License
Cemu is licensed under [Mozilla Public License 2.0](/LICENSE.txt). Exempt from this are all files in the dependencies directory for which the licenses of the original code apply as well as some individual files in the src folder, as specified in those file headers respectively.
