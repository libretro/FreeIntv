# FreeIntv with Android Touchscreen Support
FreeIntv is a libretro emulation core for the Mattel Intellivision designed to be compatible with joypads from the SNES era forward even if they originally required a number pad.

**Current Version**: Enhanced with full touchscreen support for Android devices (October 24, 2025)

## Authors & Contributors

FreeIntv was created by David Richardson.
The PSG and STIC emulation was made closer to hardware and optimized by Oscar Toledo G. (nanochess), who also added save states.

The Intellivoice code has been contributed by Joe Zbiciak (author of jzintv), and adapted by Oscar Toledo G. (nanochess)

**Touchscreen Enhancement**: Android touchscreen / Windows mouse / Linux mouse keypad and utility button implementation (October 2025) by Jason Carr

<div style="margin: 18px 0;"><img width="1291" height="727" alt="FreeIntv Screenshot 1" src="screenshots/Screenshot 2025-11-24 194829.png" style="border:2px solid #FFD700; border-radius:6px;" /></div>
<div style="margin: 18px 0;"><img width="1372" height="770" alt="FreeIntv Screenshot 2" src="screenshots/Screenshot 2025-11-24 194901.png" style="border:2px solid #FFD700; border-radius:6px;" /></div>
<div style="margin: 18px 0;"><img width="1365" height="765" alt="FreeIntv Screenshot 3" src="screenshots/Screenshot 2025-11-24 194935.png" style="border:2px solid #FFD700; border-radius:6px;" /></div>

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

## Enabling the Dual-Screen Display

The FreeIntv core features an enhanced dual-screen layout option that shows the game on one side and the keypad UI on the other.

### To Enable the Feature:

1. **Launch a game** using the FreeIntv core
2. **Open the Quick Menu** (usually `Hotkey + X` on your controller, or through RetroArch's menu)
3. **Navigate to**: Options → Core Options (or "Display Overlay")
4. Set the option to **"Enabled"** or your preferred layout
5. **Close the menu** (setting takes effect immediately)
6. **Save Configuration** (Optional but recommended):
   - In the Quick Menu: **Overrides** → **Save Core Overrides** (applies to all FreeIntv games)
   - Or: **Save Game Overrides** (applies only to the current game)

The dual-screen display will now be active on your device!

## License
The FreeIntv core is licensed under GPLv2+. More information at https://github.com/libretro/FreeIntv/blob/master/LICENSE

## BIOS
FreeIntv requires two Intellivision BIOS files to be placed in the libretro 'system' folder:

| Function | Filename* | MD5 Hash |
| --- | --- | --- | 
| Executive ROM | `exec.bin`  | `62e761035cb657903761800f4437b8af` |
| Graphics ROM | `grom.bin` | `0cd5946c6473e42e8e4c2137785e427f` |

* BIOS filenames are case-sensitive

## Entertainment Computer System
FreeIntv does not currently support Entertainment Computer System (ECS) functionality. Contributions to the code are welcome!

## Game Overlays

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

**Usage:**
1. When a game launches, FreeIntv automatically looks for a matching overlay image in the `freeintv_overlays` folder
2. If found, the overlay is displayed on the keypad area (right side of screen, or left if swapped)
3. Touch or click directly on the overlay buttons to send input to the game

#### Touch and Mouse Input
- On Android, touch the keypad buttons directly on the right side of the screen (or left if swapped)
- On Windows and Linux, use the mouse to click on the keypad area. Mouse clicks are mapped to touch events, enabling full overlay UI functionality.

## Controls

### Traditional Gamepad Controls
* **Controller Swap** - Some Intellivision games expect the left controller to be player one, others expect the right controller. This isn't a problem if you have two controllers (and don't mind juggling them) but users with only one controller or using a portable setup would be effectively locked out of some games. Controller Swap swaps the two controller interfaces so that the player does not have to physically swap controllers.

### Android Touchscreen Controls *(New)*
* **Touchscreen Keypad** - Touch any of the 12 buttons (1-9, *, 0, #) on the right side of the screen to input commands directly
* **Visual Feedback** - Button presses show highlighting on keypad buttons

### RetroPad Mapping

| RetroPad | FreeIntv Function |
| --- | --- |
| D-Pad| 8-way movement |
| Left Analog Stick | 16-way disc |
| Right Analog Stick | 8-way keypad |
| L3 | Keypad 0 |
| R3 | Keypad 5 |
| L2 | Keypad Clear |
| R2 | Keypad Enter |
| A | Left Action Button |
| B | Right Action Button |
| Y | Top Action Button |
| X | Use the Last Selected Intellivision Keypad Button. In Astrosmash, for example, you can leave "3" selected to enable instant access to hyperspace. |
| L/R | Activate the Mini-Keypad |
| Start | Pause Game |
| Select | Controller Swap |
