#ifndef STIC_H
#define STIC_H
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

extern unsigned int STICMode; // 0-foreground/background, 1-color stack/color squares 

extern int VBlank1; // counter for VBlank period 1
extern int VBlank2; // counter for VBlank period 2
extern int Cycles; // number of cycles since last STIC interput
extern int DisplayEnabled; // determines if frame should be updated or not
extern int VerticalDelay;

extern unsigned int frame[352*224]; // frame buffer

void STICDrawFrame(void);
void STICReset(void);

#endif
