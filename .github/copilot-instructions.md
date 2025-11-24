# FreeIntv with Android Touchscreen Support - AI Agent Instructions

## Project Overview

FreeIntv is a libretro emulation core for the Mattel Intellivision, recently enhanced with Android touchscreen support. The codebase is a C-based emulator that runs on the libretro framework, supporting multiple platforms (Linux, Windows, macOS, Nintendo Switch, PS Vita, etc.).

**Key Architecture**: Libretro plugin core → RetroArch frontend. The core handles CPU emulation, graphics, audio, controllers, and now dual-screen rendering with an integrated touchscreen keypad UI.

## Core Components & Data Flows

### 1. **Emulation Engine** (`src/intv.c`, `src/cp1610.c`, `src/stic.c`, `src/psg.c`)
- **CP1610**: 16-bit CPU emulator with cycle-accurate instruction execution
- **STIC**: Graphics chip that renders 352×224 pixels to frame buffer (`extern unsigned int frame[352*224]`)
- **PSG/Intellivoice**: Audio subsystem
- **Memory**: 16-bit address space (0x10000 addresses), with BIOS ROM, cartridge ROM, and RAM

**Pattern**: Each subsystem has serialization functions (`CP1610Serialize`, `STICSerialize`, etc.) for save states.

### 2. **Dual-Screen Rendering System** (`src/libretro.c: render_dual_screen()`)
The new touchscreen feature uses a horizontal layout:
```
[Game Screen (704×448)] [Keypad UI (370×600)]
└─ 2x scaled 352×224    └─ 12 buttons + indicators
[Utility Buttons (704×100)]
```
**Workspace**: 1074×600 pixels total. Display can swap positions via `display_swap` flag.

### 3. **Controller Mapping** (`src/controller.c`, `src/controller.h`)
- **RetroPad input** (from RetroArch joypad array) → Intellivision controller codes
- **Keypad codes**: 12 buttons (1-9, 0, *, #) with specific bit patterns (e.g., `K_1=0x81`, `K_5=0x42`)
- **Disc**: 16-way analog stick mapping for directional control
- **Action buttons**: Top/Left/Right buttons mapped via bit masks
- **Controller swap**: `controllerSwap` toggles left/right controller mapping (needed because different games expect different controller on player 1)

### 4. **Touchscreen Integration** (Android-specific in `src/libretro.c`)
- **Input**: RetroArch pointer callbacks → touches converted to coordinates
- **Hotspots**: Keypad and utility buttons defined as rectangles; touch within rectangle triggers action
- **Keypad hotspots**: 12 buttons at 70px spacing, positioned right of game screen
- **Utility buttons**: Menu, Quit, Swap Screen, Save/Load/Screenshot (partially implemented)
- **Button hold**: Touches kept active for 3 frames after release (`BUTTON_HOLD_FRAMES`)

## Developer Workflows

### Building
```bash
# Linux/Mac (standard libretro build)
make platform=unix

# Windows (MinGW)
make platform=win

# Android NDK
ndk-build -C jni/

# View all platform targets in Makefile
```

### Testing Workflow
1. Build core for platform
2. Load in RetroArch with appropriate BIOS files (`exec.bin`, `grom.bin`) in system folder
3. Launch Intellivision ROM to test controller input and rendering
4. On Android: touch keypad area to test pointer input handling

### Critical Libretro Callbacks (in `retro_run()`)
- `Run()` - Main emulation loop per frame
- `getControllerState()` - Convert RetroPad joypad array to Intellivision bits
- `setControllerInput()` - Write controller state to memory (addresses 0x1FE, 0x1FF)
- `render_dual_screen()` - Build composite frame buffer for display
- `Video(frame_buffer, width, height, pitch)` - Push frame to RetroArch

## Project-Specific Patterns & Conventions

### Memory-Mapped I/O
Controller input is written to specific memory addresses:
- `Memory[0x1FE + player]` = controller state (XORed with 0xFF for libretro compatibility)
- Reading/writing memory uses `readMem(adr)` and `writeMem(adr, val)`
- **Gotcha**: Controller values are inverted (XOR 0xFF), so button press = 0 bit set

### Keypad State Encoding
Keypad buttons use non-obvious bit patterns. **Never hardcode button values**—always reference constants in `controller.h`:
```c
#define K_1 0x81  // Keypad 1
#define K_5 0x42  // Keypad 5
#define K_C 0x88  // Keypad * (Clear)
#define K_E 0x28  // Keypad # (Enter)
```
Combine with bitwise OR: `state |= K_5 | D_N` for "keypad 5 + North direction"

### Libretro Integration Points
- **Core options**: Defined in `libretro_core_options.h`; read via `Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var)`
- **Video refresh**: Call `Video(frame_ptr, width, height, pitch)` at end of `retro_run()`
- **Audio sample**: Call `Audio(left_sample, right_sample)` for 16-bit PCM at 44.1kHz
- **Serialization**: Save/load state uses versioned struct (version 0x4f544702)

### Coordinate System for Touchscreen
- Origin (0,0) at top-left
- Utility buttons positioned below game screen: Y starts at 448
- Keypad positioned right of game screen: X starts at 704
- When `display_swap=true`, keypad moves to X=0 and game to X=370

### Frame Buffer Format
- Resolution: 352×224 pixels (native Intellivision)
- Format: `unsigned int frame[352*224]` (RGBA, 32-bit per pixel)
- Rendered 2x for display (704×448 after scaling)

## Critical Files for Common Tasks

| Task | Key Files |
|------|-----------|
| Add new controller input | `controller.c`, `controller.h` |
| Fix emulation bugs | `cp1610.c` (CPU), `stic.c` (graphics), `psg.c` (audio) |
| Implement UI features | `libretro.c` (rendering, touch handling) |
| Modify save states | `libretro.c` (serialization structs) |
| Change build system | `Makefile`, `Makefile.common` |
| Add platform support | `Makefile` (platform detection), `jni/` (Android) |

## Known Gotchas & Limitations

1. **ECS (Entertainment Computer System)**: Not currently supported; contributions welcome
2. **Button images**: Utility button graphics loading is partially implemented (placeholders); only "Swap Screen" button is fully functional
3. **Audio**: PSG audio works; Intellivoice support was added but needs testing with specific games
4. **Android NDK**: Uses Application.mk and Android.mk for build; requires NDK toolchain setup
5. **Serialization version**: Change 0x4f544702 if save state struct changes, or old saves will fail to load

## Build & Deploy

- **Libretro repository**: Changes should track with upstream libretro/FreeIntv
- **Core release**: Plugin compiled to `FreeIntv_libretro.{so,dll,dylib}` depending on platform
- **Android APK**: Built via RetroArch Android app; FreeIntv core loads as plugin

---

**Last Updated**: October 2025 | **Touchscreen Feature**: Added October 2025
