# FreeIntv User Guide

## Overview
FreeIntv is a libretro core for emulating the Mattel Intellivision, featuring enhanced touchscreen support and a dual-screen overlay UI. This guide explains how to set up and use the core in RetroArch, including instructions for overlay PNG files.

---

## Requirements
- RetroArch (latest recommended)
- FreeIntv core (`freeintv_libretro.so`, `.dll`, or `.dylib`)
- Intellivision BIOS files: `exec.bin`, `grom.bin` (place in RetroArch `system` folder)
- Game ROMs (Intellivision format)
- Overlay PNG files (for custom keypad/utility UI)

---

## Installation
1. **Install RetroArch**
   - Download from https://www.retroarch.com/

2. **Install FreeIntv Core & Info file**
   - Place the core binary for your platform into RetroArch's `cores` directory:
     - Windows: `FreeIntv_libretro.dll`
     - Linux: `FreeIntv_libretro.so`
     - macOS: `FreeIntv_libretro.dylib`
   - Important: Ensure the core filename is exactly `FreeIntv_libretro.*` (remove any extra platform tags or brackets added by ZIP extractors).
   - Copy the `.info` file to RetroArch's `info` folder (this lets RetroArch associate ROM extensions with the core):
     - Example path (Windows): `%APPDATA%\RetroArch\info\FreeIntv_libretro.info`
     - If RetroArch places info files elsewhere on your platform, copy it to that `info/` folder.

3. **Add BIOS Files**
   - Place `exec.bin` and `grom.bin` in the RetroArch `system` folder. Filenames are case-sensitive.
   - Example (PowerShell):
```powershell
Copy-Item .\exec.bin C:\retroarch-win64\system\exec.bin -Force
Copy-Item .\grom.bin C:\retroarch-win64\system\grom.bin -Force
```

4. **Add Game ROMs**
   - Place Intellivision ROM files in your preferred ROMs directory (e.g. `retroarch/roms/intellivision/`).
   - Supported extensions: `.intv`, `.bin`, `.rom` (the core advertises `intv|bin|rom`).
   - If RetroArch prompts which core to use when loading a ROM, it means the `.info` file wasn't recognized; see "Refreshing RetroArch core info" below.

---

## Using Overlay PNG Files
Overlay PNG files provide custom graphics for the keypad and utility buttons.

### Required PNG Files
The following PNG files should be placed in `system/FreeIntv_image_assets/`:

**Controller Template (Required for keypad display):**
- `controller_base.png` - Base keypad template layer
- `default.png` - Default overlay when ROM-specific overlay is not found

**Utility Buttons (Optional but recommended for UI):**
- `button_toggle_layout.png` - Toggles game and keypad positions in dual-screen mode
- `button_full_screen_toggle.png` - Toggles fullscreen mode in dual-screen mode
- `button_full_screen_overlay.png` - Toggles overlay visibility in fullscreen mode

**Game-Specific Overlays (Optional):**
- Named to match ROM filename (e.g., `Astrosmash.png` for `Astrosmash.rom`)
- Size: approximately 370×600 pixels

### Where to Place Overlay PNG Files
   - Place overlays and core assets into the RetroArch `system` folder under the `FreeIntv_image_assets` subfolder. Example:
     - `retroarch/system/FreeIntv_image_assets/`
   - Copy both the controller/template asset ZIP and the overlays ZIP contents into the same folder so default keypad and utility button PNGs are available.

### Naming and ROM-specific overlays
   - Recommended: name the PNG to match the ROM filename (without extension). Example:
     - ROM: `4-tris.rom` → Overlay: `4-tris.png`

### Refreshing RetroArch core info and overlays
   - RetroArch caches core info and will only list ROM extensions that match the `.info` file for a core. If you install a new core or update its `.info` file, do the following:
     - Exit RetroArch fully.
     - Delete RetroArch's info/core cache (paths vary by platform). On desktop installs you may find an `info/` cache under the RetroArch config directory (or `config/.retroarch/`).
     - On Android the core info lives under the RetroArch app data (for example `/data/data/com.retroarch/files/.retroarch/cores/`). Remove or replace the `FreeIntv_libretro.info` there and restart RetroArch.
   - If RetroArch still prompts for a core when opening a ROM, the `.info` file didn't get picked up. Re-copy the `.info` file to RetroArch's `info/` or `cores/` location and restart RetroArch.

