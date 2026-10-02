# Native Switch2Kit qualification

`qualify-switch2kit.yml` is an intentionally manual, bounded build and package-loader check. It does not replace upstream CI or the two-minute session smoke. It does not modify SDK pins or publish an application/release.

## Activation and use

1. Review this change together with the compatible Windows helper in PR #6. The workflow must be present on the repository's default branch before GitHub exposes `workflow_dispatch`. Merging is a separate maintainer decision.
2. Enable Actions for this fork if necessary. An authorized maintainer needs permission to dispatch workflows and the repository must allow the SHA-pinned actions. No secrets, new grants or package-registry writes are needed by the job.
3. In Actions, select **Qualify native Switch2Kit package**, choose the revision and exactly one target: `linux-x64`, `windows-x64`, `macos-arm64`, or `macos-x64`. Check the selected revision contains both the workflow and its helpers. Repeat explicitly for other targets; there is no automatic all-platform matrix.
4. Retain each run's result before the three-day artifact expiry. A green smoke check or a successful SDK build is not this qualification result.

Each dispatch has one native build job, capped at 90 minutes and three compiler/vcpkg workers. A newer dispatch of the same revision ref and platform cancels the older one. Cold vcpkg builds may reach the cap; inspect diagnostics before deliberately retrying. There is no automatic paid/repeated retry or cache-service write.

## Toolchain and scope

- Linux: Ubuntu 24.04 x64, version-pinned Swift 6.2.1 Noble container
- Windows: Windows 2022 x64, Swift 6.2.1, explicitly selected Visual Studio 2022
- macOS: native macOS 15 ARM64 or Intel runner, Xcode 26.3 selected explicitly, minimum application target macOS 15, MoltenVK 1.4.1
- CMake 3.31.6, Ninja 1.11.1.3 Python package; Python 3.12.10 on Windows/macOS and Ubuntu's Python 3.12 on Linux
- Actions are commit-pinned; Cemu's vcpkg and Switch2Kit revisions remain gitlink-pinned. Hosted runner images and OS/Homebrew dependencies receive updates; this is not a bit-for-bit reproducible build. Missing selected Xcode fails rather than silently switching toolchains.

The existing platform build helpers enable `ENABLE_SWITCH2KIT=ON`, build the complete `CemuBin` target, and stage its native libraries and notices. The verifier checks the CMake option, host import of the Switch2Kit facade, required package contents, and macOS Bluetooth purpose/minimum OS/ad-hoc signature. It archives and extracts the staged tree outside the checkout, temporarily hides original build and binary directories, removes developer loader paths, then executes the extracted application's `--version` with a 30-second bound. Linux checks the `ldd` closure: the facade and Swift-family libraries must resolve inside the moved package; other dependencies must resolve from the package or OS library directories. macOS inspects every bundled Mach-O image and recursively checks library imports and runtime search paths for package-relative or OS locations. Windows recursively checks imports against packaged DLLs and OS/API-set dependencies, requiring Swift-family DLLs in the package. These checks cover static startup dependencies; they do not exercise optional libraries loaded later during gameplay. Original directories are restored on failures and timeouts.

`--version` returns before Cemu's controller/application initialization. Therefore this checks the relocated startup loader and package structure, **not** radio permission, pairing, controller input, motion, rumble, reconnect, graphics, gameplay, notarization, or release readiness. Artifacts are diagnostics, not redistributable packages. Native OS runs and real controller/game tests remain necessary before public release claims.

## Local verifier tests

From the repository root on Linux with Python 3.12+, a C compiler, Git and binutils:

```sh
python3 tests/switch2kit/test_qualify_package.py
```

These use tiny real ELF fixtures to exercise positive relocation and negative broken-package cases. They test the verifier; they do not compile Cemu or validate the Swift transport. For a real native build, run the documented platform build helper followed by:

```sh
python3 tests/switch2kit/qualify-package.py
```

On Windows use `python` instead of `python3`. Do not run the verifier while another process is using the same build or `bin` directory: it temporarily renames both, restoring them afterward. A hard process kill cannot run cleanup; after verifying no check is active, restore any `.qualification-unavailable` directories manually before rebuilding.
