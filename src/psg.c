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

#include <string.h>
#include <stdint.h>
#include "psg.h"
#include "memory.h"

//int PSGBufferSize = 7467;
//int16_t PSGBuffer[7467]; // 14934 cpu cycles/frame ; 3733.5 psg cycles/frame ...
//int PSGBufferPos; // points to next location in output buffer

int Volume[16] = { 0, 92, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192, 10922 };

int Ticks; // CPU cycles not yet processed

int CountA; // countdowns for tone generators
int CountB; // used to modulate square-wave 
int CountC; // according to Channel Period
int CountN; // countdown for noise generator
int CountE; // countdown for envelope generator

int OutA; // outputs for each tone generator
int OutB;
int OutC;
int OutN;  // Noise generator output
int OutE;  // Envelope generator output

int ChA; // Channel Period from PSG Registers
int ChB; 
int ChC; 
           
int VolA; // volume levels assigned to each Channel from PSG Registers
int VolB; 
int VolC; 
           
int NoiseP; // Noise Period
int NoiseA; // is Noise enabled for this channel
int NoiseB; // 0 - enabled 
int NoiseC;	// 1 - disabled

           
int ToneA; // is Tone enabled for this channel
int ToneB; // 0 - enabled
int ToneC; // 1 - disabled
           
int EnvP;    // Envelope Period 
int EnvFlags; // Envelope Type
int EnvA;    // Envelope Shifts for Channels (6-bit variations only)
int EnvB; 
int EnvC; 
int StepE; // 1, 0, -1 -- Direction to Step Envelope at end of countdown

int EnvContinue; // Flags from Envelope Type
int EnvAttack;
int EnvAlternate;
int EnvHold;

void readRegisters()
{
	ChA = (Memory[0x01F0] & 0xFF) | ((Memory[0x1F4] & 0x0F)<<8);
	ChB = (Memory[0x01F1] & 0xFF) | ((Memory[0x1F5] & 0x0F)<<8);
	ChC = (Memory[0x01F2] & 0xFF) | ((Memory[0x1F6] & 0x0F)<<8);
 
	VolA = Memory[0x01FB] & 0x0F;
	VolB = Memory[0x01FC] & 0x0F;
	VolC = Memory[0x01FD] & 0x0F;

	NoiseP = (Memory[0x01F9] & 0x1F)<<1;
	NoiseA = (Memory[0x01F8]>>3) & 0x01;
	NoiseB = (Memory[0x01F8]>>4) & 0x01;
	NoiseC = (Memory[0x01F8]>>5) & 0x01;

	ToneA = Memory[0x01F8] & 0x01;
	ToneB = (Memory[0x01F8]>>1) & 0x01;
	ToneC = (Memory[0x01F8]>>2) & 0x01;

	EnvP = ((Memory[0x01F3] & 0xFF) | ((Memory[0x1F7] & 0xFF)<<8))<<1;
	EnvFlags = Memory[0x01FA] & 0x0F;
	EnvA = (Memory[0x01FB]>>4) & 0x03;
	EnvB = (Memory[0x01FC]>>4) & 0x03;
	EnvC = (Memory[0x01FD]>>4) & 0x03;

	ChA = ChA + (0x1000 * (ChA==0)); // a Channel Period value of 0 
	ChB = ChB + (0x1000 * (ChB==0)); // indicates a value of 0x1000
	ChC = ChC + (0x1000 * (ChC==0));
	// a Noise Period of 0 indicates a period of 0x40 
	NoiseP = NoiseP + (0x40 * (NoiseP==0));
	// an Envelope Period of 0 indicates a period of 0x20000
	EnvP = EnvP + (0x20000 * (EnvP==0));
	
	// Envelope Flags
	EnvContinue = (EnvFlags>>3) & 0x01;
	EnvAttack = (EnvFlags>>2) & 0x01;
	EnvAlternate = (EnvFlags>>1) & 0x01;
	EnvHold = EnvFlags & 0x01;
}

void PSGInit()
{
	PSGBufferSize = 7467; // set in psg.h

	OutA = 0; // tone generator outputs
	OutB = 0;
	OutC = 0;
	OutN = 0x14000; // noise output
	OutE = 0; // envelope output
	CountA = 0; // tone generator countdowns
	CountB = 0;
	CountC = 0;
	CountN = 0; // noise generator countdown
	CountE = 0; // envelope countdown
	readRegisters();
}

