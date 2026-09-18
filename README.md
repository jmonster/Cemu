# **Cemu - Wii U emulator**

**This fork supports the Nintendo Switch Online GameCube controller and Nintendo Switch 2 Pro Controller on macOS through [Switch2Kit](https://github.com/jmonster/Switch2Kit).**

[![Native Switch2Kit builds](https://github.com/jmonster/Cemu/actions/workflows/native-switch2kit.yml/badge.svg)](https://github.com/jmonster/Cemu/actions/workflows/native-switch2kit.yml)

Controller support is built into Cemu. There is no separate Switch2Kit app or controller driver to install, and GameCube/Pro controller setup includes recommended mappings and rumble.

## Quick start (macOS 15+)

### Get a controller-enabled build

1. Sign in to GitHub, open this fork's [Native Switch2Kit builds](https://github.com/jmonster/Cemu/actions/workflows/native-switch2kit.yml), and select a successful run with a green check. Use **Native Switch2Kit**, not the ordinary **Build check** workflow.
2. Under **Artifacts**, download **Cemu-Switch2Kit-arm64** for an Apple Silicon Mac or **Cemu-Switch2Kit-x86_64** for an Intel Mac. Choose the application artifact, not a diagnostics artifact.
3. Extract the downloaded ZIP, then extract **integration-app.zip** inside it. Move **Cemu_release.app** to Applications and open it. Reopen this same app for later sessions.

Downloads currently come from GitHub Actions, not a published release. Application artifacts expire after 14 days; when no application download is available, use [Build from source](#build-from-source-alternative) below. Ordinary upstream Cemu downloads do not include this Switch2Kit integration.

These are development builds, not notarized releases. For an unverified-developer warning, use Apple's [per-app Open Anyway instructions](https://support.apple.com/en-us/102445) only when you trust the download's source. Do not disable Gatekeeper globally.

### Connect and play

1. Turn on your Mac's Bluetooth and close other apps managing the controller, including the Switch2Kit dashboard or Dolphin. In Cemu, open **Options > Input settings**, click **Find Switch 2 Controllers**, allow Bluetooth access, and hold the controller's **Sync** button until its player lights sweep. The search lasts 60 seconds; click Find again to retry.
2. On the desired controller tab, select your **GameCube** or **Pro Controller 2** in the dropdown beside **Emulated controller**. Cemu applies the button and stick mappings automatically. An empty first slot becomes a **Wii U GamePad**; other empty slots become **Wii U Pro Controllers**. Use the emulated controller type your game supports.
3. Check that the input display responds to button presses, releases, and stick movement. Open the physical controller's **Settings** to adjust **Rumble** and click **Test rumble**. Close Input settings and open your Wii U game.

Cemu saves assignments and mappings. On later launches, use **Find Switch 2 Controllers** again to connect; **Disconnect Switch 2 Controllers** stops the current session without erasing profiles. Replacing a populated slot asks for confirmation and saves a backup; reconnecting does not reset custom mappings.

The NSO GameCube controller has no stick-click buttons, so bind those actions to spare buttons or a keyboard when a game needs them. A controller does not replace the Wii U touchscreen. Motion requires a measured, device-matching `.s2kmotion` profile and **Use motion** in the physical controller's Settings; it is not automatically calibrated and is not required for ordinary button/stick input. See the [full controller guide](docs/Switch2Kit.md) for these details and for adding both Joy-Con 2 halves through **+ > SDLController**.

**No Find button?** Open the controller-enabled app above, not an upstream or backend-disabled build. **No controller?** Check Bluetooth access for Cemu in **System Settings > Privacy & Security > Bluetooth**, close competing controller apps, and retry Find while holding Sync.

### Build from source (alternative)

<details>
<summary>Build and launch the controller-enabled app on your Mac</summary>

Use macOS 15+, [Xcode](https://developer.apple.com/xcode/) 26+ with Swift 6.2+, and [Homebrew](https://brew.sh/). Open Xcode once to finish setup and select it under **Xcode > Settings > Locations > Command Line Tools**. On Apple Silicon, use a native Terminal and native Homebrew, not Rosetta.

Run these commands in Terminal:

```sh
brew install cmake ninja nasm automake libtool molten-vk
git clone --recurse-submodules https://github.com/jmonster/Cemu.git cemu-switch2kit
cd cemu-switch2kit
bash scripts/build-switch2kit.sh --run
```

The helper builds this fork and its pinned dependencies, enables Switch2Kit, and opens **bin/Cemu_release.app**. No separate SDK checkout or patching is needed. Once it opens, follow [Connect and play](#connect-and-play).

For later launches, reopen that app. To update the source build, quit Cemu, run `git pull --ff-only` from this checkout, and rerun `bash scripts/build-switch2kit.sh --run`. The helper updates the pinned submodules without replacing your settings. The ordinary upstream build instructions below do not enable Switch2Kit by default.

</details>

See the [full Switch2Kit controller guide](docs/Switch2Kit.md) for custom mappings, profile backups, multiplayer, motion calibration, and testing limits. Automated build/launch checks do not establish physical-controller or gameplay acceptance. Switch2Kit support in this fork is macOS-only; the SDK's experimental Linux work is separate.

## Upstream Cemu information

The information below describes Cemu generally, including builds without this fork's Switch2Kit feature. Controller-enabled builds have the macOS 15+ requirements above.

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

For this fork's Switch2Kit support, use [Get a controller-enabled build](#get-a-controller-enabled-build) above instead.

You can download the latest upstream Cemu releases for Windows, Linux and Mac from the [upstream GitHub Releases](https://github.com/cemu-project/Cemu/releases/). For Linux you can also find upstream Cemu on [Flathub](https://flathub.org/apps/info.cemu.Cemu).

On Windows, Cemu is available both as an installer and in a portable format, where no installation is required besides extracting it in a safe place.

The native macOS build is currently purely experimental and should not be considered stable or ready for issue-free gameplay. There are also known issues with degraded performance due to the use of MoltenVK and Rosetta for ARM Macs. We appreciate your patience while we improve Cemu for macOS.

Pre-2.0 releases can be found on Cemu's [changelog page](https://cemu.info/changelog.html).

## Build Instructions

For a controller-enabled macOS build, use [Build from source](#build-from-source-alternative) above. For ordinary upstream-style builds on Windows, Linux or macOS, view [BUILD.md](/BUILD.md).

## Issues

Issues with the emulator should be filed using [GitHub Issues](https://github.com/cemu-project/Cemu/issues).  
The old bug tracker can be found at [bugs.cemu.info](https://bugs.cemu.info) and still contains relevant issues and feature suggestions.

## Contributing

If you want to contribute you can take a look at our [contribution guidelines](/CONTRIBUTING.md).

## License
Cemu is licensed under [Mozilla Public License 2.0](/LICENSE.txt). Exempt from this are all files in the dependencies directory for which the licenses of the original code apply as well as some individual files in the src folder, as specified in those file headers respectively.
