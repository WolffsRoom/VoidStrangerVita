<p align="center">
  <img src="assets/repo/VoidStrangerVita.png" alt="Void Stranger running on PS Vita" width="900">
</p>

<p align="center">
  An <em>unofficial</em> native port of <strong>Void Stranger</strong> for the PlayStation Vita.
</p>

The project runs the original Windows/Steam GameMaker data through a customized version of [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch), with rendering provided by [VitaGL](https://github.com/Rinnegatamante/vitaGL). The Vita port includes native controls, multilingual support, Vita-specific graphics options, palette/tone control, runtime texture caching, native startup video playback, native trophies and optimized audio.
</p>

> This repository and its releases do not include the complete commercial game data. A legitimate Steam copy of **Void Stranger 1.1.3** is required.

## Project Status

<div align="center">
  <a href="https://github.com/WolffsRoom/VoidStrangerVita/releases"><img src="https://img.shields.io/github/downloads/WolffsRoom/VoidStrangerVita/total?style=for-the-badge&color=blue&logo=github" alt="Downloads"></a>
  <a href="https://github.com/WolffsRoom/VoidStrangerVita/releases/latest"><img src="https://img.shields.io/github/v/release/WolffsRoom/VoidStrangerVita?style=for-the-badge&color=brightgreen&logo=github" alt="Release"></a>
  <br>
  <img src="https://img.shields.io/badge/OVERALL_PROGRESS-95%25-ffc107?style=for-the-badge" alt="Progress">
  <img src="https://img.shields.io/badge/SOURCE-PC%2FSTEAM-004aa5?style=for-the-badge&logo=steam&logoColor=white" alt="Source">
  <img src="https://img.shields.io/badge/STATE-PLAYABLE-brightgreen?style=for-the-badge" alt="State">
</div>

<div align="center">

<br>
The game is fully playable, with virtually all rooms redesigned. Some FPS drops may occur during battles with heavy effects.
<br>

### Support this and other projects
_If you enjoy my work, consider supporting the development!_

[<img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="48">](https://www.buymeacoffee.com/5rsrt7j4z8f)

</div>

<br>

## Installation Guide

### Requirements

- A legally obtained Steam copy of **Void Stranger 1.1.3**;
- Plugin [kubridge](https://github.com/TheOfficialFloW/kubridge/releases/) (`kubridge.skprx`);
- Plugin [FdFix](https://github.com/TheOfficialFloW/FdFix/releases/) (`fd_fix.skprx`), unless you use rePatch;
- Plugin `libshacccg.suprx` installed on the Vita.
- Plugin [NoTrpDrm](https://github.com/Rinnegatamante/NoTrpDrm) (`NoTrpDrm.suprx`) for native trophy (_Optional_).

Add the kernel plugins to `ux0:tai/config.txt` under `*KERNEL`:

```text
*KERNEL
ux0:tai/kubridge.skprx
ux0:tai/fd_fix.skprx
```

> [!NOTE]
> Do not install `fd_fix.skprx` together with u have a rePatch Plugin.

### HOW TO APPLY THE PATCH:

The **Void Stranger Vita Patcher v2.1** requires an official, unmodified Steam installation of **Void Stranger 1.1.3**.

1. Purchase and install [Void Stranger on Steam](https://store.steampowered.com/app/2121980/Void_Stranger/).
2. Download `VoidStranger-vX.X.vpk` and `Void Stranger Vita Patcher vX.X.zip` from the [latest release](https://github.com/WolffsRoom/VoidStrangerVita/releases/latest).
3. Extract the patcher ZIP and run `VoidStrangerVitaPatcher.exe`.
4. Select or drag the official Steam `Void Stranger` installation folder when requested.
5. Choose an output directory and wait for the patcher to generate the `voidstranger` folder.
6. Copy the generated `voidstranger` folder to `ux0:data/` on the PS Vita.
7. Install `VoidStranger-vX.X.vpk` using VitaShell.
8. Start the game. On the **first boot**, the Vita generates its BC3/RGBA4444 texture cache automatically. This first startup can take longer than subsequent boots.

> [!IMPORTANT]
> Do **not** copy old `pvr/` or `texture-cache/` folders from previous builds. Current releases generate these caches directly on the Vita from the active `data.win`.

#### Folder Structure

Before the first boot, the generated data should look similar to:

```text
ux0:data/voidstranger/
|-- data.win
|-- audiogroup1.dat
|-- audiogroup2.dat
|-- voidstranger_data.csv
|-- options.ini
|-- splash.png
|-- intro/
|   |-- IntroVoidStranger.mp4
|   |-- LongVideo.mp4
|   `-- skip.png
`-- Languages/
    |-- EN/
    |-- FI/
    |-- ES/
    |-- FR/
    |-- IT/
    `-- PTBR/
```

After the first boot, the Vita creates its own `pvr/`, `texture-cache/` and save-related files as needed.

<table align="center">
  <tr>
    <td align="center" width="50%">
      <h3>Native loading and cache generation</h3>
      <img src="assets/new_load/preview_real_ingame_loading.gif" alt="Void Stranger Vita loading and texture generation" width="360">
      <br><br>
      The first boot prepares the Vita-side texture cache from the active game data. Later boots reuse the generated cache.
    </td>
    <td align="center" width="50%">
      <h3>Sera installation assistant</h3>
      <img src="assets/info_load/preview/missing_files_scene_preview.gif" alt="Sera missing-files installation assistant" width="360">
      <br><br>
      When required data is missing or misplaced, the v2.1 assistant checks <code>ux0:data/voidstranger/</code>, helps organize unambiguous files and points the player to the patcher when a clean data set is required.
    </td>
  </tr>
</table>

## Supported Languages

<table align="center">
  <thead>
    <tr>
      <th align="center">Language</th>
      <th align="center">Code</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center">English</td>
      <td align="center">EN</td>
    </tr>
    <tr>
      <td align="center">Finnish / Suomi</td>
      <td align="center">FI</td>
    </tr>
    <tr>
      <td align="center">Spanish / Español</td>
      <td align="center">ES</td>
    </tr>
    <tr>
      <td align="center">French / Français</td>
      <td align="center">FR</td>
    </tr>
    <tr>
      <td align="center">Italian / Italiano</td>
      <td align="center">IT</td>
    </tr>
    <tr>
      <td align="center">Brazilian Portuguese / Português do Brasil</td>
      <td align="center">PTBR</td>
    </tr>
  </tbody>
</table>

The multilingual implementation is based on work from [Void Stranger International](https://github.com/GiAnMMV/Void-Stranger-International), with Vita-specific integration and Brazilian Portuguese support included in the patcher.
## Control Layout

The Vita build uses the controller graphics stored in `assets/controls/`, matching the visual language of the in-game controller reference.
<table align="center">
  <thead>
    <tr>
      <th align="center">PS Vita</th>
      <th align="center">Control</th>
      <th align="left">Action</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center">D-Pad / Left Stick</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_8.png" width="24">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_9.png" width="24">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_10.png" width="24">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_11.png" width="24">
      </td>
      <td>Move / navigate</td>
    </tr>
    <tr>
      <td align="center">Cross</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_0.png" width="24">
      </td>
      <td>Action / confirm</td>
    </tr>
    <tr>
      <td align="center">Circle / Square</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_1.png" width="24">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_2.png" width="24">
      </td>
      <td>Secondary action / cancel</td>
    </tr>
    <tr>
      <td align="center">Triangle</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_3.png" width="24">
      </td>
      <td>Menu / original <code>C</code> action</td>
    </tr>
    <tr>
      <td align="center">Start</td>
      <td align="center">START</td>
      <td>Enter / pause / resume</td>
    </tr>
    <tr>
      <td align="center">Select</td>
      <td align="center">SELECT</td>
      <td>Escape / back</td>
    </tr>
    <tr>
      <td align="center">L</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_5.png" width="24">
      </td>
      <td>Original Page Down action</td>
    </tr>
    <tr>
      <td align="center">R</td>
      <td align="center">
        <img src="assets/controls/spr_menu_controllerlayout_duals_dark_4.png" width="24">
      </td>
      <td>Original Page Up action</td>
    </tr>
  </tbody>
</table>

### Touch support

The front touchscreen is used contextually rather than replacing the physical controls. In v2.1:

- menus and the in-game trophy browser accept touch input for selection and scrolling;
- the special fruit/orange interaction can be confirmed by a fresh touch on the front panel when `obj_orange.can_eat` is active;
- that fruit touch behaves like a single **Cross** press and intentionally does not auto-repeat while the screen is held;
- normal movement and regular gameplay remain on the Vita buttons, D-Pad and analog stick.

> [!NOTE]
> The in-game **Settings > Graphics** menu includes VSync, FPS mode, brightness, Stretch Screen and the original palette/tone presets. 
> _The obsolete desktop Scaling entry is hidden on Vita._

## Manual for PS Vita Edition

The VPK includes an eleven-page PS Vita manual. Small previews are shown below; the source pages live in `assets/manual/manual/`.

<details>
  <summary>
    <p align="center">
      <img src="assets/manual/manual/001.png" alt="Manual page 1" width="500">
      <br>
      <sub><b>Toque para visualizar o manual</b> (ainda em progresso).</sub>
    </p>
  </summary>

  <br>

  <p align="center">
    <img src="assets/manual/manual/002.png" alt="Manual page 2" width="150">
    <img src="assets/manual/manual/003.png" alt="Manual page 3" width="150">
    <img src="assets/manual/manual/004.png" alt="Manual page 4" width="150">
    <img src="assets/manual/manual/005.png" alt="Manual page 5" width="150">
    <img src="assets/manual/manual/006.png" alt="Manual page 6" width="150">
  </p>

  <p align="center">
    <img src="assets/manual/manual/007.png" alt="Manual page 7" width="150">
    <img src="assets/manual/manual/008.png" alt="Manual page 8" width="150">
    <img src="assets/manual/manual/009.png" alt="Manual page 9" width="150">
    <img src="assets/manual/manual/010.png" alt="Manual page 10" width="150">
    <img src="assets/manual/manual/011.png" alt="Manual page 11" width="150">
  </p>
</details>

## Native Trophies

Void Stranger Vita v2.1 includes a **30-entry trophy set** with native PS Vita trophy support, an in-game trophy browser and unlock notifications.

Native trophies use communication ID `VSTR00001_00`.

On startup, existing local unlocks are reconciled with the PS Vita trophy database so previously earned progress is preserved whenever possible.

| Trophy / event | Unlock condition |
| :--- | :--- |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **The Long Way** | Watch the full startup movie |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **Cut to the Chase** | Skip the startup movie |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **First Steps** | Make your first movement |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **First Descent** | Complete the first room transition |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **Memory Lane** | Open the Memories album |
| <img src="https://github.com/user-attachments/assets/6f35d4bd-7043-4cce-aa8d-0c75dafb041a" width="24"> **Game achievement sync** | Original game progress, including `VS_PENDANT`, is merged into the Vita trophy state |

Additional port-specific trophy events also track Vita-side actions such as changing **palette/tone** and **Stretch Screen** settings.

Trophy progress and unlock status can be reviewed directly from the in-game Vita settings interface.

> [!NOTE]
> `NoTrpDrm` is required on compatible real PS Vita systems for the native trophy pack to register correctly.

### NoTrpDrm setup

1. Download `NoTrpDrm.suprx` from the [NoTrpDrm releases](https://github.com/Rinnegatamante/NoTrpDrm/releases).
2. Copy it to the taiHEN directory used by your Vita (`ur0:tai/` is recommended when that is your active configuration).
3. Add it under `*main` in the active `config.txt`:

```text
*main
ur0:tai/NoTrpDrm.suprx
```

If your active taiHEN configuration is on `ux0:tai/`, use `ux0:tai/NoTrpDrm.suprx` instead. Reboot the Vita or reload taiHEN afterwards.

> [!IMPORTANT]
> The NoTrpDrm implementation used by this project targets retail firmware **3.60 through 3.68**. Firmware spoofing does not change the console's underlying firmware. Treat this custom trophy set as homebrew/local data and do not attempt to synchronize it with PSN.

## Screenshots

<table align="center">
  <tr>
    <td align="center">
      <img src="assets/prints/2026-10-04-210132-640041.png"
           alt="Void Stranger Vita - v2.1 gameplay"
           width="100%">
    </td>
    <td align="center">
      <img src="assets/prints/2026-10-04-210139-079717.png"
           alt="Void Stranger Vita - v2.1 Vita interface"
           width="100%">
    </td>
  </tr>

  <tr>
    <td align="center">
      <img src="assets/prints/2026-10-04-210145-302476.png"
           alt="Void Stranger Vita - v2.1 gameplay scene"
           width="100%">
    </td>
    <td align="center">
      <img src="assets/prints/2026-09-19-215712-711947.png"
           alt="Void Stranger Vita - gameplay"
           width="100%">
    </td>
  </tr>
</table>

---

## How It Works

Void Stranger Vita does not emulate Windows or run the original Windows executable. It reads the official GameMaker data and executes it through a customized native runner:

```text
Official Void Stranger 1.1.3 Steam files
                  |
                  v
        Void Stranger Vita Patcher
                  |
                  v
          Vita-compatible data.win
                  |
                  v
        Customized Butterscotch
                  |
                  v
          VitaGL + OpenAL
                  |
                  v
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


## IA Notice

GPT-5.6 and GPT-6.1 through Codex was used as a development assistant for diagnostics, implementation support, project organization and technical documentation. 

## Credits

Void Stranger Vita exists because of the work, research and patience of several people, communities and projects:

| Project / contributor | Contribution |
| :--- | :--- |
| **distheusurper2** | Extensive real-hardware testing, detailed issue reports and repeated validation throughout development. His patience and kindness made a large part of v2.0 possible; this project is as much his as mine, even if he would rather not take the credit. |
| [System Erasure](https://store.steampowered.com/app/2121980/Void_Stranger/) | Creators of **Void Stranger** and the original game, art, audio and design on which this port depends. |
| [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch) | GameMaker runner foundation used by the Vita port to execute the original Windows/Steam game data. |
| [VitaGL](https://github.com/Rinnegatamante/vitaGL) | Graphics layer used to bring the renderer to PS Vita hardware. |
| [Void Stranger International](https://github.com/GiAnMMV/Void-Stranger-International) | Foundation for the multilingual implementation and community language support. |
| [Void Stranger Wiki — Statues](https://voidstranger.miraheze.org/wiki/Statues) | Community documentation and reference material used while researching, implementing and validating statue-related behavior in the Vita port. |
| [YoYo Loader Vita compatibility research](https://github.com/Rinnegatamante/YoYo-Loader-Vita-Compatibility/issues/1121) | Prior PS Vita/GameMaker compatibility research and technical reference material. |
| [sinister-kid/voidstranger_vita](https://github.com/sinister-kid/voidstranger_vita) | Earlier community work and reference material for running Void Stranger on PS Vita. |

A special thanks also goes to the **Void Stranger community** for documenting the game's mechanics, secrets and behavior in such detail. Community research has been invaluable while reproducing and validating the original experience on PS Vita.

*Void Stranger and its assets belong to their respective owners. This is a community-made project and is not affiliated with System Erasure.*
