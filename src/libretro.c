/*
	This file is part of FreeIntv.

	FreeIntv is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeIntv is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with FreeIntv; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "libretro.h"
#include "libretro_core_options.h"
#include <file/file_path.h>
#include <retro_miscellaneous.h>

#include "intv.h"
#include "cp1610.h"
#include "memory.h"
#include "stic.h"
#include "psg.h"
#include "ivoice.h"
#include "controller.h"
#include "osd.h"

// Include stb_image header (implementation in stb_image_impl.c)
#include "stb_image.h"

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224

// ========================================
// HORIZONTAL LAYOUT DISPLAY CONFIGURATION
// ========================================
// Game Screen: Left side (704×448, 2x scaled from 352×224)
// Utility Buttons: Below game (704×100)
// Keypad: Right side (370×600)
// Total Workspace: 1074 × 600 pixels (keypad full height)

#define WORKSPACE_WIDTH 1074    // Game (704) + Keypad (370)
#define WORKSPACE_HEIGHT 600    // Keypad full height (600px)
#define GAME_SCREEN_WIDTH 704   // 352 * 2x
#define GAME_SCREEN_HEIGHT 448  // 224 * 2x
#define UTILITY_AREA_WIDTH 704  // Same as game width
#define UTILITY_AREA_HEIGHT 100 // Space for 6 buttons in 2 rows
#define KEYPAD_WIDTH 370        // Keypad overlay width
#define KEYPAD_HEIGHT 600       // Keypad overlay height
#define UTILITY_BUTTON_WIDTH 60
#define UTILITY_BUTTON_HEIGHT 50

// Keypad hotspot configuration
#define OVERLAY_HOTSPOT_COUNT 12
#define OVERLAY_HOTSPOT_SIZE 70

// RetroArch utility button codes
#define RETROARCH_MENU 1000
#define RETROARCH_PAUSE 1001
#define RETROARCH_REWIND 1002
#define RETROARCH_SAVE 1003
#define RETROARCH_LOAD 1004
#define RETROARCH_SWAP_OVERLAY 1005
#define RETROARCH_QUIT 1006
#define RETROARCH_RESET 1007
#define RETROARCH_SCREENSHOT 1008
#define RETROARCH_TOGGLE_DISPLAY 1009

#define UTILITY_BUTTON_COUNT 8  // Increased to 8 for fullscreen overlay toggle button (index 7)
#define MENU_BUTTON_WIDTH 200
#define MENU_BUTTON_HEIGHT 50

// Fullscreen mode constants
#define FULLSCREEN_STRIP_HEIGHT 50   // Height of bottom auto-hide strip in fullscreen
#define FULLSCREEN_HIDE_DELAY 300    // Frames before auto-hiding (300 frames @ 60fps = 5 seconds)
#define FULLSCREEN_TOUCH_ZONE 80     // Touch area from bottom to reveal strip (pixels)

// NOTE: Keypad codes are defined in controller.c - DO NOT redefine here!
// Using correct codes from controller.c:
// extern from controller.c: K_1=0x81, K_2=0x41, K_3=0x21, K_4=0x82, K_5=0x42, K_6=0x22
// K_7=0x84, K_8=0x44, K_9=0x24, K_0=0x48, K_C=0x88, K_E=0x28
// These are declared in controller.c and used by setControllerInput()

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* label;
    int command;  // RetroArch command code
} utility_button_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int id;
    int keypad_code;
} overlay_hotspot_t;

overlay_hotspot_t overlay_hotspots[OVERLAY_HOTSPOT_COUNT];

// Utility buttons positioned in 704×152 utility workspace (below game screen)
// Layout: Up to 7 buttons available, currently using 2 (indices 0 and 2)
// Available space: X=0-704 (704px = same as game width), Y=448-600 (152px height)
// Spacing calculation for even distribution:
// - Top gap: 17px (Y=448 to 465)
// - Button 2 (Toggle Layout): 50px (Y=465 to 515)
// - Middle gap: 17px (Y=515 to 532)
// - Button 0 (Fullscreen): 50px (Y=532 to 582)
// - Bottom gap: 18px (Y=582 to 600)
// - Toggle Layout (Button 2) on top row (Y=465)
// - Fullscreen (Button 0) on bottom row (Y=532)
// Note: Not all buttons are rendered/loaded - see render_dual_screen() for enabled buttons
static utility_button_t utility_buttons[UTILITY_BUTTON_COUNT] = {
    // Currently active buttons - CENTERED ON SEPARATE ROWS WITH EVEN SPACING
    {252, 532, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Fullscreen", 0},                                        // Button 0: Fullscreen Toggle (BOTTOM CENTER)
    {0, 0, 0, 0, "", 0},                                                                                        // Button 1: Empty (reserved)
    {252, 465, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT, "Toggle Layout", 0},                                       // Button 2: Toggle Layout (TOP CENTER)
    {0, 0, 0, 0, "", 0},                                                                                        // Button 3: Empty (reserved)
    {0, 0, 0, 0, "", 0},                                                                                        // Button 4: Empty (reserved)
    {0, 0, 0, 0, "", 0},                                                                                        // Button 5: Empty (reserved)
    {0, 0, 0, 0, "", 0},                                                                                        // Button 6: Empty (reserved)
    {0, 0, 0, 0, "", 0}                                                                                         // Button 7: Overlay Toggle (fullscreen only)
};

// Display system variables
static int dual_screen_enabled = 1;
static void* dual_screen_buffer = NULL;
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;
static int display_swap = 0;  // 0 = game left/keypad right, 1 = game right/keypad left

// Fullscreen mode variables
static int fullscreen_mode = 0;  // 0 = dual screen, 1 = fullscreen game only
static int fullscreen_strip_visible = 0;  // 1 = show bottom control strip, 0 = hidden
static int fullscreen_hide_timer = 0;  // Countdown timer for auto-hiding strip
static int fullscreen_bar_pinned = 0;  // 0 = auto-hide when not touching, 1 = always visible
static int overlay_visible_in_fullscreen = 0;  // 0 = hidden (default), 1 = show transparent overlay on game in fullscreen
static int fullscreen_overlay_on_left = 0;  // 0 = overlay on right (default), 1 = overlay on left
static int fullscreen_keypad_opacity = 40;  // Opacity percentage: 20, 40, 60, 80, 100

// Keypad scaling in fullscreen mode
static float keypad_scale_factor = 1.0f;  // Scale factor for keypad in fullscreen (1.0 = original 370x600 size)
static int scaled_keypad_width = 370;     // Scaled keypad width for fullscreen
static int scaled_keypad_height = 600;    // Scaled keypad height for fullscreen

// Utility button positions in fullscreen (for highlights)
static int button_x_pos[3] = {0};  // Positions of buttons 0, 1, 2 in fullscreen strip
static int button_y_pos = 0;       // Y position of utility buttons in fullscreen strip

// Hotspot input tracking
static int hotspot_pressed[OVERLAY_HOTSPOT_COUNT] = {0};  // Track which hotspots are currently pressed

// Utility button input tracking
static int utility_button_pressed[UTILITY_BUTTON_COUNT] = {0};  // Track which buttons are currently pressed

// PNG overlay system
static char current_rom_path[512] = {0};
static char system_dir[512] = {0};
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;

// Debug logging to file
static FILE* debug_log_file = NULL;
static void debug_log(const char* format, ...) {
    if (!debug_log_file) {
        // Try multiple paths
        const char* paths[] = {
            "/storage/emulated/0/Download/freeintv_debug.log",
            "/data/local/tmp/freeintv_debug.log",
            "/sdcard/freeintv_debug.log",
            "/storage/3861-3938/freeintv_debug.log"
        };
        for (int i = 0; i < 4; i++) {
            debug_log_file = fopen(paths[i], "a");
            if (debug_log_file) {
                fprintf(debug_log_file, "[LOG STARTED] Path: %s\n", paths[i]);
                fflush(debug_log_file);
                break;
            }
        }
    }
    if (debug_log_file) {
        va_list args;
        va_start(args, format);
        vfprintf(debug_log_file, format, args);
        fprintf(debug_log_file, "\n");
        fflush(debug_log_file);
        va_end(args);
    }
}
static int overlay_width = 370;
static int overlay_height = 600;

// Controller base
static unsigned int* controller_base = NULL;
static int controller_base_loaded = 0;
static int controller_base_width = 446;
static int controller_base_height = 620;

// Individual button images for utility buttons (6 separate PNGs)
// Top row: button_ra_menu.png, button_quit.png, button_toggle_layout.png
// Bottom row: button_save.png, button_load.png, button_screenshot.png
static struct {
    unsigned int* buffer;
    int loaded;
    int width;
    int height;
} utility_button_images[UTILITY_BUTTON_COUNT] = {
    {NULL, 0, 0, 0},  // Button 0: button_full_screen_toggle.png (Fullscreen)
    {NULL, 0, 0, 0},  // Button 1: Empty (reserved)
    {NULL, 0, 0, 0},  // Button 2: button_toggle_layout.png (Toggle Layout)
    {NULL, 0, 0, 0},  // Button 3: Empty (reserved)
    {NULL, 0, 0, 0},  // Button 4: Empty (reserved)
    {NULL, 0, 0, 0},  // Button 5: Empty (reserved)
    {NULL, 0, 0, 0},  // Button 6: Empty (reserved)
};

// Fullscreen overlay toggle button image (only loaded/used in fullscreen mode)
static struct {
    unsigned int* buffer;
    int loaded;
    int width;
    int height;
} button_fullscreen_overlay_image = {NULL, 0, 0, 0};

// Fullscreen exit button image (only loaded/used in fullscreen mode)
static struct {
    unsigned int* buffer;
    int loaded;
    int width;
    int height;
} button_fullscreen_exit_image = {NULL, 0, 0, 0};

static const char* button_filenames[UTILITY_BUTTON_COUNT] = {
    "button_full_screen_toggle.png",
    "",
    "button_toggle_layout.png",
    "",
    "",
    "",
    ""
};

// Fullscreen strip button image filename
static const char* button_fullscreen_overlay_filename = "button_full_screen_overlay.png";

// Fullscreen-specific button 0 (exit fullscreen) image
static const char* button_fullscreen_exit_filename = "button_multi_screen_toggle.png";

// Initialize overlay hotspots for keypad (positioned on RIGHT side)
// scale_factor: 1.0 = original size (370x600), < 1.0 = scaled down for fullscreen
static void init_overlay_hotspots(float scale_factor)
{
    printf("[INIT] Initializing overlay hotspots (scale=%.2f)...\n", scale_factor);
    fflush(stdout);
    
    // Layout: 4 rows x 3 columns, positioned on RIGHT side of workspace
    // Scale the hotspot sizes and gaps based on scale_factor
    int hotspot_w = (int)(OVERLAY_HOTSPOT_SIZE * scale_factor);
    int hotspot_h = (int)(OVERLAY_HOTSPOT_SIZE * scale_factor);
    int gap_x = (int)(28 * scale_factor);
    int gap_y = (int)(29 * scale_factor);
    int rows = 4;
    int cols = 3;
    
    // Scaled keypad and controller dimensions
    int scaled_keypad_w = (int)(KEYPAD_WIDTH * scale_factor);
    int scaled_keypad_h = (int)(KEYPAD_HEIGHT * scale_factor);
    int scaled_ctrl_base_w = (int)(controller_base_width * scale_factor);
    int scaled_ctrl_base_h = (int)(controller_base_height * scale_factor);
    
    // Calculate hotspots relative to keypad area (not absolute workspace)
    // This makes them independent of whether keypad is on left/right or scaled
    
    // IMPORTANT: Controller base is scaled proportionally
    // This creates the same proportional centering offset
    int ctrl_base_x_offset = (scaled_keypad_w - scaled_ctrl_base_w) / 2;
    
    // Center hotspots within the ACTUAL scaled controller base
    int hotspots_width = 3 * hotspot_w + 2 * gap_x;
    int hotspots_x_in_base = (scaled_ctrl_base_w - hotspots_width) / 2;
    int top_margin = (int)(183 * scale_factor);  // Scaled top margin
    
    // Positions are relative to the keypad area (0 = left edge of keypad)
    // NOT relative to GAME_SCREEN_WIDTH or workspace
    int start_x = ctrl_base_x_offset + hotspots_x_in_base;
    int start_y = top_margin;
    
    int keypad_map[12] = { K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_C, K_0, K_E };
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int idx = row * cols + col;
            overlay_hotspots[idx].x = start_x + col * (hotspot_w + gap_x);
            overlay_hotspots[idx].y = start_y + row * (hotspot_h + gap_y);
            overlay_hotspots[idx].width = hotspot_w;
            overlay_hotspots[idx].height = hotspot_h;
            overlay_hotspots[idx].id = idx + 1;
            overlay_hotspots[idx].keypad_code = keypad_map[idx];
            printf("[INIT] Hotspot %d: pos=(%d,%d), size=%dx%d, keypad_code=0x%02X (relative to keypad)\n",
                   idx, overlay_hotspots[idx].x, overlay_hotspots[idx].y,
                   overlay_hotspots[idx].width, overlay_hotspots[idx].height,
                   overlay_hotspots[idx].keypad_code);
        }
    }
    printf("[INIT] Hotspot initialization complete! (scale=%.2f)\n", scale_factor);
    fflush(stdout);
}

// Helper function to build system overlay path (handles both Windows \\ and Android / paths)
static void build_system_overlay_path(char* out_path, size_t out_size, const char* filename)
{
    if (!out_path || !system_dir[0] || !filename) {
        printf("[DEBUG] build_system_overlay_path: out_path=%p, system_dir_empty=%d, filename=%p\n", 
               out_path, !system_dir[0], filename);
        return;
    }
    
    // Detect path separator based on system_dir content
    char sep = strchr(system_dir, '/') ? '/' : '\\';
    
    // Check if system_dir already has trailing separator
    size_t dir_len = strlen(system_dir);
    char last_char = system_dir[dir_len - 1];
    int has_trailing_sep = (last_char == '/' || last_char == '\\');
    
    if (has_trailing_sep) {
        snprintf(out_path, out_size, "%sFreeIntv_image_assets%c%s", system_dir, sep, filename);
    } else {
        snprintf(out_path, out_size, "%s%cFreeIntv_image_assets%c%s", system_dir, sep, sep, filename);
    }
    
    printf("[DEBUG] Built overlay path: %s\n", out_path);
    printf("[DEBUG]   sep='%c' | trailing_sep=%d | filename=%s\n", sep, has_trailing_sep, filename);
    
    // Try to open file to verify it exists
    FILE* test = fopen(out_path, "rb");
    if (test) {
        fclose(test);
        printf("[DEBUG]   File status: EXISTS ✓\n");
    } else {
        printf("[DEBUG]   File status: NOT FOUND (errno=%d)\n", errno);
    }
}

// Forward declaration for build_overlay_path (ROM-specific overlay)
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size);

// Load controller base PNG
static void load_controller_base(void)
{
    if (controller_base_loaded || !system_dir[0]) {
        return;
    }
    
    char base_path[512];
    build_system_overlay_path(base_path, sizeof(base_path), "controller_base.png");
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(base_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        build_system_overlay_path(base_path, sizeof(base_path), "default.png");
        img_data = stbi_load(base_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[CONTROLLER] Loaded controller base: %dx%d\n", width, height);
        controller_base_width = width;
        controller_base_height = height;
        
        if (!controller_base) {
            controller_base = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        }
        
        if (controller_base) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    controller_base[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            controller_base_loaded = 1;
            stbi_image_free(img_data);
            printf("[CONTROLLER] Controller base loaded successfully\n");
        }
    }
}

// Load individual button images for utility buttons
static void load_utility_buttons(void)
{
    if (!system_dir[0]) {
        return;
    }
    
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        // ENABLED: Load button 0 (fullscreen toggle) and button 2 (toggle layout)
        // All other utility buttons are disabled
        if (i != 0 && i != 2) {
            // Skip loading for disabled buttons
            continue;
        }
        
        if (utility_button_images[i].loaded) {
            continue;  // Already loaded
        }
        
        char btn_path[512];
        build_system_overlay_path(btn_path, sizeof(btn_path), button_filenames[i]);
        
        int width, height, channels;
        unsigned char* img_data = stbi_load(btn_path, &width, &height, &channels, 4);
        
        if (img_data) {
            printf("[UTILITY_BUTTON] Loaded button %d (%s): %dx%d\n", i, button_filenames[i], width, height);
            utility_button_images[i].width = width;
            utility_button_images[i].height = height;
            
            if (!utility_button_images[i].buffer) {
                utility_button_images[i].buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
            }
            
            if (utility_button_images[i].buffer) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        unsigned char* pixel = img_data + (y * width + x) * 4;
                        unsigned int alpha = pixel[3];
                        unsigned int r = pixel[0];
                        unsigned int g = pixel[1];
                        unsigned int b = pixel[2];
                        utility_button_images[i].buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                    }
                }
                utility_button_images[i].loaded = 1;
                stbi_image_free(img_data);
                printf("[UTILITY_BUTTON] Button %d loaded successfully\n", i);
            }
        } else {
            printf("[UTILITY_BUTTON] Failed to load %s from %s\n", button_filenames[i], btn_path);
        }
    }
    
    // Load fullscreen overlay toggle button image
    if (!button_fullscreen_overlay_image.loaded) {
        char overlay_btn_path[512];
        build_system_overlay_path(overlay_btn_path, sizeof(overlay_btn_path), button_fullscreen_overlay_filename);

        
        int width, height, channels;
        unsigned char* img_data = stbi_load(overlay_btn_path, &width, &height, &channels, 4);
        
        if (img_data) {
            printf("[FULLSCREEN_OVERLAY_BUTTON] Loaded fullscreen overlay button: %dx%d\n", width, height);
            button_fullscreen_overlay_image.width = width;
            button_fullscreen_overlay_image.height = height;
            
            if (!button_fullscreen_overlay_image.buffer) {
                button_fullscreen_overlay_image.buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
            }
            
            if (button_fullscreen_overlay_image.buffer) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        unsigned char* pixel = img_data + (y * width + x) * 4;
                        unsigned int alpha = pixel[3];
                        unsigned int r = pixel[0];
                        unsigned int g = pixel[1];
                        unsigned int b = pixel[2];
                        button_fullscreen_overlay_image.buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                    }
                }
                button_fullscreen_overlay_image.loaded = 1;
                stbi_image_free(img_data);
                printf("[FULLSCREEN_OVERLAY_BUTTON] Fullscreen overlay button loaded successfully\n");
            }
        } else {
            printf("[FULLSCREEN_OVERLAY_BUTTON] Failed to load fullscreen_overlay.png from %s\n", overlay_btn_path);
        }
    }
    
    // Load fullscreen exit button image
    if (!button_fullscreen_exit_image.loaded) {
        char exit_btn_path[512];
        build_system_overlay_path(exit_btn_path, sizeof(exit_btn_path), button_fullscreen_exit_filename);

        
        int width, height, channels;
        unsigned char* img_data = stbi_load(exit_btn_path, &width, &height, &channels, 4);
        
        if (img_data) {
            printf("[FULLSCREEN_EXIT_BUTTON] Loaded fullscreen exit button: %dx%d\n", width, height);
            button_fullscreen_exit_image.width = width;
            button_fullscreen_exit_image.height = height;
            
            if (!button_fullscreen_exit_image.buffer) {
                button_fullscreen_exit_image.buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
            }
            
            if (button_fullscreen_exit_image.buffer) {
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        unsigned char* pixel = img_data + (y * width + x) * 4;
                        unsigned int alpha = pixel[3];
                        unsigned int r = pixel[0];
                        unsigned int g = pixel[1];
                        unsigned int b = pixel[2];
                        button_fullscreen_exit_image.buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                    }
                }
                button_fullscreen_exit_image.loaded = 1;
                stbi_image_free(img_data);
                printf("[FULLSCREEN_EXIT_BUTTON] Fullscreen exit button loaded successfully\n");
            }
        } else {
            printf("[FULLSCREEN_EXIT_BUTTON] Failed to load button_multi_screen_toggle.png from %s\n", exit_btn_path);
        }
    }
}

// Cleanup utility button images
static void cleanup_utility_buttons(void)
{
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        if (utility_button_images[i].buffer) {
            free(utility_button_images[i].buffer);
            utility_button_images[i].buffer = NULL;
        }
        utility_button_images[i].loaded = 0;
        utility_button_images[i].width = 0;
        utility_button_images[i].height = 0;
    }
    
    // Cleanup fullscreen overlay button image
    if (button_fullscreen_overlay_image.buffer) {
        free(button_fullscreen_overlay_image.buffer);
        button_fullscreen_overlay_image.buffer = NULL;
    }
    button_fullscreen_overlay_image.loaded = 0;
    button_fullscreen_overlay_image.width = 0;
    button_fullscreen_overlay_image.height = 0;
}

// Build overlay path from ROM name
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size)
{
    if (!rom_path || !overlay_path || overlay_path_size == 0 || system_dir[0] == '\0') {
        overlay_path[0] = '\0';
        return;
    }
    
    const char* filename = rom_path;
    const char* p = rom_path;
    while (*p) {
        if (*p == '\\' || *p == '/') {
            filename = p + 1;
        }
        p++;
    }
    
    const char* ext = filename;
    const char* q = filename;
    while (*q) {
        if (*q == '.') {
            ext = q;
        }
        q++;
    }
    size_t name_len = ext - filename;
    
    // Detect path separator based on system_dir content
    char sep = strchr(system_dir, '/') ? '/' : '\\';
    snprintf(overlay_path, overlay_path_size, 
             "%s%cFreeIntv_image_assets%coverlays%c%.*s.png",
             system_dir, sep, sep, sep, (int)name_len, filename);
    printf("[DEBUG] ROM overlay path (sep=%c): %s\n", sep, overlay_path);
}

// Load overlay for ROM
static void load_overlay_for_rom(const char* rom_path)
{
    if (!rom_path || !dual_screen_enabled) return;
    
    char overlay_path[1024];
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path));
    
    overlay_loaded = 0;
    
    if (overlay_buffer) {
        free(overlay_buffer);
        overlay_buffer = NULL;
    }
    
    int width, height, channels;
    unsigned char* img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
    
    if (!img_data) {
        char jpg_path[1024];
        strncpy(jpg_path, overlay_path, sizeof(jpg_path) - 1);
        char* ext = strrchr(jpg_path, '.');
        if (ext) {
            strcpy(ext, ".jpg");
            img_data = stbi_load(jpg_path, &width, &height, &channels, 4);
        }
    }
    
    if (!img_data && system_dir[0] != '\0') {
        char default_path[1024];
        build_system_overlay_path(default_path, sizeof(default_path), "default.png");
        img_data = stbi_load(default_path, &width, &height, &channels, 4);
    }
    
    if (img_data) {
        printf("[OVERLAY] Loaded overlay: %dx%d\n", width, height);
        overlay_width = width;
        overlay_height = height;
        overlay_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    overlay_buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            init_overlay_hotspots(1.0f);  // Initialize with normal scale (1.0 = full size)
        }
        stbi_image_free(img_data);
    } else {
        overlay_width = 370;
        overlay_height = 600;
        overlay_buffer = (unsigned int*)malloc(overlay_width * overlay_height * sizeof(unsigned int));
        if (overlay_buffer) {
            for (int y = 0; y < overlay_height; y++) {
                for (int x = 0; x < overlay_width; x++) {
                    if (y < overlay_height / 2 && x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF0000FF;
                    else if (y < overlay_height / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFF00FF00;
                    else if (x < overlay_width / 2)
                        overlay_buffer[y * overlay_width + x] = 0xFFFF0000;
                    else
                        overlay_buffer[y * overlay_width + x] = 0xFFFFFFFF;
                }
            }
        }
    }
    
    overlay_loaded = 1;
    strncpy(current_rom_path, rom_path, sizeof(current_rom_path) - 1);
}


// Render display with game screen LEFT and keypad RIGHT (or fullscreen with auto-hide strip)
static void render_dual_screen(void)
{
    if (!dual_screen_enabled) return;
    
    if (!dual_screen_buffer) {
        dual_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
    }
    if (!dual_screen_buffer) return;
    
    unsigned int* dual_buffer = (unsigned int*)dual_screen_buffer;
    extern unsigned int frame[352 * 224];
    
    // Clear entire workspace with black
    for (int i = 0; i < WORKSPACE_WIDTH * WORKSPACE_HEIGHT; i++) {
        dual_buffer[i] = 0xFF000000;
    }
    
    // =========================================
    // FULLSCREEN MODE
    // =========================================
    if (fullscreen_mode)
    {
        // In fullscreen mode, game takes up entire 1074×600 workspace
        // Render scaled game to fill the screen
        int game_height = WORKSPACE_HEIGHT;
        if (fullscreen_strip_visible) {
            // Leave room for bottom control strip
            game_height = WORKSPACE_HEIGHT - FULLSCREEN_STRIP_HEIGHT;
        }
        
        // Scale game to fit available height, maintaining 352:224 aspect ratio
        // Calculate scaling factor to fill width while maintaining aspect
        double scale_x = WORKSPACE_WIDTH / (double)GAME_WIDTH;
        double scale_y = game_height / (double)GAME_HEIGHT;
        double scale = (scale_x < scale_y) ? scale_x : scale_y;  // Use smaller to fit
        
        int scaled_width = (int)(GAME_WIDTH * scale);
        int scaled_height = (int)(GAME_HEIGHT * scale);
        int offset_x = (WORKSPACE_WIDTH - scaled_width) / 2;  // Center horizontally
        int offset_y = 0;  // Align to top
        
        // Render scaled game centered in fullscreen
        for (int y = 0; y < scaled_height && y < game_height; ++y) {
            int src_y = (int)(y / scale);
            if (src_y >= GAME_HEIGHT) src_y = GAME_HEIGHT - 1;
            
            for (int x = 0; x < scaled_width; ++x) {
                int src_x = (int)(x / scale);
                if (src_x >= GAME_WIDTH) src_x = GAME_WIDTH - 1;
                
                int workspace_x = offset_x + x;
                int workspace_y = offset_y + y;
                
                if (workspace_x >= 0 && workspace_x < WORKSPACE_WIDTH && 
                    workspace_y >= 0 && workspace_y < WORKSPACE_HEIGHT) {
                    dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 
                        frame[src_y * GAME_WIDTH + src_x];
                }
            }
        }
        
        // Calculate keypad scale factor for fullscreen mode
        // When utility bar is visible, scale keypad down to fit in remaining vertical space
        // When utility bar is hidden, keypad stays at full 600px height
        float keypad_scale = 1.0f;  // Default to full size
        int keypad_y_offset = 0;    // Default to top
        
        if (fullscreen_strip_visible) {
            // Utility bar is visible - scale keypad to fit in remaining space above the bar
            // Available height for keypad = game_height (which is WORKSPACE_HEIGHT - FULLSCREEN_STRIP_HEIGHT)
            // Keypad should scale to fit this height while maintaining 370:600 aspect ratio
            float scale_to_fit_height = (float)game_height / KEYPAD_HEIGHT;
            keypad_scale = scale_to_fit_height;
            keypad_y_offset = 0;  // Keypad at top, game scales down and sits above utility bar
        }
        // else: keypad_scale stays 1.0f and keypad_y_offset stays 0 (full size, full height)
        
        // Store scale factor and scaled dimensions for touch input processing
        keypad_scale_factor = keypad_scale;
        scaled_keypad_width = (int)(KEYPAD_WIDTH * keypad_scale);
        scaled_keypad_height = (int)(KEYPAD_HEIGHT * keypad_scale);
        
        // Calculate keypad x position in fullscreen (left or right depending on fullscreen_overlay_on_left)
        int keypad_x_workspace = fullscreen_overlay_on_left ? 0 : (WORKSPACE_WIDTH - scaled_keypad_width);
        
        // Re-initialize hotspots with new scale factor
        init_overlay_hotspots(keypad_scale);
        
        // Store keypad y offset for rendering (used below)
        int kp_y_offset = keypad_y_offset;
        int kp_x_offset = keypad_x_workspace;
        
        // Draw transparent overlay on right side in fullscreen mode (same as dual-screen layout)
        if (overlay_visible_in_fullscreen && overlay_buffer && overlay_width > 0 && overlay_height > 0)
        {
            // Scale overlay and controller base based on keypad scale
            int scaled_overlay_width = (int)(overlay_width * keypad_scale);
            int scaled_overlay_height = (int)(overlay_height * keypad_scale);
            int scaled_ctrl_base_width = (int)(controller_base_width * keypad_scale);
            int scaled_ctrl_base_height = (int)(controller_base_height * keypad_scale);
            
            // First, render game-specific overlay with 40% transparency (drawn first, so it's underneath)
            // Draw overlay at its position, centered within the scaled keypad area
            int overlay_x_offset = (scaled_keypad_width - scaled_overlay_width) / 2;
            int overlay_x_workspace = kp_x_offset + overlay_x_offset;
            int overlay_y_offset = (scaled_keypad_height - scaled_overlay_height) / 2;
            
            // Render scaled overlay with 40% transparency using bilinear scaling
            for (int y = 0; y < scaled_overlay_height && (kp_y_offset + overlay_y_offset + y) < WORKSPACE_HEIGHT; ++y) {
                int src_y = (int)(y / keypad_scale);
                if (src_y >= overlay_height) src_y = overlay_height - 1;
                
                for (int x = 0; x < scaled_overlay_width; ++x) {
                    int src_x = (int)(x / keypad_scale);
                    if (src_x >= overlay_width) src_x = overlay_width - 1;
                    
                    int workspace_x = overlay_x_workspace + x;
                    int workspace_y = kp_y_offset + overlay_y_offset + y;
                    
                    if (workspace_x >= 0 && workspace_x < WORKSPACE_WIDTH && 
                        workspace_y >= 0 && workspace_y < WORKSPACE_HEIGHT) {
                        
                        unsigned int overlay_pixel = overlay_buffer[src_y * overlay_width + src_x];
                        unsigned int overlay_alpha = (overlay_pixel >> 24) & 0xFF;
                        
                        if (overlay_alpha > 0) {
                            // Apply configurable opacity to overlay
                            unsigned int final_alpha = (overlay_alpha * fullscreen_keypad_opacity) / 100;
                            unsigned int inv_alpha = 255 - final_alpha;
                            
                            unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                            
                            unsigned int overlay_r = (overlay_pixel >> 16) & 0xFF;
                            unsigned int overlay_g = (overlay_pixel >> 8) & 0xFF;
                            unsigned int overlay_b = overlay_pixel & 0xFF;
                            
                            unsigned int existing_r = (existing >> 16) & 0xFF;
                            unsigned int existing_g = (existing >> 8) & 0xFF;
                            unsigned int existing_b = existing & 0xFF;
                            
                            unsigned int blended_r = (overlay_r * final_alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (overlay_g * final_alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (overlay_b * final_alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
            
            // Then, render controller_base template on top (drawn last, so it's on top)
            if (controller_base_loaded && controller_base && controller_base_width > 0 && controller_base_height > 0)
            {
                // Draw controller_base at its position, centered within the scaled keypad area
                int ctrl_base_x_offset = (scaled_keypad_width - scaled_ctrl_base_width) / 2;
                int ctrl_base_x_workspace = kp_x_offset + ctrl_base_x_offset;
                int ctrl_base_y_offset = (scaled_keypad_height - scaled_ctrl_base_height) / 2;
                
                // Render scaled controller base with 40% transparency
                for (int y = 0; y < scaled_ctrl_base_height && (kp_y_offset + ctrl_base_y_offset + y) < WORKSPACE_HEIGHT; ++y) {
                    int src_y = (int)(y / keypad_scale);
                    if (src_y >= controller_base_height) src_y = controller_base_height - 1;
                    
                    for (int x = 0; x < scaled_ctrl_base_width; ++x) {
                        int src_x = (int)(x / keypad_scale);
                        if (src_x >= controller_base_width) src_x = controller_base_width - 1;
                        
                        int workspace_x = ctrl_base_x_workspace + x;
                        int workspace_y = kp_y_offset + ctrl_base_y_offset + y;
                        
                        if (workspace_x >= 0 && workspace_x < WORKSPACE_WIDTH && 
                            workspace_y >= 0 && workspace_y < WORKSPACE_HEIGHT) {
                            
                            unsigned int base_pixel = controller_base[src_y * controller_base_width + src_x];
                            unsigned int base_alpha = (base_pixel >> 24) & 0xFF;
                            
                            if (base_alpha > 0) {
                                // Apply configurable opacity to controller base
                                unsigned int final_alpha = (base_alpha * fullscreen_keypad_opacity) / 100;
                                unsigned int inv_alpha = 255 - final_alpha;
                                
                                unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                                
                                unsigned int base_r = (base_pixel >> 16) & 0xFF;
                                unsigned int base_g = (base_pixel >> 8) & 0xFF;
                                unsigned int base_b = base_pixel & 0xFF;
                                
                                unsigned int existing_r = (existing >> 16) & 0xFF;
                                unsigned int existing_g = (existing >> 8) & 0xFF;
                                unsigned int existing_b = existing & 0xFF;
                                
                                unsigned int blended_r = (base_r * final_alpha + existing_r * inv_alpha) / 255;
                                unsigned int blended_g = (base_g * final_alpha + existing_g * inv_alpha) / 255;
                                unsigned int blended_b = (base_b * final_alpha + existing_b * inv_alpha) / 255;
                                
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                    (blended_r << 16) | (blended_g << 8) | blended_b;
                            }
                        }
                    }
                }
            }
        }
        
        // Draw auto-hide strip at bottom if visible
        if (fullscreen_strip_visible)
        {
            int strip_y = WORKSPACE_HEIGHT - FULLSCREEN_STRIP_HEIGHT;
            
            // Background for strip (dark semi-transparent bar)
            unsigned int strip_bg = 0xDD1a1a1a;
            for (int y = strip_y; y < WORKSPACE_HEIGHT; ++y) {
                for (int x = 0; x < WORKSPACE_WIDTH; ++x) {
                    dual_buffer[y * WORKSPACE_WIDTH + x] = strip_bg;
                }
            }
            
            // === PIN BUTTON on far left ===
            // Small circle button to toggle auto-hide behavior
            int pin_radius = 8;  // Small circle
            int pin_x = pin_radius + 10;  // 10 pixels from left edge
            int pin_y = strip_y + (FULLSCREEN_STRIP_HEIGHT / 2);  // Centered vertically in strip
            unsigned int pin_outline_color = 0xFFFFFFFF;  // White outline
            unsigned int pin_fill_color = 0xFF00FF00;    // Green fill when pinned
            
            // Draw circle outline
            for (int y = pin_y - pin_radius; y <= pin_y + pin_radius; y++) {
                for (int x = pin_x - pin_radius; x <= pin_x + pin_radius; x++) {
                    if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                        int dx = x - pin_x;
                        int dy = y - pin_y;
                        int dist_sq = dx * dx + dy * dy;
                        
                        // Draw outline (within 1 pixel of circle edge)
                        if (dist_sq <= (pin_radius + 1) * (pin_radius + 1) && 
                            dist_sq >= (pin_radius - 1) * (pin_radius - 1)) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = pin_outline_color;
                        }
                        // Fill circle if pinned
                        else if (fullscreen_bar_pinned && dist_sq <= pin_radius * pin_radius) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = pin_fill_color;
                        }
                    }
                }
            }
            
            // === OPACITY DOTS on far right ===
            // 5 small circles for opacity adjustment: 20%, 40%, 60%, 80%, 100%
            int opacity_values[5] = {20, 40, 60, 80, 100};
            int dot_radius = 5;
            int dot_spacing = 14;  // Space between dots
            int dots_start_x = WORKSPACE_WIDTH - (5 * dot_spacing) - 10;  // 10 pixels from right edge
            int dots_y = strip_y + (FULLSCREEN_STRIP_HEIGHT / 2);
            
            for (int i = 0; i < 5; i++) {
                int dot_x = dots_start_x + (i * dot_spacing);
                unsigned int dot_outline_color = 0xFFFFFFFF;  // White outline
                unsigned int dot_fill_color = 0xFF00AAFF;    // Blue fill for selected
                
                // Check if this is the current opacity level
                int is_selected = (fullscreen_keypad_opacity == opacity_values[i]);
                
                // Draw dot outline and fill
                for (int y = dots_y - dot_radius; y <= dots_y + dot_radius; y++) {
                    for (int x = dot_x - dot_radius; x <= dot_x + dot_radius; x++) {
                        if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                            int dx = x - dot_x;
                            int dy = y - dots_y;
                            int dist_sq = dx * dx + dy * dy;
                            
                            // Draw outline
                            if (dist_sq <= (dot_radius + 1) * (dot_radius + 1) && 
                                dist_sq >= (dot_radius - 1) * (dot_radius - 1)) {
                                dual_buffer[y * WORKSPACE_WIDTH + x] = dot_outline_color;
                            }
                            // Fill dot if selected
                            else if (is_selected && dist_sq <= dot_radius * dot_radius) {
                                dual_buffer[y * WORKSPACE_WIDTH + x] = dot_fill_color;
                            }
                        }
                    }
                }
            }
            
            // Draw utility buttons in the strip
            // Show 3 buttons in fullscreen mode: Exit (Button 0), Show Overlay (Button 7), and Toggle Layout (Button 2)
            // Position them at: 1/4, 1/2, and 3/4 of workspace width
            button_y_pos = strip_y + (FULLSCREEN_STRIP_HEIGHT - MENU_BUTTON_HEIGHT) / 2;
            int button_y = button_y_pos;  // Local reference to static variable for convenience
            
            // Button positions for fullscreen strip (3 buttons evenly spaced)
            button_x_pos[0] = (WORKSPACE_WIDTH / 4) - (MENU_BUTTON_WIDTH / 2);        // 1/4 - Exit
            button_x_pos[1] = (WORKSPACE_WIDTH / 2) - (MENU_BUTTON_WIDTH / 2);        // 1/2 - Toggle Layout
            button_x_pos[2] = (3 * WORKSPACE_WIDTH / 4) - (MENU_BUTTON_WIDTH / 2);    // 3/4 - Show Overlay
            
            // Button 0: Exit fullscreen
            int button_x = button_x_pos[0];
            
            // Try to render button 0 PNG image if loaded (use fullscreen-specific image in fullscreen mode)
            if (button_fullscreen_exit_image.loaded && button_fullscreen_exit_image.buffer) {
                int img_width = button_fullscreen_exit_image.width;
                int img_height = button_fullscreen_exit_image.height;
                
                // Blit button image to fullscreen strip
                for (int img_y = 0; img_y < img_height; img_y++) {
                    for (int img_x = 0; img_x < img_width; img_x++) {
                        int workspace_x = button_x + img_x;
                        int workspace_y = button_y + img_y;
                        
                        if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                        if (workspace_x < 0) continue;
                        
                        unsigned int button_pixel = button_fullscreen_exit_image.buffer[img_y * img_width + img_x];
                        unsigned int alpha = (button_pixel >> 24) & 0xFF;
                        
                        if (alpha > 0) {
                            // Blend with alpha
                            if (alpha == 255) {
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                            } else {
                                // Alpha blend
                                unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                                unsigned int inv_alpha = 255 - alpha;
                                
                                unsigned int r = ((button_pixel >> 16) & 0xFF);
                                unsigned int g = ((button_pixel >> 8) & 0xFF);
                                unsigned int b = (button_pixel & 0xFF);
                                
                                unsigned int existing_r = ((existing >> 16) & 0xFF);
                                unsigned int existing_g = ((existing >> 8) & 0xFF);
                                unsigned int existing_b = (existing & 0xFF);
                                
                                unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                                unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                                unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                                
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                    (blended_r << 16) | (blended_g << 8) | blended_b;
                            }
                        }
                    }
                }
            } else {
                // Fallback: Draw placeholder button rectangle (gold color) if PNG not loaded
                unsigned int button_color = 0xFFFFD700;
                for (int y = button_y; y < button_y + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = button_x; x < button_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = button_color;
                        }
                    }
                }
            }
            
            // Button 1: Show/Hide overlay in fullscreen
            button_x = button_x_pos[1];
            
            // Try to render fullscreen overlay toggle button PNG image if loaded
            if (button_fullscreen_overlay_image.loaded && button_fullscreen_overlay_image.buffer) {
                int img_width = button_fullscreen_overlay_image.width;
                int img_height = button_fullscreen_overlay_image.height;
                
                // Blit button image to fullscreen strip
                for (int img_y = 0; img_y < img_height; img_y++) {
                    for (int img_x = 0; img_x < img_width; img_x++) {
                        int workspace_x = button_x + img_x;
                        int workspace_y = button_y + img_y;
                        
                        if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                        if (workspace_x < 0) continue;
                        
                        unsigned int button_pixel = button_fullscreen_overlay_image.buffer[img_y * img_width + img_x];
                        unsigned int alpha = (button_pixel >> 24) & 0xFF;
                        
                        if (alpha > 0) {
                            // Blend with alpha
                            if (alpha == 255) {
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                            } else {
                                // Alpha blend
                                unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                                unsigned int inv_alpha = 255 - alpha;
                                
                                unsigned int r = ((button_pixel >> 16) & 0xFF);
                                unsigned int g = ((button_pixel >> 8) & 0xFF);
                                unsigned int b = (button_pixel & 0xFF);
                                
                                unsigned int existing_r = ((existing >> 16) & 0xFF);
                                unsigned int existing_g = ((existing >> 8) & 0xFF);
                                unsigned int existing_b = (existing & 0xFF);
                                
                                unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                                unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                                unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                                
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                    (blended_r << 16) | (blended_g << 8) | blended_b;
                            }
                        }
                    }
                }
            } else {
                // Fallback: Draw placeholder button rectangle (gold color)
                unsigned int button_color = 0xFFFFD700;
                for (int y = button_y; y < button_y + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = button_x; x < button_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = button_color;
                        }
                    }
                }
            }
            
            // Button 2: Toggle layout overlay left/right in fullscreen
            button_x = button_x_pos[1];
            
            // Try to render button 2 PNG image if loaded
            if (utility_button_images[2].loaded && utility_button_images[2].buffer) {
                int img_width = utility_button_images[2].width;
                int img_height = utility_button_images[2].height;
                
                // Blit button image to fullscreen strip
                for (int img_y = 0; img_y < img_height; img_y++) {
                    for (int img_x = 0; img_x < img_width; img_x++) {
                        int workspace_x = button_x + img_x;
                        int workspace_y = button_y + img_y;
                        
                        if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                        if (workspace_x < 0) continue;
                        
                        unsigned int button_pixel = utility_button_images[2].buffer[img_y * img_width + img_x];
                        unsigned int alpha = (button_pixel >> 24) & 0xFF;
                        
                        if (alpha > 0) {
                            // Blend with alpha
                            if (alpha == 255) {
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                            } else {
                                // Alpha blend
                                unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                                unsigned int inv_alpha = 255 - alpha;
                                
                                unsigned int r = ((button_pixel >> 16) & 0xFF);
                                unsigned int g = ((button_pixel >> 8) & 0xFF);
                                unsigned int b = (button_pixel & 0xFF);
                                
                                unsigned int existing_r = ((existing >> 16) & 0xFF);
                                unsigned int existing_g = ((existing >> 8) & 0xFF);
                                unsigned int existing_b = (existing & 0xFF);
                                
                                unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                                unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                                unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                                
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                    (blended_r << 16) | (blended_g << 8) | blended_b;
                            }
                        }
                    }
                }
            } else {
                // Fallback: Draw placeholder button rectangle (gold color)
                unsigned int button_color = 0xFFFFD700;
                for (int y = button_y; y < button_y + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = button_x; x < button_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = button_color;
                        }
                    }
                }
            }
            
            // Button 7: Show/Hide overlay in fullscreen
            button_x = button_x_pos[2];
            
            // Try to render fullscreen overlay toggle button PNG image if loaded
            if (button_fullscreen_overlay_image.loaded && button_fullscreen_overlay_image.buffer) {
                int img_width = button_fullscreen_overlay_image.width;
                int img_height = button_fullscreen_overlay_image.height;
                
                // Blit button image to fullscreen strip
                for (int img_y = 0; img_y < img_height; img_y++) {
                    for (int img_x = 0; img_x < img_width; img_x++) {
                        int workspace_x = button_x + img_x;
                        int workspace_y = button_y + img_y;
                        
                        if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                        if (workspace_x < 0) continue;
                        
                        unsigned int button_pixel = button_fullscreen_overlay_image.buffer[img_y * img_width + img_x];
                        unsigned int alpha = (button_pixel >> 24) & 0xFF;
                        
                        if (alpha > 0) {
                            // Blend with alpha
                            if (alpha == 255) {
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                            } else {
                                // Alpha blend
                                unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                                unsigned int inv_alpha = 255 - alpha;
                                
                                unsigned int r = ((button_pixel >> 16) & 0xFF);
                                unsigned int g = ((button_pixel >> 8) & 0xFF);
                                unsigned int b = (button_pixel & 0xFF);
                                
                                unsigned int existing_r = ((existing >> 16) & 0xFF);
                                unsigned int existing_g = ((existing >> 8) & 0xFF);
                                unsigned int existing_b = (existing & 0xFF);
                                
                                unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                                unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                                unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                                
                                dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | 
                                    (blended_r << 16) | (blended_g << 8) | blended_b;
                            }
                        }
                    }
                }
            } else {
                // Fallback: Draw placeholder button rectangle (gold color)
                unsigned int button_color = 0xFFFFD700;
                for (int y = button_y; y < button_y + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = button_x; x < button_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            dual_buffer[y * WORKSPACE_WIDTH + x] = button_color;
                        }
                    }
                }
            }
            
            // Highlight if pressed
            if (utility_button_pressed[0]) {
                unsigned int highlight_color = 0x88FFFF00;
                int btn_x = button_x_pos[0];
                for (int y = button_y_pos; y < button_y_pos + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = btn_x; x < btn_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                            unsigned int alpha = (highlight_color >> 24) & 0xFF;
                            unsigned int inv_alpha = 255 - alpha;
                            
                            unsigned int r = ((highlight_color >> 16) & 0xFF);
                            unsigned int g = ((highlight_color >> 8) & 0xFF);
                            unsigned int b = (highlight_color & 0xFF);
                            
                            unsigned int existing_r = ((existing >> 16) & 0xFF);
                            unsigned int existing_g = ((existing >> 8) & 0xFF);
                            unsigned int existing_b = (existing & 0xFF);
                            
                            unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | 
                                (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
            
            // Highlight toggle layout button if pressed
            if (utility_button_pressed[2]) {
                unsigned int highlight_color = 0x88FFFF00;
                int btn_x = button_x_pos[1];
                for (int y = button_y_pos; y < button_y_pos + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = btn_x; x < btn_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                            unsigned int alpha = (highlight_color >> 24) & 0xFF;
                            unsigned int inv_alpha = 255 - alpha;
                            
                            unsigned int r = ((highlight_color >> 16) & 0xFF);
                            unsigned int g = ((highlight_color >> 8) & 0xFF);
                            unsigned int b = (highlight_color & 0xFF);
                            
                            unsigned int existing_r = ((existing >> 16) & 0xFF);
                            unsigned int existing_g = ((existing >> 8) & 0xFF);
                            unsigned int existing_b = (existing & 0xFF);
                            
                            unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | 
                                (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
            
            // Highlight overlay toggle button if pressed
            if (utility_button_pressed[7]) {
                unsigned int highlight_color = 0x88FFFF00;
                int btn_x = button_x_pos[2];
                for (int y = button_y_pos; y < button_y_pos + MENU_BUTTON_HEIGHT; ++y) {
                    if (y >= WORKSPACE_HEIGHT) break;
                    for (int x = btn_x; x < btn_x + MENU_BUTTON_WIDTH; ++x) {
                        if (x >= 0 && x < WORKSPACE_WIDTH) {
                            unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                            unsigned int alpha = (highlight_color >> 24) & 0xFF;
                            unsigned int inv_alpha = 255 - alpha;
                            
                            unsigned int r = ((highlight_color >> 16) & 0xFF);
                            unsigned int g = ((highlight_color >> 8) & 0xFF);
                            unsigned int b = (highlight_color & 0xFF);
                            
                            unsigned int existing_r = ((existing >> 16) & 0xFF);
                            unsigned int existing_g = ((existing >> 8) & 0xFF);
                            unsigned int existing_b = (existing & 0xFF);
                            
                            unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | 
                                (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
            
            // Draw border around strip
            unsigned int border_color = 0xFFc7a814;
            for (int x = 0; x < WORKSPACE_WIDTH; ++x) {
                dual_buffer[strip_y * WORKSPACE_WIDTH + x] = border_color;
                dual_buffer[(WORKSPACE_HEIGHT - 1) * WORKSPACE_WIDTH + x] = border_color;
            }

        }
        
        // === HOTSPOT HIGHLIGHTING IN FULLSCREEN - Show which buttons are pressed by touch ===
        // Highlight all pressed hotspots (from touch input detection)
        for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
            if (hotspot_pressed[i]) {
                overlay_hotspot_t *h = &overlay_hotspots[i];
                unsigned int highlight_color = 0x6600FF00;  // Green highlight for touch-pressed, 40% transparency to match keypad overlay
                
                // Calculate absolute workspace coordinates for this hotspot
                int abs_hotspot_x, abs_hotspot_y;
                int hotspot_width = h->width;
                int hotspot_height = h->height;
                
                // Fullscreen mode: hotspots relative to scaled keypad
                int scaled_keypad_w = (int)(KEYPAD_WIDTH * keypad_scale_factor);
                int keypad_x_workspace = fullscreen_overlay_on_left ? 0 : (WORKSPACE_WIDTH - scaled_keypad_w);
                abs_hotspot_x = keypad_x_workspace + h->x;
                abs_hotspot_y = 0 + h->y;  // In fullscreen, keypad Y starts at 0 (no offset)
                
                for (int y = abs_hotspot_y; y < abs_hotspot_y + hotspot_height; ++y) {
                    if (y >= WORKSPACE_HEIGHT) continue;
                    for (int x = abs_hotspot_x; x < abs_hotspot_x + hotspot_width; ++x) {
                        if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                        
                        unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                        unsigned int alpha = (highlight_color >> 24) & 0xFF;
                        unsigned int inv_alpha = 255 - alpha;
                        
                        unsigned int r = ((highlight_color >> 16) & 0xFF);
                        unsigned int g = ((highlight_color >> 8) & 0xFF);
                        unsigned int b = (highlight_color & 0xFF);
                        
                        unsigned int existing_r = ((existing >> 16) & 0xFF);
                        unsigned int existing_g = ((existing >> 8) & 0xFF);
                        unsigned int existing_b = (existing & 0xFF);
                        
                        unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                        unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                        unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                        
                        dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                    }
                }
            }
        }
        
        return;  // Done with fullscreen rendering
    }
    
    // =========================================
    // DUAL-SCREEN MODE (Normal Mode)
    // =========================================
    // Determine screen positions based on display_swap setting
    int game_x_offset = display_swap ? KEYPAD_WIDTH : 0;
    int keypad_x_offset = display_swap ? 0 : GAME_SCREEN_WIDTH;
    
    // === UTILITY SECTION BACKGROUND (drawn first, behind buttons) ===
    // Add a more visible background to the utility area
    int util_bg_x1 = game_x_offset;
    int util_bg_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    int util_bg_y1 = 448;
    int util_bg_y2 = 600;
    
    // More visible dark background color - dark blue with better contrast than near-black
    unsigned int util_bg_color = 0xFF1a2a3a;  // Dark blue-gray with visible contrast to black
    
    for (int y = util_bg_y1; y < util_bg_y2; y++) {
        if (y >= WORKSPACE_HEIGHT) break;
        for (int x = util_bg_x1; x < util_bg_x2; x++) {
            if (x < WORKSPACE_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = util_bg_color;
            }
        }
    }
    
    // === GAME SCREEN ===
    for (int y = 0; y < GAME_SCREEN_HEIGHT; ++y) {
        int src_y = y / 2;
        for (int x = 0; x < GAME_SCREEN_WIDTH; ++x) {
            int src_x = x / 2;
            int workspace_x = game_x_offset + x;
            
            if (workspace_x >= WORKSPACE_WIDTH) continue;
            
            if (src_y < GAME_HEIGHT && src_x < GAME_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = frame[src_y * GAME_WIDTH + src_x];
            } else {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000;
            }
        }
    }
    
    // === KEYPAD ===
    // Background for keypad area
    unsigned int bg_color = 0xFF1a1a1a;
    for (int y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = keypad_x_offset + x;
            if (workspace_x < WORKSPACE_WIDTH) {
                dual_buffer[y * WORKSPACE_WIDTH + workspace_x] = bg_color;
            }
        }
    }
    
    // Layer overlay and controller base
    int ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;
    int overlay_x_offset = (KEYPAD_WIDTH - overlay_width) / 2;
    
    for (int y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (int x = 0; x < KEYPAD_WIDTH; ++x) {
            int workspace_x = keypad_x_offset + x;
            int workspace_y = y;
            
            if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
            
            unsigned int pixel = bg_color;
            
            // Layer game overlay (back)
            if (overlay_buffer && y < overlay_height) {
                int overlay_x = x - overlay_x_offset;
                if (overlay_x >= 0 && overlay_x < overlay_width) {
                    unsigned int overlay_pixel = overlay_buffer[y * overlay_width + overlay_x];
                    if ((overlay_pixel >> 24) & 0xFF) {
                        pixel = overlay_pixel;
                    }
                }
            }
            
            // Layer controller base (front)
            if (controller_base_loaded && controller_base && y < controller_base_height) {
                int ctrl_x = x - ctrl_base_x_offset;
                if (ctrl_x >= 0 && ctrl_x < controller_base_width) {
                    unsigned int base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    if ((base_pixel >> 24) & 0xFF) {
                        pixel = base_pixel;
                    }
                }
            }
            
            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = pixel;
        }
    }
    
    // === UTILITY BUTTONS (BELOW game screen, move with game when swapped) ===
    // Draw utility button PNG images
    int buttons_loaded = 0;
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
        if (utility_button_images[i].loaded) {
            buttons_loaded++;
        }
    }
    
    if (buttons_loaded > 0) {
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            // Render button 0 (fullscreen toggle) and button 2 (toggle layout)
            // All other utility buttons are disabled and not rendered
            if (i != 0 && i != 2) {
                continue;
            }
            
            if (!utility_button_images[i].loaded || !utility_button_images[i].buffer) {
                continue;
            }
            
            utility_button_t* btn = &utility_buttons[i];
            int img_width = utility_button_images[i].width;
            int img_height = utility_button_images[i].height;
            
            // Apply game_x_offset to utility button position (buttons move with game)
            int button_x_offset = game_x_offset;
            
            // Blit individual button texture to workspace
            for (int img_y = 0; img_y < img_height; img_y++) {
                for (int img_x = 0; img_x < img_width; img_x++) {
                    int workspace_x = button_x_offset + btn->x + img_x;
                    int workspace_y = btn->y + img_y;
                    
                    if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                    if (workspace_x < 0) continue;
                    
                    unsigned int button_pixel = utility_button_images[i].buffer[img_y * img_width + img_x];
                    unsigned int alpha = (button_pixel >> 24) & 0xFF;
                    
                    if (alpha > 0) {
                        // Blend with alpha
                        if (alpha == 255) {
                            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = button_pixel;
                        } else {
                            // Alpha blend
                            unsigned int existing = dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                            unsigned int inv_alpha = 255 - alpha;
                            
                            unsigned int r = ((button_pixel >> 16) & 0xFF);
                            unsigned int g = ((button_pixel >> 8) & 0xFF);
                            unsigned int b = (button_pixel & 0xFF);
                            
                            unsigned int existing_r = ((existing >> 16) & 0xFF);
                            unsigned int existing_g = ((existing >> 8) & 0xFF);
                            unsigned int existing_b = (existing & 0xFF);
                            
                            unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                            unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                            unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                            
                            dual_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                }
            }
        }
        
        // === UTILITY BUTTON HIGHLIGHTING WHEN PRESSED ===
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            // Highlight buttons 0 (fullscreen toggle) and 2 (toggle layout)
            // All other utility buttons are disabled
            if (i != 0 && i != 2) {
                continue;
            }
            
            if (utility_button_pressed[i]) {
                utility_button_t* btn = &utility_buttons[i];
                unsigned int highlight_color = 0x88FFFF00;  // Yellow semi-transparent highlight
                
                // Apply game_x_offset to highlight position (buttons move with game)
                int button_x_offset = game_x_offset;
                
                for (int y = btn->y; y < btn->y + btn->height; ++y) {
                    if (y >= WORKSPACE_HEIGHT) continue;
                    for (int x = button_x_offset + btn->x; x < button_x_offset + btn->x + btn->width; ++x) {
                        if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                        
                        unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                        unsigned int alpha = (highlight_color >> 24) & 0xFF;
                        unsigned int inv_alpha = 255 - alpha;
                        
                        unsigned int r = ((highlight_color >> 16) & 0xFF);
                        unsigned int g = ((highlight_color >> 8) & 0xFF);
                        unsigned int b = (highlight_color & 0xFF);
                        
                        unsigned int existing_r = ((existing >> 16) & 0xFF);
                        unsigned int existing_g = ((existing >> 8) & 0xFF);
                        unsigned int existing_b = (existing & 0xFF);
                        
                        unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                        unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                        unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                        
                        dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                    }
                }
            }
        }
    } else {
        // Fallback: Draw gold rectangles for buttons 0 and 2 only (if utility buttons not loaded)
        unsigned int utility_color = 0xFFFFD700;
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            // Only render buttons 0 and 2
            if (i != 0 && i != 2) {
                continue;
            }
            
            utility_button_t* btn = &utility_buttons[i];
            
            // Apply game_x_offset to button position (buttons move with game)
            int button_x_offset = game_x_offset;
            
            for (int y = btn->y; y < btn->y + btn->height; ++y) {
                if (y >= WORKSPACE_HEIGHT) break;
                for (int x = button_x_offset + btn->x; x < button_x_offset + btn->x + btn->width; ++x) {
                    if (x < 0 || x >= WORKSPACE_WIDTH) break;
                    dual_buffer[y * WORKSPACE_WIDTH + x] = utility_color;
                }
            }
        }
    }
    
    // === UTILITY SECTION BORDER - 7 LAYER GRADIENT WITH 45° CORNERS (gold retro palette) ===
    // Colors from outside to inside: #605117, #927b18, #c7a814, #ffd700, #c7a814, #927b18, #605117
    int util_border_x1 = game_x_offset;
    int util_border_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    int util_border_y1 = 448;
    int util_border_y2 = 600;
    
    // 7-layer color palette (ARGB format with full opacity)
    unsigned int border_colors[7] = {
        0xFF605117,  // Layer 0 (outermost): Dark gold/brown
        0xFF927b18,  // Layer 1: Medium-dark gold
        0xFFc7a814,  // Layer 2: Medium gold
        0xFFffd700,  // Layer 3 (center): Bright gold
        0xFFc7a814,  // Layer 4: Medium gold (mirror)
        0xFF927b18,  // Layer 5: Medium-dark gold (mirror)
        0xFF605117   // Layer 6 (innermost): Dark gold/brown (mirror)
    };
    
    // Draw each layer from outside to inside
    for (int layer = 0; layer < 7; layer++) {
        int offset = layer;
        unsigned int color = border_colors[layer];
        int corner_cut = offset;  // Amount to cut corners at 45° angle
        
        // Top border line
        for (int y = util_border_y1 + offset; y < util_border_y1 + offset + 1; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom border line
        for (int y = util_border_y2 - offset - 1; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Left border line
        for (int y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x1 + offset; x < util_border_x1 + offset + 1; x++) {
                if (x >= 0 && x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Right border line
        for (int y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (int x = util_border_x2 - offset - 1; x < util_border_x2 - offset; x++) {
                if (x < WORKSPACE_WIDTH) dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Top-left 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Top-right 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom-left 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        // Bottom-right 45° corner cut
        for (int i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                dual_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
    }
    
    // === HOTSPOT HIGHLIGHTING - Show which buttons are pressed by touch ===
    // Highlight all pressed hotspots (from touch input detection)
    // Hotspots are stored relative to keypad area; must convert to workspace coordinates
    // In dual-screen: keypad on right at GAME_SCREEN_WIDTH (or left if swapped)
    // In fullscreen: keypad position calculated from scaling
    
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
        if (hotspot_pressed[i]) {
            overlay_hotspot_t *h = &overlay_hotspots[i];
            unsigned int highlight_color = 0x6600FF00;  // Green highlight for touch-pressed, 40% transparency to match keypad overlay
            
            // Calculate absolute workspace coordinates for this hotspot
            int abs_hotspot_x, abs_hotspot_y;
            int hotspot_width = h->width;
            int hotspot_height = h->height;
            
            if (fullscreen_mode) {
                // Fullscreen mode: hotspots relative to scaled keypad
                int scaled_keypad_w = (int)(KEYPAD_WIDTH * keypad_scale_factor);
                int keypad_x_workspace = fullscreen_overlay_on_left ? 0 : (WORKSPACE_WIDTH - scaled_keypad_w);
                abs_hotspot_x = keypad_x_workspace + h->x;
                abs_hotspot_y = 0 + h->y;  // In fullscreen, keypad Y starts at 0 (no offset)
            } else {
                // Dual-screen mode: hotspots relative to keypad area
                // Keypad is at GAME_SCREEN_WIDTH when not swapped, at 0 when swapped
                int keypad_x_workspace = display_swap ? 0 : GAME_SCREEN_WIDTH;
                abs_hotspot_x = keypad_x_workspace + h->x;
                abs_hotspot_y = h->y;  // Y is already absolute
            }
            
            for (int y = abs_hotspot_y; y < abs_hotspot_y + hotspot_height; ++y) {
                if (y >= WORKSPACE_HEIGHT) continue;
                for (int x = abs_hotspot_x; x < abs_hotspot_x + hotspot_width; ++x) {
                    if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                    
                    unsigned int existing = dual_buffer[y * WORKSPACE_WIDTH + x];
                    unsigned int alpha = (highlight_color >> 24) & 0xFF;
                    unsigned int inv_alpha = 255 - alpha;
                    
                    unsigned int r = ((highlight_color >> 16) & 0xFF);
                    unsigned int g = ((highlight_color >> 8) & 0xFF);
                    unsigned int b = (highlight_color & 0xFF);
                    
                    unsigned int existing_r = ((existing >> 16) & 0xFF);
                    unsigned int existing_g = ((existing >> 8) & 0xFF);
                    unsigned int existing_b = (existing & 0xFF);
                    
                    unsigned int blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                    unsigned int blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                    unsigned int blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                    
                    dual_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                }
            }
        }
    }
}

char *SystemPath;

retro_environment_t Environ;
retro_video_refresh_t Video;
retro_audio_sample_t Audio;
retro_audio_sample_batch_t AudioBatch;
retro_input_poll_t InputPoll;
retro_input_state_t InputState;

void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
void retro_set_audio_sample(retro_audio_sample_t fn) { Audio = fn; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t fn) { AudioBatch = fn; }
void retro_set_input_poll(retro_input_poll_t fn) { InputPoll = fn; }
void retro_set_input_state(retro_input_state_t fn) { InputState = fn; }

// Forward declarations
void quit(int state);

// ========================================
// HOTSPOT INPUT HANDLING
// ========================================

// Process utility button touchscreen input and trigger RetroArch commands
static void process_utility_button_input(void)
{
    static int call_count = 0;
    call_count++;
    if (call_count % 100 == 0) {
        debug_log("[UTILITY_INPUT] Function called %d times", call_count);
    }
    
    // Get pointer/touchscreen input (RETRO_DEVICE_POINTER for touchscreen, RETRO_DEVICE_MOUSE for mouse)
    int16_t ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    int mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates (-32767 to 32767) to pixel coordinates (0 to WORKSPACE_WIDTH/HEIGHT)
    int mouse_x = 0;
    int mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
    }
    
    static int last_mouse_x = -1;
    static int last_mouse_y = -1;
    static int coord_logged = 0;
    
    // Log coordinates on change or button press
    if (mouse_button && (!coord_logged || mouse_x != last_mouse_x || mouse_y != last_mouse_y)) {
        debug_log("[UTILITY] TOUCH DETECTED! Raw: x_norm=%d y_norm=%d -> Transformed: x=%d y=%d button=%d", 
                  ptr_x_normalized, ptr_y_normalized, mouse_x, mouse_y, mouse_button);
        for (int i = 0; i < UTILITY_BUTTON_COUNT; i++) {
            utility_button_t* btn = &utility_buttons[i];
            int is_over = (mouse_x >= btn->x && mouse_x < btn->x + btn->width &&
                          mouse_y >= btn->y && mouse_y < btn->y + btn->height);
            debug_log("  Btn%d [x=%d-%d y=%d-%d]: %s", i, btn->x, btn->x+btn->width, btn->y, btn->y+btn->height, is_over?"HIT":"miss");
        }
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        coord_logged = 1;
    }
    if (!mouse_button) coord_logged = 0;
    
    // =========================================
    // FULLSCREEN MODE BOTTOM STRIP DETECTION
    // =========================================
    if (fullscreen_mode)
    {
        // Touch in bottom zone reveals control strip
        int bottom_touch_zone_y = WORKSPACE_HEIGHT - FULLSCREEN_TOUCH_ZONE;
        if (mouse_y >= bottom_touch_zone_y && mouse_y < WORKSPACE_HEIGHT)
        {
            // User touched near bottom - show strip and reset timer (unless pinned)
            fullscreen_strip_visible = 1;
            if (!fullscreen_bar_pinned) {
                fullscreen_hide_timer = FULLSCREEN_HIDE_DELAY;
            }
            
            if (mouse_button && fullscreen_strip_visible)
            {
                // Process button clicks in fullscreen strip
                int strip_y = WORKSPACE_HEIGHT - FULLSCREEN_STRIP_HEIGHT;
                button_y_pos = strip_y + (FULLSCREEN_STRIP_HEIGHT - MENU_BUTTON_HEIGHT) / 2;
                int button_y = button_y_pos;  // Local reference
                
                // Check PIN BUTTON first (far left)
                static int pin_button_pressed = 0;  // Moved outside if/else to track state correctly
                int pin_radius = 8;
                int pin_x = pin_radius + 10;  // 10 pixels from left edge
                int pin_y = strip_y + (FULLSCREEN_STRIP_HEIGHT / 2);
                
                int dx = mouse_x - pin_x;
                int dy = mouse_y - pin_y;
                int dist_sq = dx * dx + dy * dy;
                
                if (dist_sq <= (pin_radius + 2) * (pin_radius + 2))  // Touch area slightly larger than visual
                {
                    if (!pin_button_pressed)
                    {
                        pin_button_pressed = 1;
                        fullscreen_bar_pinned = !fullscreen_bar_pinned;
                        debug_log("[FULLSCREEN_PIN] Pin button toggled, bar now %s", 
                                  fullscreen_bar_pinned ? "PINNED" : "AUTO-HIDE");
                    }
                }
                else
                {
                    pin_button_pressed = 0;
                }
                
                // Check OPACITY DOTS (far right)
                int opacity_values[5] = {20, 40, 60, 80, 100};
                int dot_radius = 5;
                int dot_spacing = 14;
                int dots_start_x = WORKSPACE_WIDTH - (5 * dot_spacing) - 10;
                int dots_y = strip_y + (FULLSCREEN_STRIP_HEIGHT / 2);
                
                static int last_opacity_dot_index = -1;  // Track which dot was last pressed (-1 = none)
                int current_dot_hit = -1;  // Which dot we're currently touching
                
                // Check if we're touching any opacity dot
                for (int i = 0; i < 5; i++) {
                    int dot_x = dots_start_x + (i * dot_spacing);
                    int dx_dot = mouse_x - dot_x;
                    int dy_dot = mouse_y - dots_y;
                    int dist_sq_dot = dx_dot * dx_dot + dy_dot * dy_dot;
                    
                    if (dist_sq_dot <= (dot_radius + 3) * (dot_radius + 3)) {  // Touch area slightly larger
                        current_dot_hit = i;
                        break;
                    }
                }
                
                // Only update opacity on transition from not touching to touching
                if (current_dot_hit >= 0 && last_opacity_dot_index != current_dot_hit) {
                    fullscreen_keypad_opacity = opacity_values[current_dot_hit];
                    debug_log("[FULLSCREEN_OPACITY] Opacity set to %d%% (dot %d)", fullscreen_keypad_opacity, current_dot_hit);
                }
                
                // Update last touched dot
                if (current_dot_hit >= 0) {
                    last_opacity_dot_index = current_dot_hit;
                } else if (!mouse_button) {
                    // Reset when not touching anything and button is released
                    last_opacity_dot_index = -1;
                }
                
                // Button positions for fullscreen strip (3 buttons evenly spaced)
                button_x_pos[0] = (WORKSPACE_WIDTH / 4) - (MENU_BUTTON_WIDTH / 2);        // 1/4 - Exit
                button_x_pos[1] = (WORKSPACE_WIDTH / 2) - (MENU_BUTTON_WIDTH / 2);        // 1/2 - Swap
                button_x_pos[2] = (3 * WORKSPACE_WIDTH / 4) - (MENU_BUTTON_WIDTH / 2);    // 3/4 - Show Overlay
                
                // Check button 0 (Exit fullscreen)
                int button_x = button_x_pos[0];
                int is_over = (mouse_x >= button_x && mouse_x < button_x + MENU_BUTTON_WIDTH &&
                               mouse_y >= button_y && mouse_y < button_y + MENU_BUTTON_HEIGHT);
                
                if (is_over)
                {
                    if (!utility_button_pressed[0])
                    {
                        utility_button_pressed[0] = 1;
                        
                        // Fullscreen toggle (exit fullscreen)
                        fullscreen_mode = !fullscreen_mode;
                        fullscreen_strip_visible = 0;
                        fullscreen_hide_timer = 0;
                        
                        // Reinitialize hotspots for the mode we're entering (now exiting fullscreen, so dual-screen)
                        init_overlay_hotspots(1.0f);  // Dual-screen always uses 1.0 scale
                        
                        debug_log("[FULLSCREEN_BUTTON] Button 0 (Exit fullscreen) pressed, exiting to dual-screen");
                    }
                }
                else
                {
                    utility_button_pressed[0] = 0;
                }
                
                // Check swap button (Swap overlay left/right)
                button_x = button_x_pos[1];
                is_over = (mouse_x >= button_x && mouse_x < button_x + MENU_BUTTON_WIDTH &&
                           mouse_y >= button_y && mouse_y < button_y + MENU_BUTTON_HEIGHT);
                
                if (is_over)
                {
                    if (!utility_button_pressed[2])  // Use button index 2 for swap
                    {
                        utility_button_pressed[2] = 1;
                        
                        // Toggle overlay position in fullscreen (left/right)
                        fullscreen_overlay_on_left = !fullscreen_overlay_on_left;
                        
                        debug_log("[FULLSCREEN_BUTTON] Swap button pressed, overlay now on %s", 
                                  fullscreen_overlay_on_left ? "LEFT" : "RIGHT");
                    }
                }
                else
                {
                    utility_button_pressed[2] = 0;
                }
                
                // Check overlay toggle button (Show/Hide controller overlay)
                button_x = button_x_pos[2];
                is_over = (mouse_x >= button_x && mouse_x < button_x + MENU_BUTTON_WIDTH &&
                           mouse_y >= button_y && mouse_y < button_y + MENU_BUTTON_HEIGHT);
                
                if (is_over)
                {
                    if (!utility_button_pressed[7])  // Use button index 7 for overlay toggle
                    {
                        utility_button_pressed[7] = 1;
                        
                        // Toggle overlay visibility in fullscreen
                        overlay_visible_in_fullscreen = !overlay_visible_in_fullscreen;
                        
                        debug_log("[FULLSCREEN_BUTTON] Overlay toggle button pressed, now %s", 
                                  overlay_visible_in_fullscreen ? "VISIBLE" : "HIDDEN");
                    }
                }
                else
                {
                    utility_button_pressed[7] = 0;
                }
            }
            else if (!mouse_button)
            {
                // Mouse button released - clear all fullscreen button states
                utility_button_pressed[0] = 0;
                utility_button_pressed[2] = 0;
                utility_button_pressed[7] = 0;
            }
        }
        else
        {
            // Touch away from bottom zone - start countdown to hide strip (unless pinned)
            if (fullscreen_strip_visible && !mouse_button && !fullscreen_bar_pinned) {
                fullscreen_hide_timer--;
                if (fullscreen_hide_timer <= 0) {
                    fullscreen_strip_visible = 0;
                }
            }
            // Clear button states when not touching bottom zone
            utility_button_pressed[0] = 0;
            utility_button_pressed[2] = 0;
            utility_button_pressed[7] = 0;
        }
        
        // Process hotspots in fullscreen if overlay is visible
        if (overlay_visible_in_fullscreen) {
            // Fall through to hotspot processing below for fullscreen with overlay visible
        } else {
            return;  // Don't process hotspots if overlay is hidden in fullscreen
        }
    }
    else
    {
        // We're in dual-screen mode - process dual-screen utility buttons below
    }
    
    // =========================================
    // DUAL-SCREEN MODE UTILITY BUTTON PROCESSING
    // =========================================
    // Only process dual-screen buttons if NOT in fullscreen mode
    if (!fullscreen_mode)
    {
        // Track pressed buttons
    for (int i = 0; i < UTILITY_BUTTON_COUNT; i++)
    {
        // Only process buttons 0 (Fullscreen) and 2 (Swap) in dual-screen mode
        if (i != 0 && i != 2) {
            // Clear pressed state for disabled buttons
            utility_button_pressed[i] = 0;
            continue;
        }
        
        utility_button_t* btn = &utility_buttons[i];
        
        // When display_swap is true, utility buttons move with game (apply same offset as rendering)
        // Use the same offset calculation as the rendering: display_swap ? KEYPAD_WIDTH : 0
        int button_x = (display_swap ? KEYPAD_WIDTH : 0) + btn->x;
        
        // Check if mouse is over this button
        int is_over = (mouse_x >= button_x && mouse_x < button_x + btn->width &&
                       mouse_y >= btn->y && mouse_y < btn->y + btn->height);
        
        if (is_over && mouse_button)
        {
            // Button was pressed/held over this button
            if (!utility_button_pressed[i])
            {
                // Button press detected - trigger command
                utility_button_pressed[i] = 1;
                
                // Execute button-specific action
                switch(i)
                {
                    case 0:  // Fullscreen toggle button
                        debug_log("[BUTTON] Fullscreen button pressed at x=%d y=%d", mouse_x, mouse_y);
                        fullscreen_mode = !fullscreen_mode;
                        if (fullscreen_mode) {
                            // Entering fullscreen - show strip initially
                            fullscreen_strip_visible = 1;
                            // If bar was pinned, keep timer high; otherwise use normal delay
                            fullscreen_hide_timer = fullscreen_bar_pinned ? FULLSCREEN_HIDE_DELAY : FULLSCREEN_HIDE_DELAY;
                            // Calculate scale factor for fullscreen with visible strip
                            int game_height = WORKSPACE_HEIGHT - FULLSCREEN_STRIP_HEIGHT;
                            float scale = (float)game_height / KEYPAD_HEIGHT;
                            init_overlay_hotspots(scale);
                            debug_log("[FULLSCREEN_ENTER] Pin state=%d, timer=%d", fullscreen_bar_pinned, fullscreen_hide_timer);
                        } else {
                            // Exiting fullscreen - hide strip but preserve pin state
                            fullscreen_strip_visible = 0;
                            fullscreen_hide_timer = 0;
                            // Reinitialize hotspots for dual-screen (always 1.0 scale)
                            init_overlay_hotspots(1.0f);
                            debug_log("[FULLSCREEN_EXIT] Pin state preserved=%d", fullscreen_bar_pinned);
                        }
                        break;
                    case 2:  // Toggle layout button
                        debug_log("[BUTTON] Toggle layout button pressed at x=%d y=%d", mouse_x, mouse_y);
                        display_swap = !display_swap;
                        break;
                }
            }
        }
        else
        {
            // Button released or mouse moved away
            if (utility_button_pressed[i])
            {
                utility_button_pressed[i] = 0;
                printf("[BUTTON] Button %d released\n", i);
            }
        }
    }
    }  // End of !fullscreen_mode block
}

// Process hotspot input and update controller state directly
static void process_hotspot_input(void)
{
    static int call_count = 0;
    call_count++;
    
    // Get pointer/touchscreen input (RETRO_DEVICE_POINTER for touchscreen on Android)
    // Pointer returns coordinates in -32767 to 32767 range (normalized, not pixel coords)
    int16_t ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    int16_t ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    int mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates (-32767 to 32767) to pixel coordinates (0 to WORKSPACE_WIDTH/HEIGHT)
    // Formula: pixel = (normalized + 32767) / 65534 * workspace_size
    // This maps -32767 -> 0, 0 -> 50% of screen, 32767 -> 100%
    int mouse_x = 0;
    int mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        // Transform coordinates
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
        
        // Log HOTSPOT activity
        debug_log("[HOTSPOT_INPUT] Call#%d: mouse_x=%d, mouse_y=%d, button=%d, ptr_x_norm=%d, ptr_y_norm=%d",
                  call_count, mouse_x, mouse_y, mouse_button, ptr_x_normalized, ptr_y_normalized);
    }
    
    // Track pressed hotspots
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        overlay_hotspot_t* h = &overlay_hotspots[i];
        
        // Calculate hotspot position based on display mode and overlay position
        int hotspot_x = h->x;
        int hotspot_y = h->y;
        int hotspot_width = h->width;
        int hotspot_height = h->height;
        
        if (fullscreen_mode)
        {
            // FULLSCREEN MODE: Hotspots are stored relative to keypad area
            // Need to translate them to actual workspace coordinates
            
            // Calculate the scaled keypad's position and dimensions
            int scaled_keypad_w = (int)(KEYPAD_WIDTH * keypad_scale_factor);
            int scaled_keypad_h = (int)(KEYPAD_HEIGHT * keypad_scale_factor);
            
            // Keypad position in fullscreen (left or right depending on fullscreen_overlay_on_left)
            int keypad_x_workspace = fullscreen_overlay_on_left ? 0 : (WORKSPACE_WIDTH - scaled_keypad_w);
            int keypad_y_workspace = 0;  // Always at top in fullscreen
            
            // Convert hotspot positions from keypad-relative to workspace-absolute
            hotspot_x = keypad_x_workspace + h->x;
            hotspot_y = keypad_y_workspace + h->y;
        }
        else
        {
            // DUAL-SCREEN MODE: Hotspots are stored relative to keypad area
            // Keypad is on RIGHT side at GAME_SCREEN_WIDTH
            // Need to add the base offset to make them absolute workspace coordinates
            int keypad_x_workspace = display_swap ? 0 : GAME_SCREEN_WIDTH;
            
            hotspot_x = keypad_x_workspace + h->x;
            hotspot_y = h->y;  // Y is already absolute
        }
        
        // Check if mouse is over this hotspot
        int is_over = (mouse_x >= hotspot_x && mouse_x < hotspot_x + hotspot_width &&
                       mouse_y >= hotspot_y && mouse_y < hotspot_y + hotspot_height);
        
        if (is_over && mouse_button)
        {
            // Button was pressed/held over this hotspot
            if (!hotspot_pressed[i])
            {
                // Button press detected - send keypad code
                hotspot_pressed[i] = 1;
                debug_log("[HOTSPOT_PRESS] Button %d (idx=%d) pressed at (%d,%d) code=0x%02X",
                          i, i, mouse_x, mouse_y, h->keypad_code);
            }
        }
        else
        {
            // Button released or mouse moved away
            if (hotspot_pressed[i])
            {
                hotspot_pressed[i] = 0;
                debug_log("[HOTSPOT_RELEASE] Button %d released", i);
            }
        }
    }
    
    // Build controller input from pressed hotspots (including held buttons from previous frames)
    int hotspot_input = 0;
    for (int i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        // Send hotspot input if currently pressed
        if (hotspot_pressed[i])
        {
            hotspot_input |= overlay_hotspots[i].keypad_code;
            debug_log("[HOTSPOT_COMBINE] Button %d: code=0x%02X, combined=0x%02X",
                      i, overlay_hotspots[i].keypad_code, hotspot_input);
        }
    }
    
    // Send hotspot input directly to controller 0 (player 1)
    if (hotspot_input)
    {
        debug_log("[HOTSPOT_SEND] hotspot_input=0x%02X -> setControllerInput(0, 0x%02X)",
                  hotspot_input, hotspot_input);
        setControllerInput(0, hotspot_input);
    }
}

struct retro_game_geometry Geometry;

static bool libretro_supports_option_categories = false;

int joypad0[20]; // joypad 0 state
int joypad1[20]; // joypad 1 state
int joypre0[20]; // joypad 0 previous state
int joypre1[20]; // joypad 1 previous state

bool paused = false;

bool keyboardChange = false;
bool keyboardDown = false;
int  keyboardState = 0;

// at 44.1khz, read 735 samples (44100/60) 
// at 48khz, read 800 samples (48000/60)
// e.g. audioInc = 3733.5 / 735
int audioSamples = AUDIO_FREQUENCY / 60;

double audioBufferPos = 0.0;
double audioInc;

double ivoiceBufferPos = 0.0;
double ivoiceInc;

unsigned int frameWidth = MaxWidth;
unsigned int frameHeight = MaxHeight;
unsigned int frameSize =  MaxWidth * MaxHeight; //78848

void quit(int state)
{
	cleanup_utility_buttons();
	Reset();
	MemoryInit();
}

static void Keyboard(bool down, unsigned keycode,
      uint32_t character, uint16_t key_modifiers)
{
	/* Keyboard Input */
	keyboardDown = down;
	keyboardChange = true; 
	switch (character)
	{
		case 48: keyboardState = keypadStates[10]; break; // 0
		case 49: keyboardState = keypadStates[0]; break; // 1
		case 50: keyboardState = keypadStates[1]; break; // 2
		case 51: keyboardState = keypadStates[2]; break; // 3
		case 52: keyboardState = keypadStates[3]; break; // 4
		case 53: keyboardState = keypadStates[4]; break; // 5
		case 54: keyboardState = keypadStates[5]; break; // 6
		case 55: keyboardState = keypadStates[6]; break; // 7
		case 56: keyboardState = keypadStates[7]; break; // 8
		case 57: keyboardState = keypadStates[8]; break; // 9
		case 91: keyboardState = keypadStates[9]; break; // C [
		case 93: keyboardState = keypadStates[11]; break; // E ]
		default: 
			keyboardChange = false;
			keyboardDown = false;
	}
}

