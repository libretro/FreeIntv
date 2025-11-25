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

## Enabling Onscreen Interactive Overlays

The FreeIntv core features onscreen interactive keypad overlays that display custom controller graphics directly on screen.

### To Enable the Feature:

**In RetroArch:**
1. **Launch a game** using the FreeIntv core (make sure a game is running)
2. **Open the Quick Menu** (usually by pressing `Hotkey + X` on your controller, or via the menu)
3. **Navigate to**: Options → Core Options
4. Enable the overlay option
5. **Close the menu** (the overlay takes effect immediately)
6. **Save Core Configuration** (Optional):
   - In the Quick Menu, select "Overrides" → "Save Core Overrides" to make this the default for all FreeIntv games
   - Or select "Save Game Overrides" to save the setting for this specific game only

The onscreen interactive overlay will now be active, allowing you to interact with it via touch or mouse clicks.

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
- **Android**: Touch the onscreen keypad overlay buttons to send input to the game
- **Windows/Linux**: Use the mouse to click on the overlay buttons to send input
- **Custom Overlays**: ROM-specific overlay images automatically load and display game-specific button layouts

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
