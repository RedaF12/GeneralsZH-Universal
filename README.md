<div align="center">

# Command & Conquer: Generals — Zero Hour for Android

### Adreno 840 allocation-crash fix · DXVK/Vulkan stability · OpenAL audio fix

[![Build Android APK](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/actions/workflows/build-android.yml/badge.svg)](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/actions/workflows/build-android.yml)
[![Download APK](https://img.shields.io/badge/download-APK%20v118-FF6D00?logo=android&logoColor=white)](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/releases/download/v118/Generals-Zero-Hour-Android-Adreno840-v118.apk)
[![Android](https://img.shields.io/badge/platform-Android-3DDC84?logo=android&logoColor=white)](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/actions/workflows/build-android.yml)
[![ARM64](https://img.shields.io/badge/architecture-ARM64-0091BD?logo=arm&logoColor=white)](#requirements)
[![Adreno 840](https://img.shields.io/badge/tested-Adreno%20840-76B900)](#stability-fixes)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE.md)

**An Android-focused stability fork of GeneralsX for running Zero Hour on modern Snapdragon/Adreno devices.**

<img src="https://github.com/user-attachments/assets/aeaf6692-36e6-40c8-b9f8-8066d014ec4b" alt="Command & Conquer Generals: Zero Hour running on Android" width="820">

</div>

> [!IMPORTANT]
> This repository contains engine source code only. It does **not** include Command & Conquer game data, artwork, music, videos, or other proprietary assets. You must own a legal copy of *Command & Conquer: Generals — Zero Hour* and provide its data files yourself.

## Why this fork exists

The upstream project supports several operating systems. This fork has one priority: make the **Android version stable and practical on real phones**, with special attention to Adreno 840 and the Qualcomm Vulkan driver.

Desktop source code is retained where Android shares it, so the mobile build is not broken by deleting common engine components. Desktop CI is manual-only; automatic development and validation are focused on Android.

## Stability fixes

### DXVK allocation crash

The Android DXVK path could corrupt its resource-allocation pool and repeatedly report:

```text
DxvkResourceAllocationPool: corrupted free list head
```

This fork applies an Android-only allocation fallback that bypasses the affected pooled free-list path. Other platforms keep their original allocator behavior.

### Qualcomm/Adreno driver crashes

Allocation corruption eventually surfaced as native crashes inside the stock Qualcomm `vulkan.adreno.so` driver. Removing the corrupted allocator path prevents invalid resource state from reaching the driver while keeping the existing Vulkan renderer and DXVK integration intact.

### OpenAL audio cleanup

Reused OpenAL sources previously kept old buffers attached while the buffer cache attempted to delete them, causing repeated `alDeleteBuffers` failures. Sources are now stopped and detached before cache eviction in 2D, 3D, and looping playback paths.

### Real-device validation

The current fix set was built as Android version code **118**, installed over USB, and tested in live gameplay on the target Adreno device.

| Signal checked | Result |
|---|---:|
| DXVK corrupted free-list reports | **0** |
| OpenAL `alDeleteBuffers` failures | **0** |
| Engine throw markers | **0** |
| Native crashes | **0** |

This is a focused real-device validation, not a claim that every phone, ROM, driver, map, or mod is already perfect. Useful failure reports are welcome.

## Download and install

### [Download the tested Android APK — v118](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/releases/download/v118/Generals-Zero-Hour-Android-Adreno840-v118.apk)

The APK is also available from the [v118 release page](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/releases/tag/v118), together with its checksum and fix summary.

1. Download and open the APK on an ARM64 Android device.
2. Install it. Version code 118 can replace any installed build with a lower version code.
3. Supply the game data from your legally owned Zero Hour installation when the app requests it.

Android may ask you to allow installation from the browser or file manager used to open the APK. No root access is required for the normal installation path.

## Requirements

- 64-bit Android device (`arm64-v8a`)
- Vulkan-capable GPU and driver
- A legally owned Zero Hour installation for game data
- Enough free storage for the APK, native libraries, and game files

The primary target is **Snapdragon with Adreno 840**. Other Vulkan-capable ARM64 devices may work, but should be treated as unverified until tested.

## Build the APK

The supported build path is the existing GitHub Actions workflow—there is no duplicate Android workflow in this fork.

1. Open [**Build Android APK**](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/actions/workflows/build-android.yml).
2. Choose **Run workflow**.
3. Keep the `android-vulkan` preset.
4. Set a `version_code` higher than the build already installed on your phone.
5. Optionally set `version_name` and enable release publishing.

The workflow uses pinned Android tooling and builds the engine, DXVK integration, native dependencies, and APK in one reproducible job.

## Current scope

| Area | Status |
|---|---|
| Android ARM64 build | Active priority |
| Vulkan / DXVK | Active priority |
| Adreno 840 stability | Tested target |
| Touch controls | Supported by the Android port |
| Audio | OpenAL lifecycle fix applied |
| Linux / macOS / Windows CI | Manual only |

## Known limitations

- Some custom WAV files can still produce an FFmpeg `wrong sample_count` warning. It did not crash the validated gameplay session, but malformed audio metadata should eventually be cleaned up at the asset level.
- Vulkan behavior can vary between Qualcomm driver releases and vendor ROM updates.
- Mods and custom maps may expose engine paths not covered by the current device test.
- The project is under active development; keep backups of saves and custom data.

## Reporting a problem

Open an [issue](https://github.com/txllthemall/GeneralsZH-Android-Adreno840-Allocation-Fix/issues) and include:

- phone model, SoC/GPU, Android version, and ROM;
- whether the stock Qualcomm driver or a custom driver was used;
- the exact APK version code;
- steps to reproduce the problem;
- a complete Android log captured around the crash.

Please do not upload proprietary game files with a report.

<details>
<summary><strong>Коротко по-русски</strong></summary>

Это Android-ориентированный форк GeneralsX для стабильного запуска Zero Hour на современных Snapdragon/Adreno. В нём устранён проблемный DXVK allocation-pool на Android и исправлено освобождение OpenAL-буферов. Сборка проверена в реальном бою на целевом устройстве: повторов ошибки free list, ошибок удаления аудиобуферов и нативных падений в тесте не было.

Игровых ресурсов в репозитории нет — нужна собственная легальная копия Zero Hour. APK собирается существующим workflow **Build Android APK** во вкладке Actions.

</details>

## Project lineage and credits

This work builds on the open-source Command & Conquer code release and the GeneralsX/Thyme community effort. Credit belongs to the upstream maintainers and contributors whose work made the Android port possible, including [MYSOREZ/GeneralsZH-Android-Port](https://github.com/MYSOREZ/GeneralsZH-Android-Port) and its upstream history.

Command & Conquer and related names are trademarks of Electronic Arts Inc. This is an unofficial, community-maintained project and is not endorsed by or affiliated with Electronic Arts.

Source code is distributed under the terms in [LICENSE.md](LICENSE.md). Additional bundled dependencies retain their respective licenses.