### Optional: create a small ROM database (.rdb)
   - RetroArch uses system ROM databases to enrich playlists and sometimes to help with filtering. You can add a minimal `.rdb` for convenience; place it in RetroArch's `database/rdb/` folder.


## Download Core Assets

You will need to download and set up two separate components:

1. **Core Assets Archive:**
   - [freeintv_image_assets.zip](https://github.com/jcarr71/FreeIntvTSOverlay/releases/latest/download/freeintv_image_assets.zip)
   - This contains the controller template, default keypad, and utility button PNGs.

2. **Overlay Archive:**
   - [overlays.zip](https://github.com/jcarr71/FreeIntvTSOverlay/releases/latest/download/overlays.zip)
   - This contains game-specific overlays for individual games.

### Installation Instructions:
1. **Extract freeintv_image_assets.zip to the system folder:**
   - Download `freeintv_image_assets.zip`
   - Extract it to your RetroArch `system` folder (same location where you placed `exec.bin` and `grom.bin`)
   - This creates a `FreeIntv_image_assets` folder in your system directory
   - Result: `retroarch/system/FreeIntv_image_assets/` contains `controller_base.png`, `default.png`, and button PNGs

2. **Extract overlays.zip to the FreeIntv_image_assets folder:**
   - Download `overlays.zip`
   - Extract it into the `FreeIntv_image_assets` folder you just created
   - This creates an `overlays` subfolder within `FreeIntv_image_assets`
   - Result: `retroarch/system/FreeIntv_image_assets/overlays/` contains all game-specific overlay PNG files

3. **Verify the folder structure:**
   ```
   retroarch/system/
   ├── exec.bin
   ├── grom.bin
   └── FreeIntv_image_assets/
       ├── controller_base.png
       ├── default.png
       ├── button_full_screen_toggle.png
       ├── button_full_screen_overlay.png
       ├── button_toggle_layout.png
       └── overlays/
           ├── 4-tris.png
           ├── Astrosmash.png
           ├── BattleCartridge.png
           └── (other game overlays...)
   ```

4. **Restart RetroArch** if it is running.

Your overlays and assets will now be available for use with the FreeIntv core.

## Touchscreen UI

## Touchscreen & Mouse Support
- On Android, touch the keypad area to send input to the emulator.
- On Windows and Linux, you can use the mouse to click on the keypad overlay and utility buttons. Mouse clicks are mapped to touch events, allowing full use of the overlay UI features.

### Utility Buttons (Below Game Screen)

The FreeIntv core provides three utility buttons in dual-screen mode:

#### 1. Toggle Layout Button (`button_toggle_layout.png`)
- **Function:** Switches the positions of the game screen and keypad overlay
- **Usage:** Click this button to swap which side displays the game and which displays the keypad
- **Use Case:** Convenient for left-handed players or adjusting layout preference without leaving the game

#### 2. Fullscreen Toggle Button (`button_full_screen_toggle.png`)
- **Function:** Switches between dual-screen mode and fullscreen game-only mode
- **Usage:** Click to hide the keypad overlay and maximize game screen size
- **Dual-Screen Mode:** Game (704×448 2x-scaled) + Keypad (370×600) side-by-side = 1074×600 total
- **Fullscreen Mode:** Game screen expanded to fill the display; keypad overlay hidden
- **Visual Indicator:** Button highlights yellow when pressed

#### 3. Overlay Toggle Button (Fullscreen Mode Only) (`button_full_screen_overlay.png`)
- **Function:** Toggles overlay visibility when in fullscreen mode
- **Availability:** Only active and visible when fullscreen toggle has switched to fullscreen mode
- **Usage:** Click to temporarily show/hide the keypad overlay in fullscreen mode
  - **Show Overlay:** Press this button to display the keypad overlay on top of the fullscreen game
  - **Hide Overlay:** Press again to hide the overlay and maximize game view
- **Visual Feedback:** Keypad area highlights in green to show touch-active buttons when overlay is visible
- **Dual-Screen Toggle:** If you press the "Toggle Layout" button while in fullscreen overlay mode, the keypad position swaps (left/right)

### Utility Button Auto-Hide Feature (Fullscreen Mode)

When in fullscreen mode, the utility buttons automatically hide after 5 seconds of inactivity to maximize game visibility:

- **Auto-Hide Behavior:** The utility button strip (50 pixels tall) slides out of view 5 seconds after the last button press
- **Bringing Buttons Back:** To reveal the hidden utility buttons, swipe or touch from the **bottom of the screen upward** (or click within the bottom 80 pixels of the screen)
  - This gesture resets the auto-hide timer and brings the button strip back into view
- **Timer Reset:** Any button press resets the 5-second countdown timer, so actively using buttons will keep them visible
- **Why This Exists:** Maximizes game screen space during gameplay while keeping buttons accessible when needed

### Fullscreen Mode Workflow

1. **Start in Dual-Screen Mode** (default)
   - See game on one side, keypad on the other
   - All three utility buttons visible below game screen

2. **Enter Fullscreen Mode**
   - Click "Fullscreen Toggle" button
   - Game expands to fill display; keypad hidden
   - Utility buttons disappear (only visible in dual-screen mode)

3. **Show Keypad in Fullscreen**
   - Click "Overlay Toggle" button (appears when in fullscreen mode)
   - Keypad overlay displays on top of game screen
   - Touch keypad buttons to send input while seeing the full game

4. **Toggle Keypad Position (Optional)**
   - While in fullscreen overlay mode, click "Toggle Layout" to swap keypad left/right
   - Useful if your usual keypad side is obscured by other UI elements

5. **Exit Fullscreen**
   - Click "Fullscreen Toggle" again to return to dual-screen mode
   - Keypad overlay disappears and both screens display side-by-side

---

## Troubleshooting
- If overlays do not appear, verify PNG file location and naming.
- Ensure BIOS files are present in the `system` folder.
- For touchscreen issues, check RetroArch input settings and core options.

### If RetroArch asks which core to use when opening a ROM
 - This means the `.info` file for the core is not recognized by RetroArch (or cached metadata is stale).
 - Steps to resolve:
    1. Make sure the `FreeIntv_libretro.info` file is copied into RetroArch's `info/` folder, and that the filename matches the core filename (excluding extension).
    2. Fully quit RetroArch and restart it to force a rescan.
    3. If needed, delete RetroArch's info cache (desktop: `info/` or config cache; Android: app data `.retroarch/cores/`) and restart.
    4. Rebuild playlists or use "Load Core" first, then "Load Content" and navigate to the ROM.

### Android-specific deployment notes
 - Place the correct `libfreeintv_libretro.so` for your device ABI into the RetroArch cores directory on the device.
    - Example install via ADB (replace ABI path and device paths as needed):
```bash
adb push libs/arm64-v8a/libfreeintv_libretro.so /sdcard/Download/
# then move the file on-device with a file manager or via shell to the RetroArch cores location
adb shell "su -c 'mv /sdcard/Download/libfreeintv_libretro.so /data/data/com.retroarch/files/.retroarch/cores/'"
```
 - Also ensure the `.info` file is present on the device under RetroArch's info/core cache so Android's file browser can filter `.intv|.bin|.rom` properly.

### If the core crashes on load
 - Verify you used the correct ABI binary for your device (arm64-v8a vs armeabi-v7a vs x86/x86_64).
 - Make sure the BIOS files (`exec.bin`, `grom.bin`) are present and readable.
 - Check RetroArch logs (logcat on Android or retroarch.log on desktop) to see why the core failed.

## Additional Resources
- [RetroArch Documentation](https://docs.libretro.com/)
- [FreeIntv GitHub](https://github.com/libretro/FreeIntv)

---

**Last Updated:** November 2025