void PSGNotify(int adr, int val) // PSG Registers Modified 0x01F0-0x1FD (called from writeMem)
{
	readRegisters();

	if(adr==0x1F0 || adr==0x1F4) { CountA = 0; }
	if(adr==0x1F1 || adr==0x1F5) { CountB = 0; }
	if(adr==0x1F2 || adr==0x1F6) { CountC = 0; }

	if(adr>=0x1FB && adr<=0x1FD) { Memory[adr] &= 0x001F; }

	// Envelope properties Trigger (write only register)
	if (adr==0x1FA)  
	{ 
		CountE = EnvP;
		StepE = 0;

		if (EnvAttack) // attack __/|/|/|___
		{
			OutE = 0;
			StepE = 1;
		}
		else
		{
			OutE = 15;
			StepE = -1;
		}
	}
}

void PSGTick(int ticks) // adds 1 sound sample per 4 cpu cycles to the buffer
{
	int16_t sample;
	int a, b, c;

	Ticks = Ticks + ticks;

	while(Ticks > 3)
	{
		Ticks = Ticks - 4;

		CountA--;
		CountB--;
		CountC--;
		CountN--;
		CountE--;

		/* ************** Generate Sample ************** */

		OutA = OutA ^ (CountA<=0); // Tone Generators
		OutB = OutB ^ (CountB<=0); 
		OutC = OutC ^ (CountC<=0); 

		// http://spatula-city.org/~im14u2c/intv/jzintv-1.0-beta3/doc/programming/psg.txt
		if(CountE==0) // Envelope Generator 
		{
			CountE = EnvP; // reset countdown
			OutE = OutE + StepE; // step up, step down, or hold

			if(StepE != 0 && (OutE>15 || OutE<0)) // we've reached the top or bottom
			{
				if(EnvHold)
				{ 
					StepE = 0; // stop changing (hold volume)
					if(EnvAlternate) // alternate & hold  1011 1111
					{
						OutE = 15 * (EnvAttack==0);
					}
					else // hold at 0 (1001) or 15 (1101) 
					{
						OutE = 15 * (EnvAttack==1);
					}
				}
				else
				{
					if(EnvAlternate) // triange waves__/\/\/\__ 1010  \/\/\/\___ 1110
					{
						StepE = StepE * -1;    // Swap step direction
						OutE = (OutE + StepE) & 0x0F;
					}
					else // saw-tooth waves __|\|\|\__ 1000 ___/|/|/|___ 1100
					{
						OutE = 15 * (EnvAttack==0);
					}
				}
				// Anything without continue flag set holds at 0
				if(EnvContinue==0)
				{
					OutE = 0;
					StepE = 0;
				}
			}
		}

		// http://wiki.intellivision.us/index.php?title=PSG
		// noise = (noise >> 1) ^ ((noise & 1) ? 0x14000 : 0);
		if(CountN<=0)
		{
			CountN = NoiseP;
			OutN = (OutN >> 1) ^ ((OutN & 1) * 0x14000); // Noise Generator
		}

		// http://wiki.intellivision.us/index.php?title=PSG
		// channel_output = (noise_enable OR noise_generator_output) AND (tone_enable OR tone_generator_output)
		a = (NoiseA | (OutN & 1)) & (ToneA | OutA); // Generate Sample for each channel
		b = (NoiseB | (OutN & 1)) & (ToneB | OutB);
		c = (NoiseC | (OutN & 1)) & (ToneC | OutC);

		// Adjust amplitude (Volume / Envelope)
		a = a * ( (Volume[VolA] * (EnvA==0)) | (Volume[OutE] * (EnvA!=0)) );
		b = b * ( (Volume[VolB] * (EnvB==0)) | (Volume[OutE] * (EnvB!=0)) );
		c = c * ( (Volume[VolC] * (EnvC==0)) | (Volume[OutE] * (EnvC!=0)) );

		sample = a + b + c;

		/* ********************************************* */

		CountA += ChA * (CountA<=0); // reset countdowns when they reach 0 
		CountB += ChB * (CountB<=0);
		CountC += ChC * (CountC<=0);

		PSGBuffer[PSGBufferPos] = sample; // write sample to buffer
		
		PSGBufferPos++;
		PSGBufferPos = PSGBufferPos * (PSGBufferPos < 7467); // wrap to beginning
	}
}