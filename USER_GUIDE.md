# FreeIntv with Onscreen Interactive Keypad Overlays

## Overview
FreeIntv is a libretro emulation core for the Mattel Intellivision designed to be compatible with joypads from the SNES era forward even if they originally required a number pad. This guide explains how to set up and use the core in RetroArch, including instructions for overlay PNG files.

**Current Version**: Enhanced with onscreen interactive keypad overlays for Android, Windows, and Linux (November 24, 2025)

## Authors & Contributors

FreeIntv was created by David Richardson.
The PSG and STIC emulation was made closer to hardware and optimized by Oscar Toledo G. (nanochess), who also added save states.

The Intellivoice code has been contributed by Joe Zbiciak (author of jzintv), and adapted by Oscar Toledo G. (nanochess)

**Onscreen Overlay Enhancement**: Onscreen interactive keypad overlays with touchscreen (Android) and mouse (Windows/Linux) support (November 2025) by Jason Carr

---

## Requirements
- RetroArch (latest recommended)
- FreeIntv core (`FreeIntv_libretro.so`, `.dll`, or `.dylib`)
- Intellivision BIOS files: `exec.bin`, `grom.bin` (place in RetroArch `system` folder)
- Game ROMs (Intellivision format)

---

## Installation

**Step 1: Install FreeIntv Core**
- Place the compiled core file for your platform into RetroArch's `cores/` directory:
  - Windows: `FreeIntv_libretro.dll`
  - Linux: `FreeIntv_libretro.so`
  - macOS: `FreeIntv_libretro.dylib`

**Step 2: Install the Core Info File**
- Copy `FreeIntv_libretro.info` to RetroArch's `info/` directory
- ⚠️ The filename must match the core filename exactly (excluding the file extension)

**Step 3: Add BIOS Files**
- Place the two required Intellivision BIOS files in RetroArch's `system/` directory:
  - `exec.bin` (Executive ROM)
  - `grom.bin` (Graphics ROM)

**Step 4: Launch Games**
- Load an Intellivision ROM through RetroArch using the FreeIntv core

---

## Enabling Onscreen Interactive Overlays

The FreeIntv core features onscreen interactive keypad overlays that display custom controller graphics directly on screen, allowing touch or mouse input.

### To Enable the Feature:

1. **Launch a game** using the FreeIntv core
2. **Open the Quick Menu** (usually `Hotkey + X` on your controller, or through RetroArch's menu)
3. **Navigate to**: Options → Core Options
4. Enable the overlay feature
5. **Close the menu** (RESTART to take effect)
6. **Revert to fullscreen** follow same instructions but disable core option, and restart to take effect.

The onscreen interactive overlay will now be active, allowing you to touch or click buttons directly on screen!

## Using Game Overlays

Mattel Intellivision games were often meant to be played with game-specific cards overlaid on the numeric keypad. These overlays convey information which can be very useful in gameplay. Images of a limited selection of Intellivision titles are available at: http://www.intellivisionlives.com/bluesky/games/instructions.shtml

### Overlay Setup Instructions

To use custom game overlays with the touchscreen UI on Android or mouse input on Windows/Linux:

**Folder Location:**
- Create a folder named `freeintv_overlays` in the same directory where you keep your BIOS files (`exec.bin` and `grom.bin`)
- Typically: `RetroArch/system/freeintv_overlays/`

**Overlay Image Specifications:**
- **Dimensions**: 370×600 pixels
- **Format**: PNG image (any color depth)
- **Naming**: Match your ROM filename exactly (case-insensitive on most systems)
  - Example: If your ROM is `astrosmash.bin`, name the overlay `astrosmash.png`
  - Example: If your ROM is `nightstalker.rom`, name the overlay `nightstalker.png`

**Example Folder Structure:**
```
system/
├── exec.bin
├── grom.bin
└── freeintv_overlays/
    ├── astrosmash.png
    ├── nightstalker.png
    ├── weekendwar.png
    └── (additional game overlays...)
```
