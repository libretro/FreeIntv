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

#include "intv.h"
#include "memory.h"
#include "stic.h"

void drawBackground();
void drawSprites();
void drawBorder();

// http://spatula-city.org/~im14u2c/intv/jzintv-1.0-beta3/doc/programming/stic.txt

//unsigned int STICmode = 1; // 0-foreground/background, 1-color stack/color squares 
//unsigned int frame[352*224]; // frame buffer

unsigned int cbuff[352*224]; // collision buffer

unsigned int delayH = 16; // Horizontal Delay
unsigned int delayV = 16; // Vertical Delay

unsigned int colors[16] =
{
	0x0C0005, /* 0x000000; */ // Black
	0x002DFF, /* 0x0000FF; */ // Blue
	0xFF3E00, /* 0xFF0000; */ // Red
	0xC9D464, /* 0xCBFF65; */ // Tan
	0x00780F, /* 0x007F00; */ // Dark Green
	0x00A720, /* 0x00FF00; */ // Light Green 
	0xFAEA27, /* 0xFFFF00; */ // Yellow
	0xFFFCFF, /* 0xFFFFFF; */ // White
	0xA7A8A8, /* 0x7F7F7F; */ // Gray
	0x5ACBFF, /* 0x00FFFF; */ // Cyan
	0xFFA600, /* 0xFF9F00; */ // Orange
	0x3C5800, /* 0x7F7F00; */ // Brown
	0xFF3276, /* 0xFF3FFF; */ // Pink
	0xBD95FF, /* 0x7F7FFF; */ // Violet
	0x6CCD30, /* 0x7FFF00; */ // Bright green
	0xC81A7D  /* 0xFF007F; */ // Magenta
};

void STICReset()
{
	STICmode = 1;
	VBlank1 = 0;
	VBlank2 = 0;
	SR1 = 0;
	Cycles = 0;
	DisplayEnabled = 0;
}

void STICDrawFrame()
{
	int i, j;
	// clear collision buffer 
	for(i=0; i<352*224; i++)
	{ 
		cbuff[i]=0;
	}

	delayH = 16+(Memory[0x30]<<1); //16-Memory[0x30];

	delayV = 16+(Memory[0x31]<<1); //16-Memory[0x31];

	// draw displayed area
	drawBackground();
	
	// draw MOBs
	drawSprites();

	// draw border
	drawBorder();
	
	// complete collisions e.g.:
	// if MOB2 hits MOB1, MOB1 should also hit MOB2
	for(i=0; i<8; i++)
	{
		// clear self-interactions 
		if(((Memory[0x18+i]>>i)&0x01)==1)
		{ 
			Memory[0x18+i] = Memory[0x18+i] ^ (1<<i);
		} 
		// skip non-interacting sprites
		if(((Memory[0x00+i]>>8)&0x01)==0) { continue; }
		
		// copy collisions to colliding sprites
		for(j=0; j<8; j++)
		{
			if(j==i) { continue; }
			if(((Memory[0x00+j]>>8)&0x01)==0) { continue; } // skip non-interacting sprites
			if(((Memory[0x18+j]>>i) & 1) == 1)
			{
				Memory[0x18+i] |= (1<<j);
			}
		}
	}
}

void drawBorder()
{
	int i;
	int j;
	int cbit = 1<<9;
	int color = colors[Memory[0x2C]]; // border color
	int extendTop = 16*((Memory[0x32]>>1)&0x01);
	int extendLeft = 16*(Memory[0x32]&0x01);
	for(i=0; i<16+extendTop; i++)
	{
		for(j=0; j<352; j++)
		{
			frame[(i*352)+j] = color;
			cbuff[(i*352)+j] |= cbit;
		}
	}
	for(i=15; i<209; i++)
	{
		for(j=0; j<32+extendLeft; j++)
		{
			frame[(i*352)+336+j] = color;
			cbuff[(i*352)+j] |= cbit;
		}
	}
	for(i=208; i<224; i++)
	{
		for(j=0; j<352; j++)
		{
			frame[(i*352)+j] = color;
			cbuff[(i*352)+j] |= cbit;
		}
	}
}

