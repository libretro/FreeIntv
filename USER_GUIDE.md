# FreeIntv User Guide

## Overview
FreeIntv is a libretro core for emulating the Mattel Intellivision, featuring enhanced touchscreen support and a dual-screen overlay UI. This guide explains how to set up and use the core in RetroArch, including instructions for overlay PNG files.

---

## Requirements
- RetroArch (latest recommended)
- FreeIntv core (`FreeIntv_libretro.so`, `.dll`, or `.dylib`)
- Intellivision BIOS files: `exec.bin`, `grom.bin` (place in RetroArch `system` folder)
- Game ROMs (Intellivision format)
- Overlay PNG files (for custom keypad/utility UI)

---

## Installation
1. **Install RetroArch**
   - Download from https://www.retroarch.com/
2. **Install FreeIntv Core**
   - Place the core file in RetroArch's `cores` directory
   - Load the core via RetroArch's "Load Core" menu
3. **Add BIOS Files**
   - Place `exec.bin` and `grom.bin` in the `system` folder inside your RetroArch directory
4. **Add Game ROMs**
   - Place Intellivision ROM files in your preferred ROMs directory

---

## Using Overlay PNG Files
Overlay PNG files provide custom graphics for the keypad and utility banner.


### Where to Place Overlay PNG Files
   - Base folder: `retroarch/system/freeIntv_image_assets/`
   - Game-specific overlays: `retroarch/system/freeIntv_image_assets/overlays/`


## Asset Installation

The FreeIntv core requires 3 core asset files, plus optional user-provided overlays:

**Required Core Assets (3 files) - Place in `retroarch/system/freeIntv_image_assets/`:**
1. **banner.png** (704×152 pixels)
   - Purpose: Fills the utility workspace with interactive toggle button
   - Status: REQUIRED

2. **controller_base.png**
   - Purpose: Background for keypad overlay
   - Status: Required

3. **default.png**
   - Purpose: Fallback overlay if ROM-specific overlay not found
   - Status: Required

**User-Provided Game Overlays - Place in `retroarch/system/freeIntv_image_assets/overlays/`:**
- Named to match ROM filenames (e.g., `astrosmash.png` for `astrosmash.bin`)
- One PNG per game (optional, but recommended for best experience)

**Installation Instructions:**
1. Create the folder structure in your RetroArch system directory:
   ```
   retroarch/system/freeIntv_image_assets/
   ├── banner.png
   ├── controller_base.png
   ├── default.png
   └── overlays/
       ├── astrosmash.png
       ├── nightstalker.png
       └── (more game overlays)
   ```
2. Place the 3 core asset files in `freeIntv_image_assets/`
3. Place game-specific overlays in `freeIntv_image_assets/overlays/`
4. Restart RetroArch if it is running

## Touchscreen & Pointer Support
- On Android, touch the keypad area to send input to the emulator.
- On Windows and Linux, you can use the mouse to click on the keypad overlay. Mouse clicks are mapped to touch events, allowing full use of the overlay UI features.
- **Swap Screen Button**: A toggle button in the utility banner switches between two display layouts:
  - Normal: Game screen on left, keypad on right
  - Swapped: Keypad on left, game screen on right

---

## Troubleshooting
- If overlays do not appear, verify PNG file location and naming.
- Ensure BIOS files are present in the `system` folder.
- For touchscreen issues, check RetroArch input settings and core options.

---

## Additional Resources
- [RetroArch Documentation](https://docs.libretro.com/)
- [FreeIntv GitHub](https://github.com/jcarr71/FreeIntv)

---

**Last Updated:** October 2025
