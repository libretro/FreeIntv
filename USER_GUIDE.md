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

## Using Game Overlays

Game overlay PNG files provide custom graphics for the keypad area, displaying the actual game control layout and instructions.

### Overlay Installation

**Folder Location:**
- Create a folder named `freeintv_overlays` in the same directory where your BIOS files are located
- Path: `retroarch/system/freeintv_overlays/`

**Overlay Image Requirements:**
- **Dimensions**: 370×600 pixels (required - core will not scale automatically)
- **Format**: PNG image (any color depth)
- **Naming**: Must match your ROM filename exactly (extension-insensitive)
  - ROM: `astrosmash.bin` → Overlay: `astrosmash.png`
  - ROM: `nightstalker.rom` → Overlay: `nightstalker.png`
  - ROM: `weekendwar.bin` → Overlay: `weekendwar.png`

**Installation Steps:**
1. Create the `freeintv_overlays` folder in your RetroArch system directory:
   ```
   retroarch/system/freeintv_overlays/
   ├── astrosmash.png
   ├── nightstalker.png
   ├── weekendwar.png
   └── (more game overlays as needed)
   ```
2. Place your 370×600 PNG overlay files in this folder, named to match your ROM files
3. Launch a game in FreeIntv - the overlay will load automatically if a matching PNG is found
4. No need to restart RetroArch after adding new overlays

## Touchscreen & Pointer Support
- **Android**: Touch the keypad area on the right side of the screen to send input to the game. If overlays are loaded, they display the game's specific control layout.
- **Windows/Linux**: Use the mouse to click on the keypad area. Mouse clicks are mapped to touch events for full overlay functionality.
- **Screen Position**: Use the RetroPad Select button to swap screen positions between normal (game left/keypad right) and swapped (keypad left/game right) layouts.

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
