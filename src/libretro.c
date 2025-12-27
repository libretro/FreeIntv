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

// Include embedded image assets (converted from PNG files)
#include "banner.h"
#include "keypad_frame_graphic.h"
#include "default_keypad_image.h"

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224
#define MAX_PADS 2

// ========================================
// HORIZONTAL LAYOUT DISPLAY CONFIGURATION
// ========================================
// Game Screen: Left side (704×448, 2x scaled from 352×224)
// Keypad: Right side (370×600)
// Total Workspace: 1074 × 600 pixels (keypad full height)

#define WORKSPACE_WIDTH 1074    // Game (704) + Keypad (370)
#define WORKSPACE_HEIGHT 600    // Keypad full height (600px)
#define GAME_SCREEN_WIDTH 704   // 352 * 2x
#define GAME_SCREEN_HEIGHT 448  // 224 * 2x
#define KEYPAD_WIDTH 370        // Keypad overlay width
#define KEYPAD_HEIGHT 600       // Keypad overlay height
// Keypad hotspot configuration
#define OVERLAY_HOTSPOT_COUNT 12
#define OVERLAY_HOTSPOT_SIZE 70

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
    int id;
    int keypad_code;
} overlay_hotspot_t;

overlay_hotspot_t overlay_hotspots[OVERLAY_HOTSPOT_COUNT];

// Display system variables
static int multi_screen_enabled = 0;  // Default to disabled - enable via core option
static void* multi_screen_buffer = NULL;
static const int GAME_WIDTH = 352;
static const int GAME_HEIGHT = 224;
static int display_swap = 0;  // 0 = game left/keypad right, 1 = game right/keypad left

// Hotspot input tracking
static int hotspot_pressed[OVERLAY_HOTSPOT_COUNT] = {0};  // Track which hotspots are currently pressed

// PNG overlay system
static char current_rom_path[512] = {0};
static unsigned int* overlay_buffer = NULL;
static int overlay_loaded = 0;
static int overlay_width = 370;
static int overlay_height = 600;

// Controller base
static unsigned int* controller_base = NULL;
static int controller_base_loaded = 0;
static int controller_base_width = 446;
static int controller_base_height = 620;

// Banner for utility workspace
static unsigned int* banner_buffer = NULL;
static int banner_loaded = 0;
static int banner_width = 704;
static int banner_height = 152;

// Toggle button hotspot (in the gold box area of the banner)
static int toggle_button_pressed = 0;
static int last_toggle_button_state = 0;

/* Initialize overlay hotspots for keypad (positioned on RIGHT side) */
static void init_overlay_hotspots(void)
{
    /* Layout: 4 rows x 3 columns, positioned on RIGHT side of workspace */
    int hotspot_w = OVERLAY_HOTSPOT_SIZE;
    int hotspot_h = OVERLAY_HOTSPOT_SIZE;
    int gap_x = 28;
    int gap_y = 29;
    int rows = 4;
    int cols = 3;
    
    /* Position keypad on right side: start at GAME_SCREEN_WIDTH */
    int keypad_x_offset = GAME_SCREEN_WIDTH;
    int keypad_y_offset = 0;  /* Align to top of keypad region */
    
    /* IMPORTANT: Controller base is 446px wide, centered in 370px keypad space */
    /* This creates a left/right margin of (370 - 446) / 2 = -38px (extends beyond) */
    /* Hotspots must account for this centering offset */
    int ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;  /* = -38 */
    
    /* Center hotspots within the ACTUAL controller base (446px), not the keypad space */
    int hotspots_width = 3 * hotspot_w + 2 * gap_x;  /* 266 */
    int hotspots_x_in_base = (controller_base_width - hotspots_width) / 2;  /* center in 446px */
    int top_margin = 183;  /* From DS version: hotspots start 183px from top of workspace */
    
    int start_x = keypad_x_offset + ctrl_base_x_offset + hotspots_x_in_base;
    int start_y = keypad_y_offset + top_margin;
    int row, col, idx;
    
    int keypad_map[12] = { K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_C, K_0, K_E };
    
    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            idx = row * cols + col;
            overlay_hotspots[idx].x = start_x + col * (hotspot_w + gap_x);
            overlay_hotspots[idx].y = start_y + row * (hotspot_h + gap_y);
            overlay_hotspots[idx].width = hotspot_w;
            overlay_hotspots[idx].height = hotspot_h;
            overlay_hotspots[idx].id = idx + 1;
            overlay_hotspots[idx].keypad_code = keypad_map[idx];
        }
    }
}