void drawSprites() // MOBs
{
	int i, j, k, m, n;
	int fgcolor;
	int Rx, Ry, Ra; // sprite/MOB registers
	int gfx; // address of card / sprite data
	int gdata; // current byte of sprite data
	int card; // card number
	int gram; // is sprite in GRAM or GROM
	int offset; // address of upper-left corner of sprite in framebuffer
	int sizeX; 
	int sizeY; 
	int flipX; 
	int flipY;
	int posX;
	int posY;
	int yRes; // some sprites are two-tiles
	int cbit; // collision bit for collision buffer for collision detection
	
	int gfxheight; // sprite is either 8 or 16 bytes (1 or 2 tiles) tall
	int rowheight; // line size for sprite scaling on Y-axis
	int row; // current scan line from start of sprite
	int col; // current offset from first column of sprite 

	int pixel, showpixel, bit;
	int bitstart, bitdir;
	int gfxstart, gfxdir;
	int priority = 0;

	for(i=7; i>=0; i--)
	{
		cbit = 1<<i; // set collision bit

		Rx = Memory[0x00+i];
		Ry = Memory[0x08+i];
		Ra = Memory[0x10+i];

		// if sprite x coordinate is 0, it's disabled
		// if it's not visible and not interactive, it's disabled
		if((Rx & 0xFF)==0 || ((Rx>>8)&0x03)==0) { continue; }

		gram = (Ra>>11) & 0x01;
		card = (Ra>>3) & 0xFF;
		if(gram==1) { card = card & 0x3F; } // ignore bits 6 and 7
		gfx = 0x3000 + (0x800*gram) + (card*8);
		
		sizeX = (Rx>>10) & 0x01;
		sizeY = (Ry>>8) & 0x03;
		flipX = (Ry>>10) & 0x01;
		flipY = (Ry>>11) & 0x01;
		posX = ((Rx & 0xFF)<<1)+(delayH-16);
		posY = ((Ry & 0x7F)<<1)+(delayV-16);
		yRes = (Ry>>7) & 0x01;
		priority = (Ra>>13) & 0x01;

		fgcolor = colors[((Ra>>9)&0x08)|(Ra&0x07)];	
		
		offset = posY*352 + posX;

		gfxheight=8;
		rowheight = 1<<sizeY; // 1, 2, 4, or 8 from 0, 1, 2, or 3

		if(yRes==1) //16 bit tall sprites, the second card follows in memory
		{
			gfxheight=16;
			// for double-resolution sprites, the first card drops bit0 from address
			gfx = gfx & 0xFFFE;
		}

		bitstart = 7;
		bitdir = -1;
		gfxdir = 1;
		if(flipY) { gfx += gfxheight-1; gfxdir = -1; }
		if(flipX) { bitstart = 0; bitdir = 1; }

		// draw sprites
		row=0;
		col=0;
		for(j=0; j<gfxheight; j++)
		{
			gdata = Memory[gfx];
			gfx+=gfxdir;
			if(offset+(row*352)<(352*delayV)) { row++; continue; } // clip
			for(m=0; m<rowheight; m++)
			{
				if(offset+(row*352)+32>(352*224)) { row++; continue; } // clip
				bit = bitstart;
				col = 0;
				for(k=0; k<8; k++)
				{
					if(posX+col+2+(2*sizeX)>352) { continue; } // clip

					n = offset+(row*352)+col;
					pixel = (gdata>>bit) & 1;
					
					if(((Rx>>9)&0x01)==1) // visible
					{
						showpixel = !(pixel==0 | (priority==1 & ((cbuff[n]>>8)&0x01))==1);
						frame[n] = showpixel ? fgcolor : frame[n];
						frame[n+1] = showpixel ? fgcolor : frame[n+1];
						// draw four pixels wide if sizeX==1
						frame[n+(2*sizeX)] = showpixel ? fgcolor : frame[n+(2*sizeX)]; 
						frame[n+(3*sizeX)] = showpixel ? fgcolor : frame[n+(3*sizeX)];
					}
					if(pixel==1 && ((Rx>>8)&0x01)==1) // update collisions if interacting
					{
						Memory[0x18+i] |= cbuff[n];
						Memory[0x18+i] |= cbuff[n];
						Memory[0x18+i] |= cbuff[n+(2*sizeX)];
						Memory[0x18+i] |= cbuff[n+(3*sizeX)];
						cbuff[n] |= (cbit*pixel);
						cbuff[n+1] |= (cbit*pixel);
						cbuff[n+(2*sizeX)] |= (cbit*pixel); 
						cbuff[n+(3*sizeX)] |= (cbit*pixel);
					}
					col+=2+(2*sizeX);
					bit+=bitdir;
				}
				row++; // move to next row
			}
		}
	}
}