static void check_variables(bool first_run)
{
	struct retro_variable var = {0};

	if (first_run)
	{
		var.key   = "default_p1_controller";
		var.value = NULL;

		// by default input 0 maps to Right Controller (0x1FE)
		// and input 1 maps to Left Controller (0x1FF)
		controllerSwap = 0;

		if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "left") == 0)
				controllerSwap = 1;
		}
	}
}

void retro_set_environment(retro_environment_t fn)
{
	Environ = fn;

	// Set core options
	libretro_supports_option_categories = false;
	libretro_set_core_options(Environ, &libretro_supports_option_categories);
}

void retro_init(void)
{
	char execPath[PATH_MAX_LENGTH];
	char gromPath[PATH_MAX_LENGTH];
	struct retro_keyboard_callback kb = { Keyboard };

	// controller descriptors
	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Last Selected Keypad Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Swap Left/Right Controllers" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Show Keypad" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad [1-9]" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad [1-9]" },

		{ 0 },
	};

	// init buffers, structs
	memset(frame, 0, frameSize);
	OSD_setDisplay(frame, MaxWidth, MaxHeight);

	Environ(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	// reset console
	Init();
	Reset();

	// get paths
	Environ(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &SystemPath);

	// load exec
	fill_pathname_join(execPath, SystemPath, "exec.bin", PATH_MAX_LENGTH);
	loadExec(execPath);

	// load grom
	fill_pathname_join(gromPath, SystemPath, "grom.bin", PATH_MAX_LENGTH);
	loadGrom(gromPath);

	// Setup keyboard input
	Environ(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &kb);
}

