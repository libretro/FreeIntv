#ifndef CP1610_H
#define CP1610_H
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

struct CP1610serialized {
    int Flag_DoubleByteData;
    int Flag_InteruptEnable;
    int Flag_Carry;
    int Flag_Sign;
    int Flag_Zero;
    int Flag_Overflow;
    unsigned int R[8];
};

void CP1610Serialize(struct CP1610serialized *);
void CP1610Unserialize(const struct CP1610serialized *);

void CP1610Init(void); // Adds opcodes to lookup tables

void CP1610Reset(void); // reset cpu

int CP1610Tick(int debug); // execute a single instruction, return cycles used

#endif