// Helper function to build system overlay path (handles both Windows \\ and Android / paths)
// Forward declaration for build_overlay_path (ROM-specific overlay)
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size, const char* system_dir);

// Load controller base PNG from embedded data
static void load_controller_base(void)
{
    int width, height, channels, y, x;
    unsigned char* img_data;
    unsigned char* pixel;
    unsigned int alpha, r, g, b;
    
    if (controller_base_loaded) {
        return;
    }
    
    img_data = stbi_load_from_memory(keypad_frame_graphic, keypad_frame_graphic_len, &width, &height, &channels, 4);
    
    if (img_data) {
        controller_base_width = width;
        controller_base_height = height;
        
        if (!controller_base) {
            controller_base = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        }
        
        if (controller_base) {
            int y, x;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    controller_base[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            controller_base_loaded = 1;
        }
    } else {
    }
}

// Load banner PNG from embedded data
static void load_banner(void)
{
    int width, height, channels, y, x;
    unsigned char* img_data;
    unsigned char* pixel;
    unsigned int alpha, r, g, b;
    
    if (banner_loaded) {
        return;
    }
    
    img_data = stbi_load_from_memory(banner, banner_len, &width, &height, &channels, 4);
    
    if (!img_data) {
        return;
    }
    
    banner_width = width;
    banner_height = height;
    
    if (!banner_buffer) {
        banner_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
    }
    
    if (banner_buffer) {
        int y, x;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                unsigned char* pixel = img_data + (y * width + x) * 4;
                unsigned int alpha = pixel[3];
                unsigned int r = pixel[0];
                unsigned int g = pixel[1];
                unsigned int b = pixel[2];
                banner_buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
            }
        }
        banner_loaded = 1;
        stbi_image_free(img_data);
    }
}

// Build overlay path from ROM name - looks in system/freeintv_overlays folder
static void build_overlay_path(const char* rom_path, char* overlay_path, size_t overlay_path_size, const char* system_dir)
{
    const char* filename;
    const char* last_slash;
    char rom_basename[512];
    char* ext;
    
    if (!rom_path || !overlay_path || overlay_path_size == 0 || !system_dir) {
        overlay_path[0] = '\0';
        return;
    }
    
    // Get ROM filename without extension
    filename = rom_path;
    last_slash = strrchr(rom_path, '\\');
    if (!last_slash) last_slash = strrchr(rom_path, '/');
    if (last_slash) filename = last_slash + 1;
    
    // Remove extension
    strncpy(rom_basename, filename, sizeof(rom_basename) - 1);
    rom_basename[sizeof(rom_basename) - 1] = '\0';
    ext = strrchr(rom_basename, '.');
    if (ext) *ext = '\0';
    
    // Build path: [system_dir]\freeintv_overlays\[rom_name].png
    // Use backslash for Windows, forward slash for other systems
    #ifdef _WIN32
    snprintf(overlay_path, overlay_path_size, "%s\\freeintv_overlays\\%s.png", system_dir, rom_basename);
    #else
    snprintf(overlay_path, overlay_path_size, "%s/freeintv_overlays/%s.png", system_dir, rom_basename);
    #endif
}

