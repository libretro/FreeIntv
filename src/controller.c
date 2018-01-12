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
#include "memory.h"

const double PI = 3.14159265358979323846;

// 16-way DISC directions, clockwise from North
// ---------------------N    NNE  NE   ENE   E    ESE  SE   SSE   S    SSW  SW  WSW    W    WNW  NW   NNW
int discDirections[16]={ 0xFB,0xEB,0xE9,0xF9, 0xFD,0xED,0xEC,0xFC, 0xFE,0xEE,0xE6,0xF6, 0xF7,0xE7,0xE3,0xF3 };

// keypad states: 1,2,3, 4,5,6, 7,8,9, C,0,E
int keypadStates[12]={ 0x7E,0xBE,0xDE, 0x7D,0xBD,0xDD, 0x7B,0xBB,0xDB, 0xB7,0x77,0xD7 };

// buttons: Top, Left, Right
// 0xF7, 0xFD, 0xFE

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
	Memory[(player^controllerSwap) + 0x1FE] = state & 0xFF;
}

int getControllerState(int joypad[], int player)
{
	// converts joypad input for use by system  
	int Lx = 0; // left analog X
	int Ly = 0; // left analog Y
	double theta; // analog joy angle
	int norm; // theta, normalized 

	int state = 0xFF;

	if(joypad[0]!=0) { state &= 0xFB; } // 0xFB - Up
	if(joypad[1]!=0) { state &= 0xFE; } // 0xFE - Down
	if(joypad[2]!=0) { state &= 0xF7; } // 0xF7 - Left
	if(joypad[3]!=0) { state &= 0xFD; } // 0xFD - Right

	if(joypad[0]!=0 && joypad[2]!=0) { state &= 0xE3; } // 0xE3 - Up+Left
	if(joypad[0]!=0 && joypad[3]!=0) { state &= 0xE9; } // 0xE9 - Up+Right
	if(joypad[1]!=0 && joypad[2]!=0) { state &= 0xE6; } // 0x36 - Down+Left
	if(joypad[1]!=0 && joypad[3]!=0) { state &= 0xEC; } // 0x3C - Down+Right

	if(joypad[7]!=0) { state &= 0x5F; } // 0xF7 - Button Top
	if(joypad[4]!=0) { state &= 0x9F; } // 0xFD - Button Left
	if(joypad[5]!=0) { state &= 0x3F; } // 0xFE - Button Right

	if(joypad[6]!=0) { state &= getQuickKeypadState(player); }

	/* Analog Controls for 16-way disc control */

	Lx = joypad[14] / 8192;
	Ly = joypad[15] / 8192;
	if(Lx != 0 || Ly != 0)
	{
		// find angle 
		theta = atan2((double)Ly, (double)Lx) + PI;
		// normalize
		if(theta<0.0) { theta = 0.0; }
		norm = (int)floor((theta/(2*PI))*15.0);
		norm -= 4; 
		if(norm<0) { norm += 16; }
		state = state & discDirections[norm & 0x0F];
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
	int state = 0xFF;

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

void drawMiniKeypad(int player, int frame[])
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