void drawBackground()
{
	int i, j, k, a;
	int row, col, offset;
	unsigned int card; // BACKTAB card info
	unsigned int bgcolor;
	unsigned int fgcolor;
	unsigned int gram; // 0-GROM, 1-GRAM
	unsigned int cardnum; // card index
	unsigned int gfx; // card graphic address
	unsigned int gdata; // current card graphic byte
	
	int CSP = 0x28; // Color Stack Pointer: 0x28-0x2B
	int advcolor; // advance CSP
	int c[4] = {0, 0, 0, 0}; // color squares colors
	int sc = 0; // selected color
	int cbit = 1<<8; // collision bit for BG

	if(STICmode==0) // Foreground/Background Mode
	{
		for(i=0; i<240; i++)
		{
			row = i / 20;
			col = i % 20;
			offset = (352*16*row)+(16*col);
			offset += delayV*352 + delayH;

			card = Memory[0x200+i];
			fgcolor = colors[card & 0x07];
			bgcolor = colors[((card>>9)&0x03) | ((card>>11)&0x04) | ((card>>9)&0x08)];
			gram = (card>>11) & 0x01;
			cardnum = (card>>3) & 0x3F;

			gfx = 0x3000 + (0x800*gram) + (cardnum*8);

			for(j=0; j<8; j++)
			{
				gdata = Memory[gfx+j];
				for(k=0; k<8; k++)
				{
					sc = ((gdata>>k)&1)==1 ? fgcolor : bgcolor;
					a = offset+(352*j*2)+((7-k)*2);
					frame[a] = sc;
					frame[a+1] = sc;
					frame[a+352] = sc;
					frame[a+352+1] = sc;
					if(((gdata>>k)&1)==1)
					{
						cbuff[a] |= cbit;
						cbuff[a+1] |= cbit;
						cbuff[a+352] |= cbit;
						cbuff[a+352+1] |= cbit;
					}
				}
			}
		}
	}
	else // Color Stack Modes
	{
		for(i=0; i<240; i++)
		{
			row = i / 20;
			col = i % 20;
			offset = (352*16*row)+(16*col);
			offset += delayV*352 + delayH;
			
			card = Memory[0x200+i];

			if(((card>>11)&0x03)==2) // Color Squares Mode
			{
				c[0] = colors[card & 0x07];
				c[1] = colors[(card>>3) & 0x07];
				c[2] = colors[(card>>6) & 0x07];
				c[3] = colors[((card>>11)&0x04)|((card>>9)&0x03)];

				for(j=0; j<8; j++)
				{
					for(k=0; k<8; k++)
					{
						a = offset+(352*j*2)+(k*2);
						sc = ((j>4)<<1) & (k>4); // select color
						frame[a] = c[sc];
						frame[a+1] = c[sc];
						frame[a+352] = c[sc];
						frame[a+352+1] = c[sc];
						cbuff[a] |= cbit*(sc<7);
						cbuff[a+1] |= cbit*(sc<7);
						cbuff[a+352] |= cbit*(sc<7);
						cbuff[a+352+1] |= cbit*(sc<7);
					}
				}
				
			}
			else // Normal Color Stack Mode 
			{
				gram = (card>>11) & 0x01;
				if(gram) { fgcolor = colors[(card&0x07) | ((card>>9)&0x08)]; }					
				
				advcolor = (card>>13) & 0x01;
				CSP = (CSP+advcolor) & 0x2B; // cycle through 0x28-0x2B
				
				fgcolor = colors[(card&0x07)|((card>>9)&0x08)];
				bgcolor = colors[Memory[CSP] & 0x0F];

				cardnum = (card>>3) & 0xFF;
				if(gram) { cardnum = cardnum & 0x3F; }

				gfx = 0x3000 + (0x800*gram) + (cardnum*8);
				
				for(j=0; j<8; j++)
				{
					gdata = Memory[gfx+j];
					for(k=0; k<8; k++)
					{
						sc = ((gdata>>k)&1)==1 ? fgcolor : bgcolor;
						a = offset+(352*j*2)+((7-k)*2);
						frame[a] = sc;
						frame[a+1] = sc;
						frame[a+352] = sc;
						frame[a+352+1] = sc;
						if(((gdata>>k)&1)==1)
						{
							cbuff[a] |= cbit;
							cbuff[a+1] |= cbit;
							cbuff[a+352] |= cbit;
							cbuff[a+352+1] |= cbit;
						}
					}
				}
			}
		}
	}
}