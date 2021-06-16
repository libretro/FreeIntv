/*
	This file is part of FreeIntv.

	FreeIntv is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	FreeIntv is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeIntv.  If not, see http://www.gnu.org/licenses/
*/
#include <math.h>
#include "controller.h"

#include "cp1610.h"
#include "memory.h"

const double PI = 3.14159265358979323846;

#define K_1 0x81
#define K_2 0x41
#define K_3 0x21
#define K_4 0x82
#define K_5 0x42
#define K_6 0x22
#define K_7 0x84
#define K_8 0x44
#define K_9 0x24
#define K_0 0x48
#define K_C 0x88
#define K_E 0x28

#define B_TOP 0xA0
#define B_LEFT 0x60
#define B_RIGHT 0xC0

#define D_N   0x04
#define D_NNE 0x14
#define D_NE  0x16
#define D_ENE 0x06
#define D_E   0x02
#define D_ESE 0x12
#define D_SE  0x13
#define D_SSE 0x03
#define D_S   0x01
#define D_SSW 0x11
#define D_SW  0x19
#define D_WSW 0x09
#define D_W   0x08
#define D_WNW 0x18
#define D_NW  0x1C
#define D_NNW 0x0C


int controllerSwap;

// 16-way DISC directions, clockwise from North
int discDirections[16] ={ D_N, D_NNE, D_NE, D_ENE, D_E, D_ESE, D_SE, D_SSE, D_S, D_SSW, D_SW, D_WSW, D_W, D_WNW, D_NW, D_NNW };
int keypadDirections[8]={ K_2, K_3, K_6, K_9, K_8, K_7, K_4, K_1 };

// keypad states: 1,2,3, 4,5,6, 7,8,9, C,0,E
int keypadStates[12]={ K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_C, K_0, K_E };

int cursor[4] = { 0, 0, 0, 0 }; // mini keypad cursor (button row/column p0x,p0y p1x,p1y)

int getQuickKeypadState(int player);

void controllerInit()
{
	controllerSwap = 0;
	// by default input 0 maps to Right Controller (0x1FE)
	// and input 1 maps to Left Controller (0x1FF)
	// pressing select (freeintv_libretro.c) will
	// swap the left and right controllers
}

void setControllerInput(int player, int state)
{
	CTX(Memory)[(player^controllerSwap) + 0x1FE] = (state^0xFF) & 0xFF;
}

int getControllerState(int joypad[], int player)
{
	// converts joypad input for use by system  
	int Lx = 0; // left analog X
	int Ly = 0; // left analog Y
	int Rx = 0; // right analog X
	int Ry = 0; // right analog Y
	double theta; // analog joy angle
	int norm; // theta, normalized 

	int state = 0; //0xFF;

	if(joypad[0]!=0) { state |= D_N; } // 0xFB - Up
	if(joypad[1]!=0) { state |= D_S; } // 0xFE - Down
	if(joypad[2]!=0) { state |= D_W; } // 0xF7 - Left
	if(joypad[3]!=0) { state |= D_E; } // 0xFD - Right

	if(joypad[0]!=0 && joypad[2]!=0) { state |= D_NW; } // 0xE3 - Up+Left
	if(joypad[0]!=0 && joypad[3]!=0) { state |= D_NE; } // 0xE9 - Up+Right
	if(joypad[1]!=0 && joypad[2]!=0) { state |= D_SW; } // 0x36 - Down+Left
	if(joypad[1]!=0 && joypad[3]!=0) { state |= D_SE; } // 0x3C - Down+Right

	if(joypad[7]!=0) { state |= B_TOP; } // 0x5F - Button Top
	if(joypad[4]!=0) { state |= B_LEFT; } // 0x9F - Button Left
	if(joypad[5]!=0) { state |= B_RIGHT; } // 0x3F - Button Right

	if(joypad[6]!=0) { state |= getQuickKeypadState(player); }

	/* Analog Controls for 16-way disc control */

	Lx = joypad[14] / 8192;
	Ly = joypad[15] / 8192;
	if(Lx != 0 || Ly != 0)
	{
		// find angle 
		theta = atan2((double)Ly, (double)Lx) + PI;
		// normalize
		if(theta<0.0) { theta = 0.0; }
		norm = floor((theta/(2*PI))*15.0);
		norm -= 3; 
		if(norm<0) { norm += 16; }
		state |= discDirections[norm & 0x0F];
	}

	// Right-analog to keypad mapping (for Tron Deadly Discs)
	Rx = joypad[16] / 8192;
	Ry = joypad[17] / 8192;
	if(Rx != 0 || Ry != 0)
	{
		// find angle 
		theta = atan2((double)Ry, (double)Rx) + PI;
		// normalize
		if(theta<0.0) { theta = 0.0; }
		norm = floor((theta/(2*PI))*7.0);
		norm -= 1; 
		if(norm<0) { norm += 8; }
		state |= keypadDirections[norm & 0x07];
	}

	return state;
}

