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
#include <string.h>
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

#define DefaultFPS 60
#define MaxWidth 352
#define MaxHeight 224

#define RETRO_DEVICE_INTV_CONTROL_KEYMAP          RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)

#define MAX_PADS 2
#define JOYPAD_BUTTONS 18

static unsigned input_devices[MAX_PADS];

char *SystemPath;

retro_environment_t Environ;
retro_video_refresh_t Video;
retro_audio_sample_t Audio;
retro_audio_sample_batch_t AudioBatch;
retro_input_poll_t InputPoll;
retro_input_state_t InputState;
retro_log_printf_t log_cb;

void retro_set_video_refresh(retro_video_refresh_t fn) { Video = fn; }
void retro_set_audio_sample(retro_audio_sample_t fn) { Audio = fn; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t fn) { AudioBatch = fn; }
void retro_set_input_poll(retro_input_poll_t fn) { InputPoll = fn; }
void retro_set_input_state(retro_input_state_t fn) { InputState = fn; }

struct retro_game_geometry Geometry;

static bool libretro_supports_bitmasks = false;

int joypad0[20]; // joypad 0 state
int joypad1[20]; // joypad 1 state
int joypre0[20]; // joypad 0 previous state
int joypre1[20]; // joypad 1 previous state

bool paused = false;

bool keyboardChange = false;
bool keyboardDown = false;
int  keyboardState = 0;
static bool dual_controllers = false;

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

void init_input_descriptors(void)
{
	// This will hold the final set of descriptor info after we evaluate the configured inputs
	struct retro_input_descriptor desc[(JOYPAD_BUTTONS * MAX_PADS) + 1];

	// Define descriptors for both the default and keymap layouts for both players
	struct retro_input_descriptor desc_p1_default[] = {
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
	};

	struct retro_input_descriptor desc_p1_keymap[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Keypad 1" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Keypad 9" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Keypad 3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Keypad 7" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad 4(-)/6(+)" },
		{ 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad 2(+)/8(-)" },
	};

	struct retro_input_descriptor desc_p2_default[] = {
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

	struct retro_input_descriptor desc_p2_keymap[] = {
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Disc Left" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Disc Up" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Disc Down" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Disc Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Left Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Right Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Top Action Button" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Keypad 1" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Keypad 9" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Console Pause" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Keypad 3" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Keypad 7" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Keypad Clear" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Keypad Enter" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Keypad 0" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Keypad 5" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Keypad 4(-)/6(+)" },
		{ 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Keypad 2(+)/8(-)" },
		{ 0 },
	};

	// Evaluate the assigned device for P1 and P2 and copy the appropriate descriptors for each
	switch (input_devices[0])
	{
		case RETRO_DEVICE_JOYPAD:
			memcpy(desc, desc_p1_default, JOYPAD_BUTTONS * sizeof(struct retro_input_descriptor));
			break;
		case RETRO_DEVICE_INTV_CONTROL_KEYMAP:
			memcpy(desc, desc_p1_keymap, JOYPAD_BUTTONS * sizeof(struct retro_input_descriptor));
			break;
	}

	switch (input_devices[1])
	{
		case RETRO_DEVICE_JOYPAD:
			memcpy(desc + JOYPAD_BUTTONS, desc_p2_default, (JOYPAD_BUTTONS + 1) * sizeof(struct retro_input_descriptor));
			break;
		case RETRO_DEVICE_INTV_CONTROL_KEYMAP:
			memcpy(desc + JOYPAD_BUTTONS, desc_p2_keymap, (JOYPAD_BUTTONS + 1) * sizeof(struct retro_input_descriptor));
			break;
	}

	Environ(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
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

	// Check if the Dual Controller option should be set
	var.key   = "dual_controller_enabled";
	var.value = NULL;

	dual_controllers = false;

	if (Environ(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strcmp(var.value, "enabled") == 0)
			dual_controllers = true;
	}
}

static void update_input(void)
{
	int i, j;
	int joypad_bits[2];

	InputPoll();

	for(i = 0; i < 20; i++) // Copy previous state 
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

	joypad0[14] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad0[15] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

	// If we have enabled the Dual Controller option, the keypad and action buttons will come from the other controller
	if (!dual_controllers)
	{
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

		joypad0[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
		joypad0[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	}
	else
	{
		joypad0[4] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 1 : 0;
		joypad0[5] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 1 : 0;
		joypad0[6] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_X)      ? 1 : 0;
		joypad0[7] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)      ? 1 : 0;

		joypad0[8] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 1 : 0;
		joypad0[9] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;

		joypad0[10] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L)     ? 1 : 0;
		joypad0[11] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R)     ? 1 : 0;
		joypad0[12] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)    ? 1 : 0;
		joypad0[13] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)    ? 1 : 0;
		joypad0[18] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)    ? 1 : 0;
		joypad0[19] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)    ? 1 : 0;

		joypad0[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
		joypad0[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	}

	/* JoyPad 1 */
	joypad1[0] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 1 : 0;
	joypad1[1] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 1 : 0;
	joypad1[2] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 1 : 0;
	joypad1[3] = joypad_bits[1] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 1 : 0;

	joypad1[14] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
	joypad1[15] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

	// If we have enabled the Dual Controller option, the keypad and action buttons will come from the other controller
	if (!dual_controllers)
	{
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

		joypad1[16] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
		joypad1[17] = InputState(1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	}
	else
	{
		joypad1[4] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 1 : 0;
		joypad1[5] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 1 : 0;
		joypad1[6] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_X)      ? 1 : 0;
		joypad1[7] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)      ? 1 : 0;

		joypad1[8] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 1 : 0;
		joypad1[9] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;

		joypad1[10] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L)     ? 1 : 0;
		joypad1[11] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R)     ? 1 : 0;
		joypad1[12] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)    ? 1 : 0;
		joypad1[13] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)    ? 1 : 0;
		joypad1[18] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)    ? 1 : 0;
		joypad1[19] = joypad_bits[0] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)    ? 1 : 0;

		joypad1[16] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
		joypad1[17] = InputState(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
	}
}

