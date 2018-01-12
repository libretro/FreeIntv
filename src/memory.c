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

// unsigned int Memory[0x10000];

void writeMem(int adr, int val) // Write (should handle hooks/alias)
{
	if(adr>=0x100 & adr<=0x1FF)
	{
		val = val & 0xFF;
	}

	Memory[adr & 0xFFFF] = val & 0xFFFF;

	//STIC Alias
	if((adr>=0x4000 && adr<=0x403F) || (adr>=0x8000 && adr<=0x803F) || (adr>=0xC000 && adr<=0xC03F))
	{ 
		Memory[adr & 0x3FFF] = val & 0xFFFF;
	}
	
	//GRAM Alias
	if((adr>=0x7800 && adr<=0x7FFF) || (adr>=0xB800 && adr<=0xBFFF) || (adr>=0xF800 && adr<=0xFFFF))
	{ 
		Memory[adr & 0x3FFF] = val & 0xFFFF;
	}

	//PSG Registers
	if(adr>=0x01F0 && adr<=0x1FD)
	{
		PSGNotify(adr, val);
	}
	
	if(VBlank1>0)
	{
		// STIC Display Enable
		if(adr==0x20 || adr==0x4020 || adr==0x8020 || adr==0xC020)
		{
			DisplayEnabled = 1;
		}
		// STIC Mode Select 
		if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
		{
			STICmode = 0;
		}
	}
}
int readMem(int adr) // Read (should handle hooks/alias)
{
	int val = Memory[adr & 0xFFFF];

	if(adr>=0x100 & adr<=0x1FF)
	{
		val = val & 0xFF;
	}

	if(VBlank1>0)
	{
		if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
		{
			STICmode = 1;
		}
	}
	return val;
}

void MemoryInit()
{
	int i;
	for(i=0x0000; i<=0x003F; i++) { Memory[i] = 0x3FFF; } // STIC Registers
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