// Load overlay for ROM
static void load_overlay_for_rom(const char* rom_path, const char* system_dir)
{
    char overlay_path[1024], jpg_path[1024];
    int width, height, channels, y, x, from_file;
    unsigned char* img_data;
    unsigned char* pixel;
    unsigned int alpha, r, g, b;
    char* ext;
    
    if (!rom_path || !system_dir || !multi_screen_enabled) {
        return;
    }
    
    build_overlay_path(rom_path, overlay_path, sizeof(overlay_path), system_dir);
    
    overlay_loaded = 0;
    
    if (overlay_buffer) {
        free(overlay_buffer);
        overlay_buffer = NULL;
    }
    
    img_data = stbi_load(overlay_path, &width, &height, &channels, 4);
    from_file = 1;
    
    if (!img_data) {
        // Try JPG format
        strncpy(jpg_path, overlay_path, sizeof(jpg_path) - 1);
        ext = strrchr(jpg_path, '.');
        if (ext) {
            strcpy(ext, ".jpg");
            img_data = stbi_load(jpg_path, &width, &height, &channels, 4);
            from_file = 1;
        }
    }
    
    // Fall back to embedded default image
    if (!img_data) {
        img_data = stbi_load_from_memory(default_keypad_image, default_keypad_image_len, &width, &height, &channels, 4);
        from_file = 0;
    }
    
    if (img_data) {
        overlay_width = width;
        overlay_height = height;
        overlay_buffer = (unsigned int*)malloc(width * height * sizeof(unsigned int));
        
        if (overlay_buffer) {
            int y, x;
            for (y = 0; y < height; y++) {
                for (x = 0; x < width; x++) {
                    unsigned char* pixel = img_data + (y * width + x) * 4;
                    unsigned int alpha = pixel[3];
                    unsigned int r = pixel[0];
                    unsigned int g = pixel[1];
                    unsigned int b = pixel[2];
                    overlay_buffer[y * width + x] = (alpha << 24) | (r << 16) | (g << 8) | b;
                }
            }
            init_overlay_hotspots();
        }
        // Only free if it came from a file, not if it's embedded
        if (from_file) {
            stbi_image_free(img_data);
        }
    } else {
        overlay_width = 370;
        overlay_height = 600;
        overlay_buffer = (unsigned int*)malloc(overlay_width * overlay_height * sizeof(unsigned int));
        if (overlay_buffer) {
            int y, x;
            for (y = 0; y < overlay_height; y++) {
                for (x = 0; x < overlay_width; x++) {
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


// Render display with game screen LEFT and keypad RIGHT
static void render_multi_screen(void)
{
    int i, y, x;
    unsigned int* multi_buffer;
    extern unsigned int frame[352 * 224];
    int game_x_offset, keypad_x_offset;
    int util_bg_x1, util_bg_x2, util_bg_y1, util_bg_y2;
    int src_y, src_x, workspace_x, workspace_y;
    unsigned int bg_color;
    int overlay_x, overlay_y, overlay_workspace_x, overlay_workspace_y;
    unsigned int overlay_pixel, overlay_pixel_val;
    int banner_start_x, banner_start_y;
    int banner_x, banner_y;
    int banner_workspace_x, banner_workspace_y;
    unsigned int banner_pixel;
    unsigned int existing;
    int blended_r, blended_g, blended_b;
    float alpha;
    int button_idx, btn_y, btn_x;
    int color;
    int hotspot_idx, workspace_idx;
    int layer, offset, corner_cut;
    unsigned int border_colors[7];
    int util_border_x1, util_border_x2, util_border_y1, util_border_y2;
    unsigned int pixel, base_pixel;
    unsigned int inv_alpha;
    unsigned int base_r, base_g, base_b;
    unsigned int bg_r, bg_g, bg_b;
    int ctrl_base_x_offset, overlay_x_offset, ctrl_x;
    unsigned int utility_bg_color;
    unsigned int r, g, b;
    unsigned int existing_r, existing_g, existing_b;
    int hotspot_x_adjust;
    unsigned int highlight_color;
    
    if (!multi_screen_enabled) return;
    
    if (!multi_screen_buffer) {
        multi_screen_buffer = malloc(WORKSPACE_WIDTH * WORKSPACE_HEIGHT * sizeof(unsigned int));
    }
    if (!multi_screen_buffer) return;
    
    multi_buffer = (unsigned int*)multi_screen_buffer;
    
    /* Clear entire workspace with black */
    for (i = 0; i < WORKSPACE_WIDTH * WORKSPACE_HEIGHT; i++) {
        multi_buffer[i] = 0xFF000000;
    }
    
    /* Determine screen positions based on display_swap setting */
    game_x_offset = display_swap ? KEYPAD_WIDTH : 0;
    keypad_x_offset = display_swap ? 0 : GAME_SCREEN_WIDTH;
    
    /* === UTILITY SECTION BACKGROUND (drawn first, behind buttons) === */
    /* Add a more visible background to the utility area */
    util_bg_x1 = game_x_offset;
    util_bg_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    util_bg_y1 = 448;
    util_bg_y2 = 600;
    
    /* More visible dark background color - dark blue with better contrast than near-black */
    utility_bg_color = 0xFF1a2a3a;  /* Dark blue-gray with visible contrast to black */
    
    for (y = util_bg_y1; y < util_bg_y2; y++) {
        if (y >= WORKSPACE_HEIGHT) break;
        for (x = util_bg_x1; x < util_bg_x2; x++) {
            if (x < WORKSPACE_WIDTH) {
                multi_buffer[y * WORKSPACE_WIDTH + x] = utility_bg_color;
            }
        }
    }
    
    // === GAME SCREEN ===
    for (y = 0; y < GAME_SCREEN_HEIGHT; ++y) {
        src_y = y / 2;
        for (x = 0; x < GAME_SCREEN_WIDTH; ++x) {
            src_x = x / 2;
            workspace_x = game_x_offset + x;
            
            if (workspace_x >= WORKSPACE_WIDTH) continue;
            
            if (src_y < GAME_HEIGHT && src_x < GAME_WIDTH) {
                multi_buffer[y * WORKSPACE_WIDTH + workspace_x] = frame[src_y * GAME_WIDTH + src_x];
            } else {
                multi_buffer[y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000;
            }
        }
    }
    
    /* === KEYPAD === */
    /* Background for keypad area */
    bg_color = 0xFF1a1a1a;
    for (y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (x = 0; x < KEYPAD_WIDTH; ++x) {
            workspace_x = keypad_x_offset + x;
            if (workspace_x < WORKSPACE_WIDTH) {
                multi_buffer[y * WORKSPACE_WIDTH + workspace_x] = bg_color;
            }
        }
    }
    
    // Layer overlay and controller base
    ctrl_base_x_offset = (KEYPAD_WIDTH - controller_base_width) / 2;
    overlay_x_offset = (KEYPAD_WIDTH - overlay_width) / 2;
    
    
    for (y = 0; y < KEYPAD_HEIGHT && y < WORKSPACE_HEIGHT; ++y) {
        for (x = 0; x < KEYPAD_WIDTH; ++x) {
            workspace_x = keypad_x_offset + x;
            workspace_y = y;
            
            if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
            
            pixel = bg_color;
            
            // If overlay is loaded, show overlay with controller base on top
            if (overlay_loaded && overlay_buffer && y < overlay_height) {
                overlay_x = x - overlay_x_offset;
                if (overlay_x >= 0 && overlay_x < overlay_width) {
                    overlay_pixel_val = overlay_buffer[y * overlay_width + overlay_x];
                    if ((overlay_pixel_val >> 24) & 0xFF) {
                        pixel = overlay_pixel_val;
                    }
                }
            }
            // Only use controller base if NO overlay is loaded
            else if (!overlay_loaded && controller_base_loaded && controller_base && y < controller_base_height) {
                ctrl_x = x - ctrl_base_x_offset;
                if (ctrl_x >= 0 && ctrl_x < controller_base_width) {
                    base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    alpha = (base_pixel >> 24) & 0xFF;
                    if (alpha > 0) {
                        pixel = base_pixel;
                    }
                }
            }
            
            // Layer controller base on top (with overlay showing through transparent areas)
            if (overlay_loaded && controller_base_loaded && controller_base && y < controller_base_height) {
                ctrl_x = x - ctrl_base_x_offset;
                if (ctrl_x >= 0 && ctrl_x < controller_base_width) {
                    base_pixel = controller_base[y * controller_base_width + ctrl_x];
                    alpha = (base_pixel >> 24) & 0xFF;
                    if (alpha > 0) {
                        if (alpha == 255) {
                            // Fully opaque: use controller base
                            pixel = base_pixel;
                        } else {
                            // Semi-transparent: blend controller base over overlay
                            inv_alpha = 255 - alpha;
                            base_r = (base_pixel >> 16) & 0xFF;
                            base_g = (base_pixel >> 8) & 0xFF;
                            base_b = base_pixel & 0xFF;
                            
                            bg_r = (pixel >> 16) & 0xFF;
                            bg_g = (pixel >> 8) & 0xFF;
                            bg_b = pixel & 0xFF;
                            
                            blended_r = (base_r * alpha + bg_r * inv_alpha) / 255;
                            blended_g = (base_g * alpha + bg_g * inv_alpha) / 255;
                            blended_b = (base_b * alpha + bg_b * inv_alpha) / 255;
                            
                            pixel = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                        }
                    }
                    // If alpha == 0 (transparent), pixel stays as overlay
                }
            }
            
            multi_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = pixel;
        }
    }
    

    
    /* === RENDER BANNER IN UTILITY WORKSPACE === */
    if (banner_loaded && banner_buffer) {
        /* Blit banner to utility area at position (game_x_offset, 448) */
        for (banner_y = 0; banner_y < banner_height; banner_y++) {
            for (banner_x = 0; banner_x < banner_width; banner_x++) {
                workspace_x = game_x_offset + banner_x;
                workspace_y = 448 + banner_y;                if (workspace_x >= WORKSPACE_WIDTH || workspace_y >= WORKSPACE_HEIGHT) continue;
                if (workspace_x < 0) continue;
                
                banner_pixel = banner_buffer[banner_y * banner_width + banner_x];
                alpha = (banner_pixel >> 24) & 0xFF;
                
                if (alpha > 0) {
                    // Blend with alpha
                    if (alpha == 255) {
                        multi_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = banner_pixel;
                    } else {
                        // Alpha blend
                        existing = multi_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x];
                        inv_alpha = 255 - alpha;
                        
                        r = ((banner_pixel >> 16) & 0xFF);
                        g = ((banner_pixel >> 8) & 0xFF);
                        b = (banner_pixel & 0xFF);
                        
                        existing_r = ((existing >> 16) & 0xFF);
                        existing_g = ((existing >> 8) & 0xFF);
                        existing_b = (existing & 0xFF);
                        
                        blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                        blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                        blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                        
                        multi_buffer[workspace_y * WORKSPACE_WIDTH + workspace_x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
                    }
                }
            }
        }
    } else {
        /* Fallback: Draw dark background if banner not loaded */
        unsigned int utility_bg_color = 0xFF1a2a3a;  /* Dark blue-gray */
        for (y = 448; y < 600; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (x = game_x_offset; x < game_x_offset + GAME_SCREEN_WIDTH; x++) {
                if (x >= WORKSPACE_WIDTH) break;
                multi_buffer[y * WORKSPACE_WIDTH + x] = utility_bg_color;
            }
        }
    }
    
    // === UTILITY SECTION BORDER - 7 LAYER GRADIENT WITH 45° CORNERS (gold retro palette) ===
    // Colors from outside to inside: #605117, #927b18, #c7a814, #ffd700, #c7a814, #927b18, #605117
    util_border_x1 = game_x_offset;
    util_border_x2 = game_x_offset + GAME_SCREEN_WIDTH;
    util_border_y1 = 448;
    util_border_y2 = 600;
    
    // 7-layer color palette (ARGB format with full opacity)
    border_colors[0] = 0xFF605117;  // Layer 0 (outermost): Dark gold/brown
    border_colors[1] = 0xFF927b18;  // Layer 1: Medium-dark gold
    border_colors[2] = 0xFFc7a814;  // Layer 2: Medium gold
    border_colors[3] = 0xFFffd700;  // Layer 3 (center): Bright gold
    border_colors[4] = 0xFFc7a814;  // Layer 4: Medium gold (mirror)
    border_colors[5] = 0xFF927b18;  // Layer 5: Medium-dark gold (mirror)
    border_colors[6] = 0xFF605117;  // Layer 6 (innermost): Dark gold/brown (mirror)
    
    /* Draw each layer from outside to inside */
    for (layer = 0; layer < 7; layer++) {
        int offset = layer;
        unsigned int color = border_colors[layer];
        int corner_cut = offset;  /* Amount to cut corners at 45° angle */
        int i;
        
        /* Top border line */
        for (y = util_border_y1 + offset; y < util_border_y1 + offset + 1; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Bottom border line */
        for (y = util_border_y2 - offset - 1; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (x = util_border_x1 + corner_cut; x < util_border_x2 - corner_cut; x++) {
                if (x < WORKSPACE_WIDTH) multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Left border line */
        for (y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (x = util_border_x1 + offset; x < util_border_x1 + offset + 1; x++) {
                if (x >= 0 && x < WORKSPACE_WIDTH) multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Right border line */
        for (y = util_border_y1 + offset; y < util_border_y2 - offset; y++) {
            if (y >= WORKSPACE_HEIGHT) break;
            for (x = util_border_x2 - offset - 1; x < util_border_x2 - offset; x++) {
                if (x < WORKSPACE_WIDTH) multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Top-left 45° corner cut */
        for (i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Top-right 45° corner cut */
        for (i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y1 + offset + i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y < WORKSPACE_HEIGHT) {
                multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Bottom-left 45° corner cut */
        for (i = 0; i < corner_cut; i++) {
            int x = util_border_x1 + i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
        
        /* Bottom-right 45° corner cut */
        for (i = 0; i < corner_cut; i++) {
            int x = util_border_x2 - 1 - i;
            int y = util_border_y2 - 1 - offset - i;
            if (x >= 0 && x < WORKSPACE_WIDTH && y >= 0 && y < WORKSPACE_HEIGHT) {
                multi_buffer[y * WORKSPACE_WIDTH + x] = color;
            }
        }
    }
    
    /* === HOTSPOT HIGHLIGHTING - Show which buttons are pressed by touch === */
    /* Highlight all pressed hotspots (from touch input detection) */
    /* When display_swap is true, hotspots translate from right side to left side */
    hotspot_x_adjust = display_swap ? (-GAME_SCREEN_WIDTH) : 0;
    
    for (i = 0; i < OVERLAY_HOTSPOT_COUNT; i++) {
        if (hotspot_pressed[i]) {
            overlay_hotspot_t *h = &overlay_hotspots[i];
            unsigned int highlight_color = 0xAA00FF00;  /* Green highlight for touch-pressed */
            
            for (y = h->y; y < h->y + h->height; ++y) {
                if (y >= WORKSPACE_HEIGHT) continue;
                for (x = h->x + hotspot_x_adjust; x < h->x + h->width + hotspot_x_adjust; ++x) {
                    if (x < 0 || x >= WORKSPACE_WIDTH) continue;
                    
                    existing = multi_buffer[y * WORKSPACE_WIDTH + x];
                    alpha = (highlight_color >> 24) & 0xFF;
                    inv_alpha = 255 - alpha;
                    
                    r = ((highlight_color >> 16) & 0xFF);
                    g = ((highlight_color >> 8) & 0xFF);
                    b = (highlight_color & 0xFF);
                    
                    existing_r = ((existing >> 16) & 0xFF);
                    existing_g = ((existing >> 8) & 0xFF);
                    existing_b = (existing & 0xFF);
                    
                    blended_r = (r * alpha + existing_r * inv_alpha) / 255;
                    blended_g = (g * alpha + existing_g * inv_alpha) / 255;
                    blended_b = (b * alpha + existing_b * inv_alpha) / 255;
                    
                    multi_buffer[y * WORKSPACE_WIDTH + x] = 0xFF000000 | (blended_r << 16) | (blended_g << 8) | blended_b;
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
static void process_toggle_button_input(void)
{
    int16_t ptr_x_normalized;
    int16_t ptr_y_normalized;
    int mouse_button;
    int mouse_x;
    int mouse_y;
    int banner_start_x;
    int banner_start_y;
    int toggle_x;
    int toggle_y;
    int toggle_radius;
    int dx;
    int dy;
    int distance_sq;
    int is_over;
    
    // Get pointer/touchscreen input
    ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates to pixel coordinates
    mouse_x = 0;
    mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
    }
    
    // Define toggle button hotspot - 80×80 gold box
    // Position: X=611, Y=36 (relative to banner)
    // Margins: 36px from top/bottom, 13px from right
    // Center: (651, 76) relative to banner
    
    banner_start_x = display_swap ? KEYPAD_WIDTH : 0;
    banner_start_y = 448;
    
    // Toggle button center in workspace coordinates
    toggle_x = banner_start_x + 651;  // Banner X + center X of gold box
    toggle_y = banner_start_y + 76;   // Banner Y + center Y of gold box
    toggle_radius = 45;  // Approximately half diagonal of 80×80 box for circular touch detection
    
    // Check if touch is within toggle button area (circular hotspot)
    dx = mouse_x - toggle_x;
    dy = mouse_y - toggle_y;
    distance_sq = dx * dx + dy * dy;
    is_over = (distance_sq <= toggle_radius * toggle_radius);
    
    if (is_over && mouse_button) {
        if (!toggle_button_pressed) {
            toggle_button_pressed = 1;
            last_toggle_button_state = 1;
        }
    } else {
        if (toggle_button_pressed && last_toggle_button_state) {
            // Button released - perform toggle action
            display_swap = !display_swap;
        }
        toggle_button_pressed = 0;
        last_toggle_button_state = 0;
    }
}

// Process hotspot input and update controller state directly
static void process_hotspot_input(void)
{
    static int call_count = 0;
    int16_t ptr_x_normalized;
    int16_t ptr_y_normalized;
    int mouse_button;
    int mouse_x;
    int mouse_y;
    int i;
    overlay_hotspot_t* h;
    int hotspot_x;
    int is_over;
    int hotspot_input;
    
    call_count++;
    
    // Get pointer/touchscreen input (RETRO_DEVICE_POINTER for touchscreen on Android)
    // Pointer returns coordinates in -32767 to 32767 range (normalized, not pixel coords)
    ptr_x_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    ptr_y_normalized = (int16_t)InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    mouse_button = InputState(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
    
    // Transform from normalized coordinates (-32767 to 32767) to pixel coordinates (0 to WORKSPACE_WIDTH/HEIGHT)
    // Formula: pixel = (normalized + 32767) / 65534 * workspace_size
    // This maps -32767 -> 0, 0 -> 50% of screen, 32767 -> 100%
    mouse_x = 0;
    mouse_y = 0;
    if (ptr_x_normalized != 0 || ptr_y_normalized != 0 || mouse_button) {
        // Transform coordinates
        mouse_x = ((int32_t)ptr_x_normalized + 32767) * WORKSPACE_WIDTH / 65534;
        mouse_y = ((int32_t)ptr_y_normalized + 32767) * WORKSPACE_HEIGHT / 65534;
        // Clamp to workspace bounds
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= WORKSPACE_WIDTH) mouse_x = WORKSPACE_WIDTH - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= WORKSPACE_HEIGHT) mouse_y = WORKSPACE_HEIGHT - 1;
    }
    
    // Track pressed hotspots
    for (i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        h = &overlay_hotspots[i];
        
        // When display_swap is true, keypad moves to LEFT (0) and game moves to RIGHT (370)
        // Hotspots are defined with keypad on RIGHT (x starts at 704), so translate them
        // In normal mode: hotspot at original x position
        // In swapped mode: subtract GAME_SCREEN_WIDTH (704) to move to LEFT side
        hotspot_x = h->x;
        if (display_swap) {
            // Translate hotspot from RIGHT side to LEFT side
            // Original x is ~750-883 (right side), new x should be ~46-179 (left side, same relative position)
            hotspot_x = h->x - GAME_SCREEN_WIDTH;
        }
        
        // Check if mouse is over this hotspot
        is_over = (mouse_x >= hotspot_x && mouse_x < hotspot_x + h->width &&
                       mouse_y >= h->y && mouse_y < h->y + h->height);
        
        if (is_over && mouse_button)
        {
            // Button was pressed/held over this hotspot
            if (!hotspot_pressed[i])
            {
                // Button press detected - send keypad code
                hotspot_pressed[i] = 1;
            }
        }
        else
        {
            // Button released or mouse moved away
            if (hotspot_pressed[i])
            {
                hotspot_pressed[i] = 0;
            }
        }
    }
    
    // Build controller input from pressed hotspots (including held buttons from previous frames)
    hotspot_input = 0;
    for (i = 0; i < OVERLAY_HOTSPOT_COUNT; i++)
    {
        // Send hotspot input if currently pressed
        if (hotspot_pressed[i])
        {
            hotspot_input |= overlay_hotspots[i].keypad_code;
        }
    }
    
    // Send hotspot input directly to controller 0 (player 1)
    if (hotspot_input)
    {
        setControllerInput(0, hotspot_input);
    }
}

struct retro_game_geometry Geometry;

static bool libretro_supports_bitmasks = false;
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

static void update_input(void)
{
	int i, j;
	int joypad_bits[2];

	InputPoll();

	for (i = 0; i < 20; i++) // Copy previous state 
	{
		joypre0[i] = joypad0[i];
		joypre1[i] = joypad1[i];
	}

	if (libretro_supports_bitmasks)
	{
		for (j = 0; j < MAX_PADS; j++)
			joypad_bits[j] = InputState(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
		}
	else
	{
		for (j = 0; j < MAX_PADS; j++)
		{
			joypad_bits[j] = 0;
			for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
				joypad_bits[j] |= InputState(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
		}
	}

	/* JoyPad 0 */
	joypad0[0] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 1 : 0;
	joypad0[1] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 1 : 0;
	joypad0[2] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 1 : 0;
	joypad0[3] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 1 : 0;

	joypad0[4] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 1 : 0;
	joypad0[5] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 1 : 0;
	joypad0[6] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X)      ? 1 : 0;
	joypad0[7] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)      ? 1 : 0;

	joypad0[8] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 1 : 0;
	joypad0[9] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;

	joypad0[10] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L)     ? 1 : 0;
	joypad0[11] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R)     ? 1 : 0;
	joypad0[12] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)    ? 1 : 0;
	joypad0[13] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)    ? 1 : 0;
	joypad0[18] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)    ? 1 : 0;
	joypad0[19] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)    ? 1 : 0;

	joypad0[14] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[15] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad0[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

	/* JoyPad 1 */
	joypad1[0] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 1 : 0;
	joypad1[1] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 1 : 0;
	joypad1[2] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 1 : 0;
	joypad1[3] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 1 : 0;

	joypad1[4] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 1 : 0;
	joypad1[5] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 1 : 0;
	joypad1[6] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_X)      ? 1 : 0;
	joypad1[7] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)      ? 1 : 0;

	joypad1[8] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 1 : 0;
	joypad1[9] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;

	joypad1[10] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L)     ? 1 : 0;
	joypad1[11] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R)     ? 1 : 0;
	joypad1[12] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)    ? 1 : 0;
	joypad1[13] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)    ? 1 : 0;
	joypad1[18] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)    ? 1 : 0;
	joypad1[19] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)    ? 1 : 0;

	joypad1[14] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[15] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
	joypad1[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
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

		// Check multi-screen overlay option
		var.key   = "freeintv_multiscreen_overlay";
		var.value = NULL;
		multi_screen_enabled = 0;  // Default disabled

		if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			if (strcmp(var.value, "enabled") == 0)
				multi_screen_enabled = 1;
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

	if (Environ(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
		libretro_supports_bitmasks = true;

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
		
		// Load embedded asset images (controller base, banner, overlay)
		load_controller_base();
		load_banner();
		load_overlay_for_rom(info->path, SystemPath);
		init_overlay_hotspots();	return true;
}

void retro_unload_game(void)
{
	quit(0);
}

void retro_run(void)
{
	int c, i, j, k, l;
	int showKeypad0;
	int showKeypad1;
	bool options_updated;
	static int debug_frame_count = 0;
	int px;
	int py;
	int pp;
	FILE *f;
	int any_hotspot_pressed;
	int h;
	
	showKeypad0 = false;
	showKeypad1 = false;

	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
		check_variables(false);

	update_input();

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
			OSD_drawTextBG(3, 17, " freeintv 1.2          LICENSE GPL V2+");
			OSD_drawTextBG(3, 18, "                                      ");
		}
	}
	else
	{
		// SINGLE-SCREEN MODE: Use original FreeIntv keypad popup behavior
		if (!multi_screen_enabled)
		{
			// Player 1: L/R button shows keypad overlay
			if(joypad0[10] | joypad0[11]) // left/right shoulder down
			{
				showKeypad0 = true;
				setControllerInput(0, getKeypadState(0, joypad0, joypre0));
			}
			else
			{
				showKeypad0 = false;
				setControllerInput(0, getControllerState(joypad0, 0));
			}

			// Player 2: L/R button shows keypad overlay
			if(joypad1[10] | joypad1[11]) // left/right shoulder down
			{
				showKeypad1 = true;
				setControllerInput(1, getKeypadState(1, joypad1, joypre1));
			}
			else
			{
				showKeypad1 = false;
				setControllerInput(1, getControllerState(joypad1, 1));
			}
		}
		// MULTI-SCREEN MODE: Use overlay hotspot system
		else
		{
			// Process hotspot input directly - each hotspot assigned to its relative keypad button
			process_hotspot_input();
			
			// Process toggle button input - map to screen swap
			process_toggle_button_input();
			
			// Keep regular controller input for compatibility with non-overlay gameplay
			// If no hotspot is pressed, fall back to standard controller input
			any_hotspot_pressed = 0;
			for (h = 0; h < OVERLAY_HOTSPOT_COUNT; h++)
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
	
	// Render multi-screen display (game + keypad)
	render_multi_screen();
	
	// Send frame to libretro
	if (multi_screen_enabled && multi_screen_buffer) {
		Video(multi_screen_buffer, WORKSPACE_WIDTH, WORKSPACE_HEIGHT, sizeof(unsigned int) * WORKSPACE_WIDTH);
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
	info->library_name = "freeintv";
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
	
	// Report dimensions based on multi-screen mode
	if (multi_screen_enabled) {
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
	libretro_supports_bitmasks = false;
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