bool retro_load_game(const struct retro_game_info *info)
{
	check_variables(true);
	LoadGame(info->path);
	
	// Capture system directory and load overlays
	if (SystemPath && SystemPath[0]) {
		strncpy(system_dir, SystemPath, sizeof(system_dir) - 1);
		system_dir[sizeof(system_dir) - 1] = '\0';
		printf("[GAME] ==================== SYSTEM DIR DEBUG ====================\n");
		printf("[GAME] Raw SystemPath from RetroArch: %s\n", SystemPath);
		printf("[GAME] Copied to system_dir: %s\n", system_dir);
		printf("[GAME] system_dir length: %zu\n", strlen(system_dir));
		printf("[GAME] Last char: '%c' (code %d)\n", system_dir[strlen(system_dir)-1], system_dir[strlen(system_dir)-1]);
		printf("[GAME] ============================================================\n");
		
		// Load controller base, utility buttons, and ROM-specific overlay
		load_controller_base();
		load_utility_buttons();
		load_overlay_for_rom(info->path);
		init_overlay_hotspots(1.0f);  // Initialize with normal scale (1.0 = full size)
	} else {
		printf("[GAME] ERROR: SystemPath is NULL or empty!\n");
	}
	
	return true;
}

void retro_unload_game(void)
{
	quit(0);
}

