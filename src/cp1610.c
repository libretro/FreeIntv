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
#include <string.h>
#include "intv.h"
#include "memory.h"

// http://wiki.intellivision.us/index.php?title=CP1610#Instruction_Set
// http://spatula-city.org/~im14u2c/chips/GICP1600.pdf
// ftp://bitsavers.informatik.uni-stuttgart.de/components/gi/CP1600/CP-1600_Microprocessor_Users_Manual_May75.pdf

int (*OpCodes[0x400])(int);
int Interuptable[0x400];
const char *Nmemonic[0x400];

unsigned int R[8] = {0, 0, 0, 0, 0, 0, 0x02F1, 0x1000}; // Registers R0-R7

const int PC = 7; // const Program Counter (R7)
const int SP = 6; // const Stack Pointer (R6)

int InstructionRegister = 0; // four external lines?

int Flag_DoubleByteData = 0;
int Flag_InteruptEnable = 0;
int Flag_Carry = 0;
int Flag_Sign = 0;
int Flag_Zero = 0;
int Flag_Overflow = 0;

void CP1610Reset()
{
	Flag_DoubleByteData = 0;
	Flag_InteruptEnable = 0;
	Flag_Carry = 0;
	Flag_Sign = 0;
	Flag_Zero = 0;
	Flag_Overflow = 0;
	R[0] = R[1] = R[2] = R[3] = R[4] = R[5] = 0;
	R[SP] = 0x02F1; // Stack is at System Ram 0x02F1-0x0318
	R[PC] = 0x1000; // EXEC entry point
}

int readIndirect(int reg) // Read Indirect, handle SDBD, update autoincriment registers
{
	int val = 0;
	int adr = 0;

	if(reg==6) { R[reg] = R[reg] - 1; } // decriment R6 (SP) before read
	adr = R[reg];
	
	if(Flag_DoubleByteData==0)
	{
		val = readMem(adr);
	}
	else
	{
		if(reg==4 || reg==5 || reg==7)
		{
			R[reg] = (R[reg]+1) & 0xFFFF;
		}
		val = ((readMem(adr+1) & 0xFF)<<8) | (readMem(adr) & 0xFF);
	}
	if(reg==4 || reg==5 || reg==7) // autoincrement registers R4-R7 excluding SP (R6)
	{
		R[reg] = (R[reg]+1) & 0xFFFF;
	} 
	return val;
}

int writeIndirect(int reg, int val)
{
	int adr = R[reg];
	writeMem(adr, val);
	if(reg>=4) // autoincrement registers R4-R7
	{
		R[reg] = (R[reg]+1) & 0xFFFF;
	}
}

int readOperand()
{
	int val = readMem(R[PC]);
	R[PC]++;
	return val;
}

int readOperandIndirect()
{
	int adr = readMem(R[PC]);
	int val = readMem(adr);
	R[PC]++;
	return val;
}

int SetFlagsCSZ(int reg)
{
	Flag_Carry = (R[reg] & 0x10000) != 0;
	SetFlagsSZ(reg);
}
int SetFlagsSZ(int reg)
{
	R[reg] = R[reg] & 0xFFFF;
	Flag_Sign = (R[reg] & 0x8000)!=0;
	Flag_Zero = R[reg]==0;
}
int AddSetSZOC(int A, int B)
{
	int signa = A & 0x8000;
	int signb = B & 0x8000;
	int result = (A+B);
	int signr =  result & 0x8000;
	
	Flag_Overflow = (signa==signb && signa!=signr) ? 1 : 0;
	Flag_Carry = (result & 0x10000) != 0;
	
	result = result & 0xFFFF;
	
	Flag_Sign = (result & 0x8000)!=0;
	Flag_Zero = result==0;
	return result;
}
int SubSetOC(int A, int B)
{
	int signa = A & 0x8000;
	int signb = B & 0x8000;
	int result = (A + (B ^ 0xFFFF) + 1); // A - B using 1's compliment;
	int signr =  result & 0x8000;
	Flag_Carry = (result & 0x10000)!=0;
	Flag_Overflow = (signb==signr && signa!=signb) ? 1 : 0;
	return result & 0xFFFF;
}

