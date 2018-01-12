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
#include <stdio.h>
#include <stdlib.h>
#include "intv.h"
#include "memory.h"
#include "cp1610.h"
#include "stic.h"
#include "psg.h"
#include "controller.h"
#include "cart.h"

void LoadGame(char* path) // load cart rom //
{
	if(LoadCart(path))
	{
		frame[0] = 0x00FF00;
	}
	else
	{
		frame[0] = 0xFF0000;
	}
}

void loadExec(char* path)
{
	// EXEC lives at 0x1000-0x1FFF
	int i;
	unsigned char word[2];
	FILE *fp;
	if((fp = fopen(path,"rb"))!=NULL)
	{
		for(i=0x1000; i<=0x1FFF; i++)
		{
			fread(word,sizeof(word),1,fp);
			Memory[i] = (word[0]<<8) | word[1];
		}

		fclose(fp);
		frame[2] = 0x00FF00;
	}
	else
	{
		frame[2] = 0xFF0000;
	}
}

void loadGrom(char* path)
{
	// GROM lives at 0x3000-0x37FF
	int i;
	unsigned char word[1];
	FILE *fp;
	if((fp = fopen(path,"rb"))!=NULL)
	{
		for(i=0x3000; i<=0x37FF; i++)
		{
			fread(word,sizeof(word),1,fp);
			Memory[i] = word[0];
		}

		fclose(fp);
		frame[4] = 0x00FF00;
	}
	else
	{
		frame[4] = 0xFF0000;
	}
}

void Reset()
{
	SR1 = 0;
	CP1610Init();
	MemoryInit();
	STICReset();
	PSGInit();
}

void Run()
{
	// run for one frame 
	// exec will call drawFrame for us only when needed
	while(exec()) { }
}

int exec() // Run one instruction 
{
	int ticks = CP1610Tick(0); // Tick CP-1610 CPU, runs one instruction, returns used cycles
	Cycles = Cycles + ticks; 

	if(ticks==0)
	{
		// Halt Instruction found! //
		printf("\nHALT! at %i\n", Cycles);
		exit(0);
		return 0;
	}

	// Tick PSG
	PSGTick(ticks);

	if(Cycles>=14934) // STIC generates an interput every 14934 cycles
	{
		Cycles = Cycles - 14934;
		SR1 = 3791; // hold  SR1 output low for 3791 cycles
		DisplayEnabled = 0;
		VBlank1 = 2900 - Cycles;
		VBlank2 = 3792 + VBlank1;
	}
	if(SR1>0) 
	{
		SR1 = SR1 - ticks;
		if(SR1<0) { SR1 = 0; }
	}
	if(VBlank1>0) 
	{
		VBlank1 = VBlank1 - ticks;
		if(VBlank1<0) { VBlank1 = 0; }
	}
	if(VBlank2>0)
	{
		VBlank2 = VBlank2 - ticks;
		if(VBlank2<0)
		{
			VBlank2 = 0;
			if(DisplayEnabled==1)
			{
				// Render Frame //
				STICDrawFrame();
			}
			return 0;
		}
	}
	return 1;
}
