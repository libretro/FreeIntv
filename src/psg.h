#ifndef PSG_H
#define PSG_H
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
#include <stdint.h>

// Circular Buffer holds up to two frames:
int16_t PSGBuffer[7467]; // 14934 cpu cycles/frame ; 3733.5 psg cycles/frame
int PSGBufferPos; // points to next location in output buffer
int PSGBufferSize;

void PSGInit(); 
void PSGTick(int ticks); // ticks PSG some number of cpu cycles 
void PSGNotify(int adr, int val); // updates PSG on register change


#endif