void retro_run(void)
{
	int c, i, j, k, l;
	int showKeypad0 = false;
	int showKeypad1 = false;

	bool options_updated  = false;
	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
		check_variables(false);

	InputPoll();
	
	// Update fullscreen mode timer
	// Only auto-hide if NOT pinned
	if (fullscreen_mode && fullscreen_strip_visible && !fullscreen_bar_pinned && fullscreen_hide_timer > 0) {
		fullscreen_hide_timer--;
		if (fullscreen_hide_timer <= 0) {
			fullscreen_strip_visible = 0;
			debug_log("[FULLSCREEN] Auto-hide timer expired, hiding strip");
		}
	}
	
	// DEBUG: Check pointer input at the start of retro_run and write to file
	static int debug_frame_count = 0;
	if (debug_frame_count < 300) {
		int px = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
		int py = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
		int pp = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
		
		FILE *f = fopen("/storage/emulated/0/Download/freeintv_pointer_debug.txt", "a");
		if (f) {
			fprintf(f, "Frame %d: POINTER x=%d y=%d pressed=%d\n", debug_frame_count, px, py, pp);
			fflush(f);
			fclose(f);
		}
		debug_frame_count++;
	}

	for(i = 0; i < 20; i++) // Copy previous state
	{
		joypre0[i] = joypad0[i];
		joypre1[i] = joypad1[i];
	}

	/* JoyPad 0 */
	joypad0[0] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad0[1] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad0[2] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad0[3] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad0[4] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad0[5] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad0[6] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad0[7] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad0[8] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad0[9] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad0[10] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad0[11] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad0[12] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad0[13] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad0[14] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[15] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[18] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad0[19] = InputState(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	/* JoyPad 1 */
	joypad1[0] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
	joypad1[1] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
	joypad1[2] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
	joypad1[3] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

	joypad1[4] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
	joypad1[5] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
	joypad1[6] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
	joypad1[7] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

	joypad1[8] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
	joypad1[9] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

	joypad1[10] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
	joypad1[11] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
	joypad1[12] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2);
	joypad1[13] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

	joypad1[14] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[15] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[18] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3);
	joypad1[19] = InputState(1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3);

	// Pause
	if((joypad0[8]==1 && joypre0[8]==0) || (joypad1[8]==1 && joypre1[8]==0))
	{
		paused = !paused;
		if(paused)
		{
			OSD_drawPaused();
			OSD_drawTextCenterBG(21, "HELP - PRESS A");
		}
	}

	if(paused)
	{
		// help menu //
		if(joypad0[4]==1 || joypad1[4]==1)
		{
			OSD_drawTextBG(3,  4, "                                      ");
			OSD_drawTextBG(3,  5, "               - HELP -               ");
			OSD_drawTextBG(3,  6, "                                      ");
			OSD_drawTextBG(3,  7, " A      - RIGHT ACTION BUTTON         ");
			OSD_drawTextBG(3,  8, " B      - LEFT ACTION BUTTON          ");
			OSD_drawTextBG(3,  9, " Y      - TOP ACTION BUTTON           ");
			OSD_drawTextBG(3, 10, " X      - LAST SELECTED KEYPAD BUTTON ");
			OSD_drawTextBG(3, 11, " L/R    - SHOW KEYPAD                 ");
			OSD_drawTextBG(3, 12, " LT/RT  - KEYPAD CLEAR/ENTER          ");
			OSD_drawTextBG(3, 13, "                                      ");
			OSD_drawTextBG(3, 14, " START  - PAUSE GAME                  ");
			OSD_drawTextBG(3, 15, " SELECT - SWAP LEFT/RIGHT CONTROLLERS ");
			OSD_drawTextBG(3, 16, "                                      ");
			OSD_drawTextBG(3, 17, " FREEINTV 1.2          LICENSE GPL V2+");
			OSD_drawTextBG(3, 18, "                                      ");
		}
	}
	else
	{
		// Process hotspot input directly - each hotspot assigned to its relative keypad button
		process_hotspot_input();
		
		// Process utility button input - map to RetroArch commands
		process_utility_button_input();
		
		// Keep regular controller input for compatibility with non-overlay gameplay
		// If no hotspot is pressed, fall back to standard controller input
		int any_hotspot_pressed = 0;
		for (int h = 0; h < OVERLAY_HOTSPOT_COUNT; h++)
		{
			if (hotspot_pressed[h])
			{
				any_hotspot_pressed = 1;
				break;
			}
		}
		
		// If no hotspots pressed, handle regular controller input
		if (!any_hotspot_pressed)
		{
			setControllerInput(0, getControllerState(joypad0, 0));
		}

		// Player 2 controller input (unchanged - no hotspot overlay for player 2)
		if(joypad1[10] | joypad1[11]) // left shoulder down
		{
			showKeypad1 = true;
			setControllerInput(1, getKeypadState(1, joypad1, joypre1));
		}
		else
		{
			showKeypad1 = false;
			setControllerInput(1, getControllerState(joypad1, 1));
		}

		if(keyboardDown || keyboardChange)
		{
			setControllerInput(0, keyboardState);
			keyboardChange = false;
		}

		// grab frame
		Run();

		// draw overlays
		if(showKeypad0) { drawMiniKeypad(0, frame); }
		if(showKeypad1) { drawMiniKeypad(1, frame); }

		// sample audio from buffer
		audioInc = 3733.5 / audioSamples;
		ivoiceInc = 1.0;

		j = 0;
		for(i=0; i<audioSamples; i++)
		{
			// Sound interpolator:
			//   The PSG module generates audio at 224010 hz (3733.5 samples per frame)
			//   Very high frequencies like 0x0001 would generate chirps on output
			//   (For example, Lock&Chase) so this code interpolates audio, making
			//   these silent as in real hardware.
			audioBufferPos += audioInc;
			k = audioBufferPos;
			l = k - j;

			c = 0;
			while (j < k)
				c += PSGBuffer[j++];
			c = c / l;
			// Finally it adds the Intellivoice output (properly generated at the
			// same frequency as output)
			c = (c + ivoiceBuffer[(int) ivoiceBufferPos]) / 2;

			Audio(c, c); // Audio(left, right)

			ivoiceBufferPos += ivoiceInc;

			if (ivoiceBufferPos >= ivoiceBufferSize)
				ivoiceBufferPos = 0.0;

			audioBufferPos = audioBufferPos * (audioBufferPos<(PSGBufferSize-1));
		}
		audioBufferPos = 0.0;
		PSGFrame();
		ivoiceBufferPos = 0.0;
		ivoice_frame();
	}

	// Swap Left/Right Controller
	if(joypad0[9]==1 || joypad1[9]==1)
	{
		if ((joypad0[9]==1 && joypre0[9]==0) || (joypad1[9]==1 && joypre1[9]==0))
		{
			controllerSwap = controllerSwap ^ 1;
		}
		if(controllerSwap==1)
		{
			OSD_drawLeftRight();
		}
		else
		{
			OSD_drawRightLeft();
		}
	}

	if (intv_halt)
		OSD_drawTextBG(3, 5, "INTELLIVISION HALTED");
	
	// Render dual-screen display (game + keypad)
	render_dual_screen();
	
	// Send frame to libretro
	if (dual_screen_enabled && dual_screen_buffer) {
		Video(dual_screen_buffer, WORKSPACE_WIDTH, WORKSPACE_HEIGHT, sizeof(unsigned int) * WORKSPACE_WIDTH);
	} else {
		Video(frame, frameWidth, frameHeight, sizeof(unsigned int) * frameWidth);
	}

}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "FreeIntv";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
	info->library_version = "1.2 " GIT_VERSION;
	info->valid_extensions = "int|bin|rom";
	info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;

	memset(info, 0, sizeof(*info));
	
	// Report dimensions based on dual-screen mode
	if (dual_screen_enabled) {
		info->geometry.base_width   = WORKSPACE_WIDTH;
		info->geometry.base_height  = WORKSPACE_HEIGHT;
		info->geometry.max_width    = WORKSPACE_WIDTH;
		info->geometry.max_height   = WORKSPACE_HEIGHT;
		info->geometry.aspect_ratio = ((float)WORKSPACE_WIDTH) / ((float)WORKSPACE_HEIGHT);
	} else {
		info->geometry.base_width   = MaxWidth;
		info->geometry.base_height  = MaxHeight;
		info->geometry.max_width    = MaxWidth;
		info->geometry.max_height   = MaxHeight;
		info->geometry.aspect_ratio = ((float)MaxWidth) / ((float)MaxHeight);
	}

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = AUDIO_FREQUENCY;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}