// Mini keypads, displayed in the corner using a shoulder button,
// should better enable controller-only play.

/* 39 x 27*/
int miniKeypadImage[1053] = 
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,1,0,0,1,1,1,0,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,1,0,0,0,0,0,1,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,1,1,0,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,0,1,1,1,1,0,1,0,0,1,1,1,0,0,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,0,1,0,0,1,0,0,1,0,0,1,1,1,1,0,1,0,0,0,1,1,1,0,1,0,
0,1,0,1,0,0,1,0,0,1,0,0,1,0,0,0,0,1,0,0,1,0,0,0,0,1,0,
0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,
0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,
0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,
0,1,0,0,0,0,1,0,0,1,0,0,1,1,1,0,0,1,0,0,0,1,1,0,0,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,0,0,1,1,1,1,0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,
0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,
0,1,0,0,0,0,0,1,0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,1,0,1,0,
0,1,0,0,0,0,1,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,
0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,1,0,
0,1,0,0,1,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,1,1,1,1,1,0,1,0,
0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,1,0,1,1,1,1,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,
0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,0,0,1,0,
0,1,0,0,1,1,1,0,0,1,0,0,1,1,1,0,0,1,0,1,1,1,1,1,0,1,0,
0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int getKeypadState(int player, int joypad[], int joypre[])
{
	int cursorX = cursor[player*2];
	int cursorY = cursor[player*2+1];
	int state = 0x0;

	// move cursor only on button down
	if(joypre[0]==0 && joypad[0]!=0) { cursorY--; if(cursorY<0) { cursorY = 3; } } // up
	if(joypre[1]==0 && joypad[1]!=0) { cursorY++; if(cursorY>3) { cursorY = 0; } } // down
	if(joypre[2]==0 && joypad[2]!=0) { cursorX--; if(cursorX<0) { cursorX = 2; } } // left
	if(joypre[3]==0 && joypad[3]!=0) { cursorX++; if(cursorX>2) { cursorX = 0; } } // right

	cursor[player*2] = cursorX;
	cursor[player*2+1] = cursorY;

	// let any face button press keypad
	if(joypad[4]!=0 || joypad[5]!=0 || joypad[6]!=0 || joypad[7]!=0)
	{
		state = keypadStates[(cursorY*3)+cursorX];
	}
	return state;
}

int getQuickKeypadState(int player)
{
	int cursorX = cursor[player*2];
	int cursorY = cursor[player*2+1];
	return keypadStates[(cursorY*3)+cursorX];
}

void drawMiniKeypad(int player, unsigned int frame[])
{
	int i, j, k;
	int cursorX = cursor[player*2];
	int cursorY = cursor[player*2+1];

	// draw keypad //
	int offset = 65120 + (player*325);
	k = 0;
	for(i=0; i<39; i++)
	{
		for(j=0; j<27; j++)
		{
			frame[offset+j] = miniKeypadImage[k]*0xFFFFFF;
			k++;
		}
		offset+=352;
	}

	// draw cursor
	offset = 65120 + (player*325) + (2*352) + 2;
	offset = offset + (8*cursorX) + ((9*352)*cursorY);
	for(i=0; i<7; i++)
	{
		frame[offset+i] = 0x00FF00;
	}
	for(i=0; i<6; i++)
	{
		offset+=352;
		frame[offset] = 0x00FF00;
		frame[offset+6] = 0x00FF00;
	}
	offset+=352;
	for(i=0; i<7; i++)
	{
		frame[offset+i] = 0x00FF00;
	}
}
