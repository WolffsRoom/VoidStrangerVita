<p align="center">
  <img src="assets/repo/LogoVoidVita.png" alt="Void Stranger Vita" width="512">
</p>
<p align="center">
  <img src="assets/repo/VoidStrangerVita.png" alt="Void Stranger running on PS Vita" width="900">
</p>

An _unofficial_ native port of **Void Stranger 1.1.3** for the PlayStation Vita.

The project runs the original Windows/Steam GameMaker data through a customized version of [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch), with rendering provided by [VitaGL](https://github.com/Rinnegatamante/vitaGL). The Vita port includes native controls, multilingual support, Vita-specific graphics options, palette/tone control, runtime texture caching and optimized audio.

> This repository and its releases do not include the complete commercial game data. A legitimate Steam copy of **Void Stranger 1.1.3** is required.

## Project Status

<div align="center">
  <a href="https://github.com/WolffsRoom/VoidStrangerVita/releases"><img src="https://img.shields.io/github/downloads/WolffsRoom/VoidStrangerVita/total?style=for-the-badge&color=blue&logo=github" alt="Downloads"></a>
  <a href="https://github.com/WolffsRoom/VoidStrangerVita/releases/latest"><img src="https://img.shields.io/github/v/release/WolffsRoom/VoidStrangerVita?style=for-the-badge&color=brightgreen&logo=github" alt="Release"></a>
  <br>
  <img src="https://img.shields.io/badge/OVERALL_PROGRESS-80%25-ffc107?style=for-the-badge" alt="Progress">
  <img src="https://img.shields.io/badge/SOURCE-PC%2FSTEAM-004aa5?style=for-the-badge&logo=steam&logoColor=white" alt="Source">
  <img src="https://img.shields.io/badge/STATE-PLAYABLE-brightgreen?style=for-the-badge" alt="State">
</div>

<br>

The current build boots and is playable on real PS Vita hardware. The port currently provides:

- native Vita executable using Butterscotch + VitaGL;
- 960x544 presentation with the original pixel-art render target;
- physical Vita controls and analog movement;
- OpenAL audio with optimized audio banks;
- Vita-side BC3/RGBA4444 texture cache generation;
- automatic cache invalidation when `data.win` changes;
- in-game graphics settings adapted for Vita, including VSync, brightness, screen stretch and palette/tone selection;
- English, Finnish, Spanish, French, Italian and Brazilian Portuguese;
- diagnostic logging at `ux0:data/voidstranger/butterscotch-probe.log`.

Further full-game hardware validation and optimization are still in progress.

<div align="center">

### Support this and other projects
If you enjoy my work, consider supporting the development!

[<img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="48">](https://www.buymeacoffee.com/5rsrt7j4z8f)

</div>

<br>

## Installation Guide

### Requirements

- A legally obtained Steam copy of **Void Stranger 1.1.3**;
- [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) (`kubridge.skprx`);
- [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) (`fd_fix.skprx`), unless you use rePatch;
- `libshacccg.suprx` installed on the Vita.

Add the kernel plugins to `ux0:tai/config.txt` under `*KERNEL`:

```text
*KERNEL
ux0:tai/kubridge.skprx
ux0:tai/fd_fix.skprx
```

> [!NOTE]
> Do not install `fd_fix.skprx` together with rePatch.

### HOW TO APPLY THE PATCH:

The **Void Stranger Vita Patcher v1.0** requires an official, unmodified Steam installation of **Void Stranger 1.1.3**.

1. Purchase and install [Void Stranger on Steam](https://store.steampowered.com/app/2121980/Void_Stranger/).
2. Download `VoidStranger.vpk` and `Void Stranger Vita Patcher v1.0.zip` from the [latest release](https://github.com/WolffsRoom/VoidStrangerVita/releases/latest).
3. Extract the patcher ZIP and run `VoidStrangerVitaPatcher.exe`.
4. Select or drag the official Steam `Void Stranger` installation folder when requested.
5. Choose an output directory and wait for the patcher to generate the `voidstranger` folder.
6. Copy the generated `voidstranger` folder to `ux0:data/` on the PS Vita.
7. Install `VoidStranger.vpk` using VitaShell.
8. Start the game. On the **first boot**, the Vita generates its BC3/RGBA4444 texture cache automatically. This first startup can take longer than subsequent boots.

> [!IMPORTANT]
> Do **not** copy old `pvr/` or `texture-cache/` folders from previous builds. Current releases generate these caches directly on the Vita from the active `data.win`.

Before the first boot, the generated data should look similar to:

```text
ux0:data/voidstranger/
├── data.win
├── audiogroup1.dat
├── audiogroup2.dat
├── voidstranger_data.csv
├── options.ini
├── splash.png
└── Languages/
    ├── EN/
    ├── FI/
    ├── ES/
    ├── FR/
    ├── IT/
    └── PTBR/
```

After the first boot, the Vita creates its own `pvr/`, `texture-cache/` and save-related files as needed.

## Supported Languages

| Language | Code |
| :--- | :---: |
| English | EN |
| Finnish / Suomi | FI |
| Spanish / Español | ES |
| French / Français | FR |
| Italian / Italiano | IT |
| Brazilian Portuguese / Português do Brasil | PTBR |

The multilingual implementation is based on work from [Void Stranger International](https://github.com/GiAnMMV/Void-Stranger-International), with Vita-specific integration and Brazilian Portuguese support included in the patcher.

## Control Layout

The Vita controls map the original PC actions to the handheld layout:

| PS Vita | Action |
| :--- | :--- |
| D-Pad / Left Stick | Move / navigate |
| Cross | Action / confirm |
| Circle or Square | Secondary action / cancel |
| Triangle | Menu / original `C` action |
| Start | Enter / pause / resume |
| Select | Escape / back |
| L | Original Page Down action |
| R | Original Page Up action |

The in-game **Settings → Graphics** menu also includes Vita-specific options such as VSync, brightness, stretch screen and the original palette/tone presets.

## Screenshots

<p align="center">
  <img src="assets/prints/Print1.png" alt="Void Stranger Vita screenshot 1" width="48%">
  <img src="assets/prints/Print2.png" alt="Void Stranger Vita screenshot 2" width="48%">
</p>

## How It Works

Void Stranger Vita does not emulate Windows or run the original Windows executable. It reads the official GameMaker data and executes it through a customized native runner:

```text
Official Void Stranger 1.1.3 Steam files
                  ↓
          Void Stranger Vita Patcher
                  ↓
        Vita-compatible data.win
                  ↓
        Customized Butterscotch
                  ↓
          VitaGL + OpenAL
                  ↓
              PS Vita
```

Texture pages are decoded from the active `data.win` on the Vita. BC3-compatible pages are compressed and cached as PVR data, while pages that need a lossless/UI-safe path use RGBA4444 caching. A fingerprint tied to `data.win` prevents stale caches from being reused after a rebuild.

## Build Instructions (For Developers)

### Requirements

- Windows 10 or 11;
- PowerShell 5.1 or newer;
- Docker Desktop with Linux containers enabled;
- Git;
- an official and unmodified Steam installation of Void Stranger 1.1.3.

Place the official game files under:

```text
data/Void.Stranger.v1.1.3/
```

Prepare the Vita data:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare-voidstranger-data.ps1 -Force
```

Build the VPK:

```bat
run_build.cmd
```

The VPK is generated at:

```text
artifacts/VoidStranger.vpk
```

The prepared data is generated at:

```text
data/prepared/voidstranger/
```

### Building the Windows patcher

The patcher project is located at:

```text
artifacts/Patcher/Build_Patch/
```

Run:

```bat
Build_Patcher.bat
```

The build process regenerates the binary `data.win` patch, embeds the optimized audio and supported language files, then creates `VoidStrangerVitaPatcher.exe` with PyInstaller.

> [!IMPORTANT]
> Commercial Void Stranger data must never be committed or distributed. Releases should contain only the VPK, patcher/patch data and other files required to transform a legitimate Steam installation.

## Credits

- [System Erasure](https://store.steampowered.com/app/2121980/Void_Stranger/) — Void Stranger;
- [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch);
- [VitaGL](https://github.com/Rinnegatamante/vitaGL);
- [Void Stranger International](https://github.com/GiAnMMV/Void-Stranger-International);
- [YoYo Loader Vita compatibility research](https://github.com/Rinnegatamante/YoYo-Loader-Vita-Compatibility/issues/1121);
- [sinister-kid/voidstranger_vita](https://github.com/sinister-kid/voidstranger_vita).

Void Stranger and its assets belong to their respective owners. This is a community-made project and is not affiliated with System Erasure.