int CP1610Tick(int debug)
{
	// execute one instruction //
	int sdbd = Flag_DoubleByteData;
	
	unsigned int instruction = readMem(R[PC]);
	
	int ticks = 0;

	if(instruction > 0x03FF)
	{
		// bad OpCode, Halt //
		return 0;
	}
	
	// DEBUG
/*
	printf("[%03x] %04x %04x %04x %04x %04x %04x %04x %04x   ", instruction, R[0], R[1], R[2], R[3], R[4], R[5], R[6], R[7]);
	printf("%s ", Nmemonic[instruction]);
	if(Flag_Sign) { printf("S"); } else { printf("-"); }
	if(Flag_Carry) { printf("C"); } else { printf("-"); }
	if(Flag_Overflow) { printf("O"); } else { printf("-"); }
	if(Flag_Zero) { printf("Z"); } else { printf("-"); }
	if(Flag_InteruptEnable) { printf("I"); } else { printf("-"); }
	if(Flag_DoubleByteData) { printf("D"); } else { printf("-"); }
	printf("\n");
*/
	R[PC]++; // point PC/R7 at operand/next address
	
	ticks = OpCodes[instruction](instruction); // execute instruction

	if(sdbd==1) { Flag_DoubleByteData = 0; } // reset SDBD

	// check interupt request
	if(Flag_InteruptEnable == 1 && SR1>0)
	{
		if(Interuptable[instruction])
		{
			// Take VBlank Interupt //
			SR1 = 0;
			writeIndirect(SP, R[PC]); // push PC...
			R[PC] = 0x1004; // Jump
		}
	}

	return ticks;
}

int HLT(int v)  { return 0; } // Halt
int SDBD(int v) { Flag_DoubleByteData = 1; return 4; } // Set Double Byte Data
int EIS(int v)  { Flag_InteruptEnable = 1; return 4; } // Enable Interrupt System
int DIS(int v)  { Flag_InteruptEnable = 0; return 4; } // Disable Interrupt System
int Jump(int v)
{ 
	// J, JE, JD, JSR, JSRE, JSRD, CALL
	// 0000:0000:0000:0100  0000:00rr:aaaa:aaff  0000:00aa:aaaa:aaaa
	int decle2 = readOperand();
	int decle3 = readOperand() & 0x3FF;
	int reg = (decle2>>8) & 0x03; // 0-R4, 1-R5, 2-R6, 3-don't store return address
	int adr = (((decle2>>2) & 0x3F)<<10) | decle3;
	int ff = decle2 & 0x03; // Interrupt flag (0-no change, 1-set, 2-clear, 3-undefined)
	if(reg!=3)
	{
		reg = reg + 4;
		R[reg] = R[PC]; // store return address (PC already advanced to PC+3)
	}
	if(ff==1) { Flag_InteruptEnable = 1; } // set Interupt flag
	if(ff==2) { Flag_InteruptEnable = 0; } // clear Interrupt flag
	R[PC] = adr; // Jump
	return 13;
}
int TCI(int v)  { return 4; } // Terminate Current Interrupt (not used)
int CLRC(int v) { Flag_Carry = 0; return 4; } // Clear Carry
int SETC(int v) { Flag_Carry = 1; return 4; } // Set Carry

int INCR(int v) // Increment Register
{
	int reg = v & 0x07;
	R[reg] = (R[reg]+1) & 0xFFFF;
	SetFlagsSZ(reg);
	return 6;
}
int DECR(int v) // Decrement Register
{
	int reg = v & 0x07;
	//R[reg] = (R[reg]-1) & 0xFFFF;
	R[reg] = (R[reg] + 0xFFFF) & 0xFFFF;
	SetFlagsSZ(reg);
	return 6;
}
int COMR(int v) // Complement Register (One's Compliment)
{
	int reg = v & 0x07;
	R[reg] = R[reg] ^ 0xFFFF;
	SetFlagsSZ(reg);
	return 6;
}
int NEGR(int v) // Negate Register (Two's Compliment)
{
	int reg = v & 0x07;
	R[reg] = AddSetSZOC((R[reg] ^ 0xFFFF), 1);
	return 6;
}
int ADCR(int v) // Add Carry to Register
{
	int reg = v & 0x07;
	R[reg] = AddSetSZOC(R[reg], Flag_Carry);
	return 6;
}
int GSWD(int v) // Get the Status Word szoc:0000:szoc:0000
{
	int reg = v & 0x03;
	unsigned int szoc = (Flag_Sign<<3) | (Flag_Zero<<2) | (Flag_Overflow<<1) | Flag_Carry;
	R[reg] = (szoc<<12) | (szoc<<4);
	return 6;
}
int NOP(int v) { return 6; } // No Operation
int SIN(int v) { return 6; } // Software Interrupt (not used)

