#ifndef PSG_H
#define PSG_H
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
#include <stdint.h>

// Circular Buffer holds up to two frames:
extern int16_t PSGBuffer[7467]; // 14934 cpu cycles/frame ; 3733.5 psg cycles/frame
extern int PSGBufferPos; // points to next location in output buffer
extern int PSGBufferSize;

struct PSGserialized {
    int PSGBufferSize;
    int16_t PSGBuffer[7467];
    int PSGBufferPos;
    
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
    
    int NoiseP; // Noise Period
    
    int EnvP;    // Envelope Period
    int StepE; // 1, 0, -1 -- Direction to Step Envelope at end of countdown
    
    int EnvContinue; // Flags from Envelope Type
    int EnvAttack;
    int EnvAlternate;
    int EnvHold;
};

void PSGSerialize(struct PSGserialized *);
void PSGUnserialize(const struct PSGserialized *);

void PSGInit(void); 
void PSGFrame(void); // Notify New Frame
void PSGTick(int ticks); // ticks PSG some number of cpu cycles 
void PSGNotify(int adr, int val); // updates PSG on register change


#endif