void retro_set_environment(retro_environment_t fn)
{
	static const struct retro_controller_description port_1[] = {
		{ "Intellivision Controller (OSD Keypad)", RETRO_DEVICE_JOYPAD },
		{ "Intellivision Controller (Direct Keypad Buttons)", RETRO_DEVICE_INTV_CONTROL_KEYMAP }
	};

	static const struct retro_controller_description port_2[] = {
		{ "Intellivision Controller (OSD Keypad)", RETRO_DEVICE_JOYPAD },
		{ "Intellivision Controller (Direct Keypad Buttons)", RETRO_DEVICE_INTV_CONTROL_KEYMAP }
	};

	static const struct retro_controller_info ports[] = {
		{ port_1, 2 },
		{ port_2, 2 },
		{ 0 },
	};

	Environ = fn;
	libretro_set_core_options(Environ);
	Environ(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	switch (device)
	{
		case RETRO_DEVICE_JOYPAD:
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "%s\n", "[libretro]: Found RETRO_DEVICE_JOYPAD");
			input_devices[port] = RETRO_DEVICE_JOYPAD;
			keypadButtonMode[port] = 0;
			break;
		case RETRO_DEVICE_INTV_CONTROL_KEYMAP:
			if (log_cb)
				log_cb(RETRO_LOG_INFO, "%s\n", "[libretro]: Found RETRO_DEVICE_INTV_CONTROL_KEYMAP");
			input_devices[port] = RETRO_DEVICE_INTV_CONTROL_KEYMAP;
			keypadButtonMode[port] = 1;
			break;
		default:
			if (log_cb)
				log_cb(RETRO_LOG_ERROR, "%s\n", "[libretro]: Invalid device, setting type to RETRO_DEVICE_JOYPAD");
			input_devices[port] = RETRO_DEVICE_JOYPAD;
			keypadButtonMode[port] = 0;
	}

	init_input_descriptors();
}

void retro_init(void)
{
	char execPath[PATH_MAX_LENGTH];
	char gromPath[PATH_MAX_LENGTH];
	struct retro_keyboard_callback kb = { Keyboard };
	struct retro_log_callback log;

	if (Environ(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
		log_cb = log.log;
	else
		log_cb = NULL;

	// init buffers, structs
	memset(frame, 0, frameSize);
	OSD_setDisplay(frame, MaxWidth, MaxHeight);
   
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
			OSD_drawTextBG(3, 17, " FREEINTV 1.2          LICENSE GPL V2+");
			OSD_drawTextBG(3, 18, "                                      ");
		}
	}
	else
	{
		// Only show the OSD keypad if the controller type is the default type - otherwise L/R are utilized as a normal input button
		// @todo need to fix this to handle the dual controller option so it works as expected even when this option is enabled
		if ((joypad0[10] | joypad0[11]) && !keypadButtonMode[0]) // left/right shoulder down
		{
			showKeypad0 = true;
			setControllerInput(0, getKeypadState(0, joypad0, joypre0));
		}
		else
		{
			showKeypad0 = false;
			setControllerInput(0, getControllerState(joypad0, 0));
		}

		if((joypad1[10] | joypad1[11]) && !keypadButtonMode[1]) // left shoulder down
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
	// Only trigger the controller swap if the controller type is the default type - otherwise select is utilized as a normal input button
	if ((joypad0[9]==1 && !keypadButtonMode[0]) || (joypad1[9]==1 && !keypadButtonMode[1]))
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
	// send frame to libretro
	Video(frame, frameWidth, frameHeight, sizeof(unsigned int) * frameWidth);

}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "FreeIntv";
	info->library_version = "1.2";
	info->valid_extensions = "int|bin|rom";
	info->need_fullpath = true;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	int pixelformat = RETRO_PIXEL_FORMAT_XRGB8888;

	memset(info, 0, sizeof(*info));
	info->geometry.base_width   = MaxWidth;
	info->geometry.base_height  = MaxHeight;
	info->geometry.max_width    = MaxWidth;
	info->geometry.max_height   = MaxHeight;
	info->geometry.aspect_ratio = ((float)MaxWidth) / ((float)MaxHeight);

	info->timing.fps = DefaultFPS;
	info->timing.sample_rate = AUDIO_FREQUENCY;

	Environ(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixelformat);
}

void retro_deinit(void)
{
	libretro_supports_bitmasks = false;
	dual_controllers = false;
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