int RSWD(int v) // Return Status Word szoc:0000
{
	int reg = v & 0x07;
	unsigned int szoc = R[reg]>>4;
	Flag_Sign = (szoc>>3) & 1;
	Flag_Zero = (szoc>>2) & 1;
	Flag_Overflow = (szoc>>1) & 1;
	Flag_Carry = szoc & 1;
	return 6;
}
int SWAP(int v) // Swap 0000:0trr
{
	int reg = v & 0x03;
	int times = (v>>2) & 1;
	int upper = (R[reg]>>8) & 0xFF;
	int lower = R[reg] & 0xFF;
	if(times==0) // single swap
	{
		R[reg] = (lower<<8) | upper;
		Flag_Sign = (R[reg]>>7) & 1;
		Flag_Zero = R[reg]==0;
		return 6;
	}
	else // double swap
	{
		R[reg] = (lower<<8) | lower;
		Flag_Sign = (R[reg]>>7) & 1;
		Flag_Zero = R[reg]==0;
		return 8;
	}
}
int SLL(int v) // Shift Logical Left 0000:1drr
{
	int reg = v & 0x03;
	int dist = ((v>>2) & 1)+1;
	R[reg] = R[reg]<<dist;
	SetFlagsSZ(reg);
	return 6+(2*(dist-1)); // 6 <<1 or 8 <<2
}
int RLC(int v) // Rotate Left Through Carry
{
	int reg = v & 0x03;
	int times = ((v>>2) & 1);
	int bit15 = (R[reg]>>15) & 1;
	int bit14 = (R[reg]>>14) & 1;
	if(times==0) // Single rotate
	{
		R[reg] = R[reg] << 1;
		R[reg] = R[reg] | Flag_Carry;
		Flag_Carry = bit15;
	}
	else // Double rotate
	{
		R[reg] = R[reg] << 2;
		R[reg] = R[reg] | ((Flag_Carry << 1) | Flag_Overflow);
		Flag_Carry = bit15;
		Flag_Overflow = bit14;
	}
	SetFlagsSZ(reg);
	return 6+(2*times); // 6 single or 8 double
}
int SLLC(int v) // Shift Logical Left through Carry
{
	// CP-1600 Manual says to use O as bit 16 and C as bit 17
	// on a double shift, and C as bit 16 on a single shift.
	// the wiki.intellivision.us site has the flags reversed
	// on double shift, setting C to old bit 15 in both cases.
	// The wiki method seems to be correct
	int reg = v & 0x03;
	int dist = ((v>>2) & 1)+1;
	int bit15 = (R[reg]>>15) & 1;
	int bit14 = (R[reg]>>14) & 1;
	R[reg] = (R[reg]<<dist);
	Flag_Carry = bit15;			
	if(dist==2)
	{
		Flag_Overflow = bit14; // wiki.intellivision.us method 
		//Flag_Carry = bit14; // CP-1600 Manual method
		//Flag_Overflow = bit15; // CP-1600 Manual method
	}
	SetFlagsSZ(reg);
	return 6+(2*(dist-1)); // 6 <<1 or 8 <<2
}
int SLR(int v) // Shift Logical Right
{
	int reg = v & 0x03;
	int dist = ((v>>2) & 1)+1;
	R[reg] = R[reg]>>dist;
	Flag_Sign = (R[reg]>>7) & 1;
	Flag_Zero = R[reg]==0;
	return 6+(2*(dist-1)); // 6 <<1 or 8 <<2
}
int SAR(int v) // Shift Arithmetic Right
{
	int reg = v & 0x03;
	int dist = ((v>>2) & 1)+1;
	int bit15 = (R[reg]>>15) & 1;

	R[reg] = R[reg]>>dist;
	if(dist==1)
	{
		R[reg] = R[reg] | (bit15<<15);
	}
	else
	{
		R[reg] = R[reg] | (bit15<<15);
		R[reg] = R[reg] | (bit15<<14); // CP-1600 manual says "sign bit copied to high bits"
	}
	Flag_Sign = (R[reg]>>7) & 1;
	Flag_Zero = R[reg]==0;
	return 6+(2*(dist-1)); // 6 <<1 or 8 <<2
}
int RRC(int v) // Rotate Right Through Carry
{
	int reg = v & 0x03;
	int dist = ((v>>2) & 1);
	int bit1 = (R[reg]>>1) & 1;
	int bit0 = R[reg] & 1;

	if(dist==0)
	{
		R[reg] = R[reg]>>1;
		R[reg] = R[reg] | (Flag_Carry<<15);
	}
	else
	{
		R[reg] = R[reg]>>2;
		R[reg] = R[reg] | (Flag_Overflow<<15);
		R[reg] = R[reg] | (Flag_Carry<<14);
		Flag_Overflow = bit1;
	}
	Flag_Carry = bit0;
	Flag_Sign = (R[reg]>>7) & 1;
	Flag_Zero = R[reg]==0;
	return 6+(2*(dist)); // 6 <<1 or 8 <<2
}
int SARC(int v) // Shift Arithmetic Right Through Carry 
{
	int reg = v & 0x03;
	int dist = ((v>>2) & 1)+1;
	int bit15 = (R[reg]>>15) & 1;
	int bit1 = (R[reg]>>1) & 1;
	int bit0 = R[reg] & 1;

	R[reg] = R[reg]>>dist;
	R[reg] = R[reg] | (bit15<<15);
	if(dist==2)
	{
		R[reg] = R[reg] | (bit15<<14); // CP-1600 manual says "sign bit copied to high 2 bits"
		Flag_Overflow = bit1;
	}
	Flag_Carry = bit0;
	Flag_Sign = (R[reg]>>7) & 1;
	Flag_Zero = R[reg]==0;
	return 6+(2*(dist)); // 6 <<1 or 8 <<2
}
int MOVR(int v) // Move Register
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	R[dreg] = R[sreg];
	SetFlagsSZ(dreg);
	if(dreg >= 6) { return 7; }
	return 6;
}
int ADDR(int v) // Add Registers
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	R[dreg] = AddSetSZOC(R[dreg], R[sreg]);
	return 6;
}
int SUBR(int v) // Subtract Registers
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	R[dreg] = SubSetOC(R[dreg], R[sreg]);
	SetFlagsSZ(dreg);
	return 6;
}
int CMPR(int v) // Compare Registers
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	int sign = R[dreg] & 0x8000;
	int res = SubSetOC(R[dreg], R[sreg]);
	Flag_Sign = (res & 0x8000)!=0;
	Flag_Zero = res==0;
	return 6;
}
int ANDR(int v) // And Registers
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	R[dreg] = R[dreg] & R[sreg];
	SetFlagsSZ(dreg);
	return 6;
}
int XORR(int v) // Xor Registers
{
	int sreg = (v >> 3) & 0x7;
	int dreg = v & 0x7;
	R[dreg] = R[dreg] ^ R[sreg];
	SetFlagsSZ(dreg);
	return 6;
}
int Branch(int v) // Branch - B, BC, BOV, BPL, BEQ, BLT, BLE, BUSC, NOPP, BNC, BNOV, BMI, BNEQ, BGE, BGT, BESC, BEXT
{
	//0000:0010:00de:nccc  aaaa:aaaa:aaaa:aaaa
	int offset = readOperand();
	int direction = (v >> 5) & 0x01;
	int ext = (v >> 4) & 0x01;
	int notbit = (v >> 3) & 0x01;
	int condition = v & 0x07;
	int branch = 0;
	if(ext==1) // BEXT - ? I have no idea if this is ever used
	{
		// wiki syas: branch if this value matches the pins EBCA0-EBCA3
		// CP-1600 Manual says:
		// These lines are the buffered outputs from the 4 least significant bits of the
		// instruction register (bits 0-3) and are used to externally select one of sixteen
		// digital states to be sampled by the CPU during the execution of the BEXT
		// (Branch on EXTernal) instruction
		// --- I don't know what is meant by 'instruction register'
		if((InstructionRegister & 0x0F)==(v & 0x0F))
		{
			if(direction==0) { R[PC] = R[PC]+offset; }
			if(direction==1) { R[PC] = R[PC]-offset-1; }
		}
		return 7;
	}
	switch(condition)
	{
		case 0: branch = 1; break; // B, NOPP
		case 1: branch = (Flag_Carry==1); break; // BC, BNC
		case 2: branch = (Flag_Overflow==1); break; // BOV, BNOV
		case 3: branch = (Flag_Sign==0); break; // BPL, BMI
		case 4: branch = (Flag_Zero==1); break; // BEQ, BNEQ
		case 5: branch = (Flag_Sign!=Flag_Overflow); break; // BLT, BGE
		case 6: branch = (Flag_Zero==1)||(Flag_Sign!=Flag_Overflow); break; // BLE, BGT
		case 7: branch = (Flag_Sign!=Flag_Carry); break; // BUSC, BESC
	}
	if(notbit==1) { branch = !branch; }
	if(branch)
	{
		if(direction==0) { R[PC] = R[PC]+offset; }
		if(direction==1) { R[PC] = R[PC]-(offset+1); }
		return 9;
	}
	return 7;
}
int MVO(int v) // Move Out
{
	int reg = v & 0x07;
	int adr = readOperand();
	writeMem(adr, R[reg]);
	return 11;
}
int MVOa(int v) // MVO@ - Move Out Indirect  0000:0010:01aa:asss
{
	// The PSHR Rx instruction is an alias for MVOa Rx, R6
	int areg = (v >> 3) & 0x7;
	int sreg = v & 0x7;
	writeIndirect(areg, R[sreg]);
	return 9;
}
int PSHR(int v) // Push Register
{
	int reg = v & 0x7;
	writeIndirect(SP, R[reg]);
	return 9;
}
int MVOI(int v) // Move Out Immediate 0000:0010:0111:1sss 
{
	return(MVOa(v)); // call indirect copies R[sss] to address in R[PC]
}
int MVI(int v) // 	Move In 0000:0010:1000:0rrr  aaaa:aaaa:aaaa:aaaa
{
	int reg = v & 0x07;
	R[reg] = readOperandIndirect();
	return 10;
}
int MVIa(int v) // Move In Indirect 0000:0010:10aa:addd
{
	int areg = (v >> 3) & 0x7;
	int dreg = v & 0x7;	
	R[dreg] = readIndirect(areg);
	if(Flag_DoubleByteData == 1) { return 10; }
	if(areg==6) { return 11; }
	return 8; 
}
int PULR(int v) // Pull Register 0000:0010:1011:0ddd
{
	int reg = v & 0x7;
	R[reg] = readIndirect(SP);
	return 11; 
}
int MVII(int v) // Move In Immediate (copies operand to register)
{
	// These instructions are only one word, so don't advance PC past operand.
	// Auto incrementing registers will move past the operands automatically.
	// This works exactly like MVI@ with PC as the address register.
	// All nnnI instructions work this way.
	v = v | 0x0038;  // set address register to PC
	return(MVIa(v)); // call indirect
}
int ADD(int v) // Add
{
	int reg = v & 0x07;
	int val = readOperandIndirect();
	R[reg] = AddSetSZOC(R[reg], val);
	return 10;
}
int ADDa(int v) // Add Indirect
{
	int areg = (v >> 3) & 0x07;
	int dreg = v & 0x07;
	int val = readIndirect(areg);
	R[dreg] = AddSetSZOC(R[dreg], val);
	if(Flag_DoubleByteData==1) { return 10; }
	if(dreg==6) { return 11; }
	return 8;
}
int ADDI(int v) // Add Immediate
{
	v = v | 0x0038;  // set address register to PC
	return(ADDa(v)); // call indirect
}
int SUB(int v) // Subtract
{
	int reg = v & 0x07;
	int sign = R[reg] & 0x8000;
	int val = readOperandIndirect();
	R[reg] = SubSetOC(R[reg], val);
	SetFlagsSZ(reg);
	return 10;
}
int SUBa(int v)  // Subtract Indirect
{
	int areg = (v >> 3) & 0x07;
	int dreg = v & 0x07;
	int sign = R[dreg] & 0x8000;
	int val = readIndirect(areg);
	R[dreg] = SubSetOC(R[dreg], val);
	SetFlagsSZ(dreg);
	if(Flag_DoubleByteData==1) { return 10; }
	if(dreg==6) { return 11; }
	return 8;
}
int SUBI(int v) // Subtract Immediate
{
	v = v | 0x0038;  // set address register to PC
	return(SUBa(v)); // call indirect
}
int CMP(int v)
{
	int reg = v & 0x07;
	int val = readOperandIndirect();
	int res = SubSetOC(R[reg], val);
	Flag_Sign = (res & 0x8000)!=0;
	Flag_Zero = res==0;
	return 10;
}
int CMPa(int v)
{
	int areg = (v >> 3) & 0x07;
	int dreg = v & 0x07;
	int val = readIndirect(areg);
	int res = SubSetOC(R[dreg], val);
	Flag_Sign = (res & 0x8000)!=0;
	Flag_Zero = res==0;
	if(Flag_DoubleByteData==1) { return 10; }
	if(dreg==6) { return 11; }
	return 8;
}
int CMPI(int v) // CMP Immediate
{
	v = v | 0x0038;  // set address register to PC
	return(CMPa(v)); // call indirect
}
int AND(int v) // And
{
	int reg = v & 0x07;
	int sign = R[reg] & 0x8000;
	int val = readOperandIndirect();
	R[reg] = R[reg] & val;
	SetFlagsSZ(reg);
	return 10;
}
int ANDa(int v) // And Indirect
{
	int areg = (v >> 3) & 0x07;
	int dreg = v & 0x07;
	int val = readIndirect(areg);
	R[dreg] = R[dreg] & val;
	SetFlagsSZ(dreg);
	if(Flag_DoubleByteData==1) { return 10; }
	if(dreg==6) { return 11; }
	return 8;
}
int ANDI(int v) // And Immediate
{
	v = v | 0x0038;  // set address register to PC
	return(ANDa(v)); // call indirect
}
int XOR(int v) // Xor
{
	int reg = v & 0x07;
	int val = readOperandIndirect();
	R[reg] = R[reg] ^ val;
	SetFlagsSZ(reg);
	return 10;
}
int XORa(int v) // Xor Indirect
{
	int areg = (v >> 3) & 0x07;
	int dreg = v & 0x07;
	int val = readIndirect(areg);
	R[dreg] = R[dreg] ^ val;
	SetFlagsSZ(dreg);
	if(Flag_DoubleByteData==1) { return 10; }
	if(dreg==6) { return 11; }
	return 8;
}
int XORI(int v) // Xor Immediate
{
	v = v | 0x0038;  // set address register to PC
	return(XORa(v)); // call indirect
}