void retro_deinit(void)
{
	libretro_supports_option_categories = false;
	quit(0);
}

void retro_reset(void)
{
	// Reset (from intv.c) //
	Reset();
}

RETRO_API void *retro_get_memory_data(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return Memory;
	}
	return 0;
}

RETRO_API size_t retro_get_memory_size(unsigned id)
{
	if(id==RETRO_MEMORY_SYSTEM_RAM)
	{
		return 0x10000;
	}
	return 0;
}

#define SERIALIZED_VERSION 0x4f544702

struct serialized {
	int version;
	struct CP1610serialized CP1610;
	struct STICserialized STIC;
	struct PSGserialized PSG;
	struct ivoiceSerialized ivoice;
	unsigned int Memory[0x10000];   // Should be equal to Memory.c
	// Extra variables from intv.c
	int SR1;
	int intv_halt;
};

size_t retro_serialize_size(void)
{
	return sizeof(struct serialized);
}

bool retro_serialize(void *data, size_t size)
{
	struct serialized *all;

	all = (struct serialized *) data;
	all->version = SERIALIZED_VERSION;
	CP1610Serialize(&all->CP1610);
	STICSerialize(&all->STIC);
	PSGSerialize(&all->PSG);
	ivoiceSerialize(&all->ivoice);
	memcpy(all->Memory, Memory, sizeof(Memory));
	all->SR1 = SR1;
	all->intv_halt = intv_halt;
	return true;
}

bool retro_unserialize(const void *data, size_t size)
{
	const struct serialized *all;

	all = (const struct serialized *) data;
	if (all->version != SERIALIZED_VERSION)
		return false;
	CP1610Unserialize(&all->CP1610);
	STICUnserialize(&all->STIC);
	PSGUnserialize(&all->PSG);
	ivoiceUnserialize(&all->ivoice);
	memcpy(Memory, all->Memory, sizeof(Memory));
	SR1 = all->SR1;
	intv_halt = all->intv_halt;
	return true;
}

/* Stubs */
unsigned int retro_api_version(void) { return RETRO_API_VERSION; }
void retro_cheat_reset(void) {  }
void retro_cheat_set(unsigned index, bool enabled, const char *code) {  }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {  }
