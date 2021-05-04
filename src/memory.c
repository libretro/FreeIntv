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
#include "memory.h"
#include "stic.h"
#include "psg.h"

#include <stdio.h>

unsigned int Memory[0x10000];

void writeMem(int adr, int val) // Write (should handle hooks/alias)
{
    val = val & 0xFFFF;
    
    if(adr>=0x100 && adr<=0x1FF)
    {
        val = val & 0xFF;
    }
    
    // STIC Display Enable
    if(adr==0x20 || adr==0x4020 || adr==0x8020 || adr==0xC020)
    {
        if (stic_reg != 0)
            DisplayEnabled = 1;
    }
    // STIC Mode Select
    if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
    {
        if (stic_reg != 0)
            STICMode = 0;
    }
    //STIC Alias
    if((adr>=0x0000 && adr<=0x003F) || (adr>=0x4000 && adr<=0x403F) || (adr>=0x8000 && adr<=0x803F) || (adr>=0xC000 && adr<=0xC03F))
    {
        if (stic_reg != 0)
            Memory[adr & 0x3F] = val;
        return;
    }
    //GRAM Alias
    if((adr>=0x3800 && adr<=0x3fff) || (adr>=0x7800 && adr<=0x7FFF) || (adr>=0xB800 && adr<=0xBFFF) || (adr>=0xF800 && adr<=0xFFFF))
    {
        if (stic_gram != 0)
            Memory[adr & 0x39FF] = val;
        return;
    }
    
    Memory[adr & 0xFFFF] = val;
    
    //PSG Registers
    if(adr>=0x01F0 && adr<=0x1FD)
    {
        PSGNotify(adr, val);
    }
}

int readMem(int adr) // Read (should handle hooks/alias)
{
	// It's safe to map ROM over GRAM aliases

	int val = Memory[adr & 0xFFFF];

	if(adr>=0x100 && adr<=0x1FF)
	{
		val = val & 0xFF;
	}

	if(stic_reg != 0)
	{
		if(adr<=0x3F)
		{
			val = Memory[adr] & 0x3FFF; // STIC registers are 14-bits wide
			// STIC register reads are strange, unused bits are always set to 1 
			if(adr<=0x7)               { val = val | 0x3800; } // 0000 - 0007 : 0011 1--- ---- ----
			if(adr>=0x08 && adr<=0x0F) { val = val | 0x3000; } // 0008 - 000F : 0011 ---- ---- ----
			//                                                 // 0010 - 0017 : 00-- ---- ---- ----
			if(adr>=0x18 && adr<=0x1F) { val = val | 0x3C00; } // 0018 - 001F : 0011 11-- ---- ----
			if(adr>=0x20 && adr<=0x27) { val =       0x3FFF; } // 0020 - 0027 : 0011 1111 1111 1111
			if(adr>=0x28 && adr<=0x2C) { val = val | 0x3FF0; } // 0028 - 002C : 0011 1111 1111 ----
			if(adr>=0x2D && adr<=0x2F) { val =       0x3FFF; } // 002D - 002F : 0011 1111 1111 1111
			if(adr>=0x30 && adr<=0x31) { val = val | 0x3FF8; } // 0030 - 0031 : 0011 1111 1111 1---
			if(adr>=0x32             ) { val = val | 0x3FFC; } // 0032        : 0011 1111 1111 11--
			if(adr>=0x33 && adr<=0x3F) { val =       0x3FFF; } // 0030 - 0031 : 0011 1111 1111 1111
		}

		// read sensitive addresses
		if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
		{
			STICMode = 1;
		}
	}
	return val;
}

void MemoryInit()
{
	int i;
	for(i=0x0000; i<=0x0007; i++) { Memory[i] = 0x3800; } // STIC Registers
	for(i=0x0008; i<=0x000F; i++) { Memory[i] = 0x3000; }
	for(i=0x0010; i<=0x0017; i++) { Memory[i] = 0x0000; }
	for(i=0x0018; i<=0x001F; i++) { Memory[i] = 0x3C00; }
	for(i=0x0020; i<=0x003F; i++) { Memory[i] = 0x3FFF; }
	for(i=0x0028; i<=0x002C; i++) { Memory[i] = 0x3FF0; }
	Memory[0x30] = 0x3FF8;
	Memory[0x31] = 0x3FF8;
	Memory[0x32] = 0x3FFC;
	for(i=0x0040; i<=0x007F; i++) { Memory[i] = 0x0000; }
	for(i=0x0080; i<=0x00FF; i++) { Memory[i] = 0xFFFF; }
	for(i=0x0100; i<=0x035F; i++) { Memory[i] = 0x0000; } // Scratch, PSG (1F0-1FF), System Ram
	for(i=0x0360; i<=0x0FFF; i++) { Memory[i] = 0xFFFF; }
	for(i=0x1000; i<=0x1FFF; i++) { Memory[i] = 0x0000; } // EXEC ROM
	for(i=0x2000; i<=0x2FFF; i++) { Memory[i] = 0xFFFF; }
	for(i=0x3000; i<=0x3FFF; i++) { Memory[i] = 0x0000; } // GROM, GRAM
	for(i=0x4000; i<=0x4FFF; i++) { Memory[i] = 0xFFFF; }
	for(i=0x5000; i<=0x5FFF; i++) { Memory[i] = 0x0000; }
	for(i=0x6000; i<=0xFFFF; i++) { Memory[i] = 0xFFFF; }
	Memory[0x1FE] = 0xFF; // Controller R
	Memory[0x1FF] = 0xFF; // Controller L
}