// Make a big table of function pointers for opcodes
// as well as a table of flags so that opcodes can
// be quickly executed and determined to be interuptable 
void addInstruction(int start, int end, int caninterupt, char *name, int (*callback)())
{
	int i;
	for(i=start; i<=end; i++)
	{
		Interuptable[i] = caninterupt;
		Nmemonic[i] = name;
		OpCodes[i] = callback;
	}
}

void CP1610Init()
{
	addInstruction(0x0000, 0x0000, 0, "HLT   ", HLT   );
	addInstruction(0x0001, 0x0001, 0, "SDBD  ", SDBD  );
	addInstruction(0x0002, 0x0002, 0, "EIS   ", EIS   );
	addInstruction(0x0003, 0x0003, 0, "DIS   ", DIS   );
	addInstruction(0x0004, 0x0004, 1, "Jump  ", Jump  ); // J, JE, JD, JSR, JSRE, JSRD, CALL
	addInstruction(0x0005, 0x0005, 0, "TCI   ", TCI   );
	addInstruction(0x0006, 0x0006, 0, "CLRC  ", CLRC  );
	addInstruction(0x0007, 0x0007, 0, "SETC  ", SETC  );
	addInstruction(0x0008, 0x000F, 1, "INCR  ", INCR  );
	addInstruction(0x0010, 0x0017, 1, "DECR  ", DECR  );
	addInstruction(0x0018, 0x001F, 1, "COMR  ", COMR  );
	addInstruction(0x0020, 0x0027, 1, "NEGR  ", NEGR  );
	addInstruction(0x0028, 0x002F, 1, "ADCR  ", ADCR  );
	addInstruction(0x0030, 0x0033, 1, "GSWD  ", GSWD  );
	addInstruction(0x0034, 0x0035, 1, "NOP   ", NOP   );
	addInstruction(0x0036, 0x0037, 1, "SIN   ", SIN   );
	addInstruction(0x0038, 0x003F, 1, "RSWD  ", RSWD  );
	addInstruction(0x0040, 0x0047, 0, "SWAP  ", SWAP  );
	addInstruction(0x0048, 0x004F, 0, "SLL   ", SLL   );
	addInstruction(0x0050, 0x0057, 0, "RLC   ", RLC   );
	addInstruction(0x0058, 0x005F, 0, "SLLC  ", SLLC  );
	addInstruction(0x0060, 0x0067, 0, "SLR   ", SLR   );
	addInstruction(0x0068, 0x006F, 0, "SAR   ", SAR   );
	addInstruction(0x0070, 0x0077, 0, "RRC   ", RRC   );
	addInstruction(0x0078, 0x007F, 0, "SARC  ", SARC  );
	addInstruction(0x0080, 0x00BF, 1, "MOVR  ", MOVR  );
	addInstruction(0x00C0, 0x00FF, 1, "ADDR  ", ADDR  );
	addInstruction(0x0100, 0x013F, 1, "SUBR  ", SUBR  );
	addInstruction(0x0140, 0x017F, 1, "CMPR  ", CMPR  );
	addInstruction(0x0180, 0x01BF, 1, "ANDR  ", ANDR  );
	addInstruction(0x01C0, 0x01FF, 1, "XORR  ", XORR  );
	addInstruction(0x0200, 0x023F, 1, "Branch", Branch); // B, BC, BOV, BPL, BEQ, BLT, BLE, BUSC, NOPP, BNC, BNOV, BMI, BNEQ, BGE, BGT, BESC, BEXT
	addInstruction(0x0240, 0x0247, 0, "MVO   ", MVO   );
	addInstruction(0x0248, 0x026F, 0, "MVO@  ", MVOa  ); // error in wiki for all aaa@ and aaaI instructions
	addInstruction(0x0270, 0x0277, 0, "PSHR  ", PSHR  ); // 
	addInstruction(0x0278, 0x027F, 0, "MVOI  ", MVOI  ); //
	addInstruction(0x0280, 0x0287, 1, "MVI   ", MVI   );
	addInstruction(0x0288, 0x02AF, 1, "MVI@  ", MVIa  ); 
	addInstruction(0x02B0, 0x02B7, 1, "PULR  ", PULR  );
	addInstruction(0x02B8, 0x02BF, 1, "MVII  ", MVII  ); 
	addInstruction(0x02C0, 0x02C7, 1, "ADD   ", ADD   ); 
	addInstruction(0x02C8, 0x02F7, 1, "ADD@  ", ADDa  );
	addInstruction(0x02F8, 0x02FF, 1, "ADDI  ", ADDI  ); 
	addInstruction(0x0300, 0x0307, 1, "SUB   ", SUB   );
	addInstruction(0x0308, 0x0337, 1, "SUB@  ", SUBa  );
	addInstruction(0x0338, 0x033F, 1, "SUBI  ", SUBI  );
	addInstruction(0x0340, 0x0347, 1, "CMP   ", CMP   );
	addInstruction(0x0348, 0x0377, 1, "CMP@  ", CMPa  );
	addInstruction(0x0378, 0x037F, 1, "CMPI  ", CMPI  );
	addInstruction(0x0380, 0x0387, 1, "AND   ", AND   );
	addInstruction(0x0388, 0x03B7, 1, "AND@  ", ANDa  );
	addInstruction(0x03B8, 0x03BF, 1, "ANDI  ", ANDI  );
	addInstruction(0x03C0, 0x03C7, 1, "XOR   ", XOR   );
	addInstruction(0x03C8, 0x03F8, 1, "XOR@  ", XORa  );
	addInstruction(0x03F7, 0x03FF, 1, "XORI  ", XORI  );
}