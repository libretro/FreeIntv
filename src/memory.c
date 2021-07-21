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

#include "cp1610.h"
#include "memory.h"
#include "stic.h"
#include "psg.h"

#include <stdio.h>

int stic_and[64] = {
    0x07ff, 0x07ff, 0x07ff, 0x07ff, 0x07ff, 0x07ff, 0x07ff, 0x07ff,
    0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x03fe, 0x03fd, 0x03fb, 0x03f7, 0x03ef, 0x03df, 0x03bf, 0x037f,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x000f, 0x000f, 0x000f, 0x000f, 0x000f, 0x0000, 0x0000, 0x0000,
    0x0007, 0x0007, 0x0003, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

int stic_or[64] = {
    0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800,
    0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x3c00, 0x3c00, 0x3c00, 0x3c00, 0x3c00, 0x3c00, 0x3c00, 0x3c00,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x3ff0, 0x3ff0, 0x3ff0, 0x3ff0, 0x3ff0, 0x3fff, 0x3fff, 0x3fff,
    0x3ff8, 0x3ff8, 0x3ffc, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
    0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff, 0x3fff,
};

void writeMem(int adr, int val) // Write (should handle hooks/alias)
{
    val &= 0xFFFF;
    adr &= 0xFFFF;
    
    // Ignore writes to protected ROM spaces
    // Note: B17 Bomber manages to write on EXEC ROM (it will crash if unprotected)
    switch (adr >> 11) {
        case 0x02:  /* Exec ROM */
        case 0x03:
        case 0x06:  /* GROM */
        case 0x0a:  /* 5000-57FF */
        case 0x0b:  /* 5800-5FFF */
        case 0x0c:  /* 6000-67FF */
        case 0x0d:  /* 6800-6FFF */
            return; /* Ignore */
        case 0x07:  /* GRAM */
        case 0x0f:
        case 0x17:
        case 0x1f:
            if (CTX(stic_gram) != 0)
                CTX(Memory)[adr & 0x39FF] = val;
            return;
    }
    if(adr>=0x100 && adr<=0x1FF)
    {
        val = val & 0xFF;
        CTX(Memory)[adr] = val;
        //PSG Registers
        if(adr>=0x01F0 && adr<=0x1FD)
        {
            PSGNotify(adr, val);
        }
        return;
    }
    
    // STIC Display Enable
    if(adr==0x20 || adr==0x4020 || adr==0x8020 || adr==0xC020)
    {
        if (CTX(stic_reg) != 0)
            CTX(DisplayEnabled) = 1;
    }
    // STIC Mode Select
    if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
    {
        if (CTX(stic_reg) != 0)
            CTX(STICMode) = 0;
    }
    //STIC Alias
    if((adr>=0x0000 && adr<=0x003F) || (adr>=0x4000 && adr<=0x403F) || (adr>=0x8000 && adr<=0x803F) || (adr>=0xC000 && adr<=0xC03F))
    {
        if (CTX(stic_reg) != 0)
            CTX(Memory)[adr & 0x3F] = (val & stic_and[adr & 0x3f]) | stic_or[adr & 0x3f];;
        return;
    }
    
    CTX(Memory)[adr] = val;
    
}

int readMem(int adr) // Read (should handle hooks/alias)
{
	// It's safe to map ROM over GRAM aliases

    int val;
    
    adr &= 0xffff;
    val = CTX(Memory)[adr];

	if(adr>=0x100 && adr<=0x1FF)
	{
		val = val & 0xFF;
	}

	if(CTX(stic_reg) != 0)
	{
		if(adr<=0x3F)
		{
            val = (CTX(Memory)[adr] & stic_and[adr]) | stic_or[adr];
		}

		// read sensitive addresses
		if(adr==0x21 || adr==0x4021 || adr==0x8021 || adr==0xC021)
		{
			CTX(STICMode) = 1;
		}
	}
	return val;
}

void MemoryInit()
{
	int i;
	for(i=0x0000; i<=0x0007; i++) { CTX(Memory)[i] = 0x3800; } // STIC Registers
	for(i=0x0008; i<=0x000F; i++) { CTX(Memory)[i] = 0x3000; }
	for(i=0x0010; i<=0x0017; i++) { CTX(Memory)[i] = 0x0000; }
	for(i=0x0018; i<=0x001F; i++) { CTX(Memory)[i] = 0x3C00; }
	for(i=0x0020; i<=0x003F; i++) { CTX(Memory)[i] = 0x3FFF; }
	for(i=0x0028; i<=0x002C; i++) { CTX(Memory)[i] = 0x3FF0; }
	CTX(Memory)[0x30] = 0x3FF8;
	CTX(Memory)[0x31] = 0x3FF8;
	CTX(Memory)[0x32] = 0x3FFC;
	for(i=0x0040; i<=0x007F; i++) { CTX(Memory)[i] = 0x0000; }
	for(i=0x0080; i<=0x00FF; i++) { CTX(Memory)[i] = 0xFFFF; }
	for(i=0x0100; i<=0x035F; i++) { CTX(Memory)[i] = 0x0000; } // Scratch, PSG (1F0-1FF), System Ram
	for(i=0x0360; i<=0x0FFF; i++) { CTX(Memory)[i] = 0xFFFF; }
	for(i=0x1000; i<=0x1FFF; i++) { CTX(Memory)[i] = 0x0000; } // EXEC ROM
	for(i=0x2000; i<=0x2FFF; i++) { CTX(Memory)[i] = 0xFFFF; }
	for(i=0x3000; i<=0x3FFF; i++) { CTX(Memory)[i] = 0x0000; } // GROM, GRAM
	for(i=0x4000; i<=0x4FFF; i++) { CTX(Memory)[i] = 0xFFFF; }
	for(i=0x5000; i<=0x5FFF; i++) { CTX(Memory)[i] = 0x0000; }
	for(i=0x6000; i<=0xFFFF; i++) { CTX(Memory)[i] = 0xFFFF; }
	CTX(Memory)[0x1FE] = 0xFF; // Controller R
	CTX(Memory)[0x1FF] = 0xFF; // Controller L
}
