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
#include "cp1610.h"
#include "stic.h"

#include <stdio.h>
#include <string.h>

void drawBackground(void);
void drawSprites(int scanline);
void drawBorder(int scanline);
void drawBackgroundFGBG(int scanline);
void drawBackgroundColorStack(int scanline);

// Video chip: TMS9927 AY-3-8900-1
// http://spatula-city.org/~im14u2c/intv/jzintv-1.0-beta3/doc/programming/stic.txt
// http://spatula-city.org/~im14u2c/intv/tech/master.html


#if defined(ABGR1555)
unsigned int color7 = 0xFFFCFF; // Copy of color 7 (for color squares mode)
unsigned int colors[16] =
{
	0x05000C, /* 0x000000; */ // Black
	0xFF2D00, /* 0x0000FF; */ // Blue
	0x003EFF, /* 0xFF0000; */ // Red
	0x64D4C9, /* 0xCBFF65; */ // Tan
	0x0F7800, /* 0x007F00; */ // Dark Green
	0x20A700, /* 0x00FF00; */ // Light Green 
	0x27EAFA, /* 0xFFFF00; */ // Yellow
	0xFFFCFF, /* 0xFFFFFF; */ // White
	0xA8A8A7, /* 0x7F7F7F; */ // Gray
	0xFFCB5A, /* 0x00FFFF; */ // Cyan
	0x00A6FF, /* 0xFF9F00; */ // Orange
	0x00583C, /* 0x7F7F00; */ // Brown
	0x7632FF, /* 0xFF3FFF; */ // Pink
	0xFF95BD, /* 0x7F7FFF; */ // Violet
	0x30CD6C, /* 0x7FFF00; */ // Bright green
	0x7D1AC8  /* 0xFF007F; */ // Magenta
};
#else
unsigned int color7 = 0xFFFCFF; // Copy of color 7 (for color squares mode)
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
#endif

int reverse[256] = // lookup table to reverse the bits in a byte //
{
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

void STICReset(void)
{
	CTX(STICMode) = 1;
	SR1 = 0;
	CTX(DisplayEnabled) = 0;
	CTX(CSP) = 0x28;
    CTX(stic_phase) = 15;
    CTX(phase_len) = 2782;
}

void drawBorder(int scanline)
{
	int i;
	int cbit = 1<<9; // bit 9 - border collision 
	int color = colors[CTX(Memory)[0x2C] & 0x0f]; // border color
	
	if(scanline>=112) { return; }
    if (scanline == CTX(delayV) - 1 || scanline == 104) {
        for(i=7 * 2; i < (9 + 160) * 2; i += 2)
        {
            CTX(collBuffer)[i] |= cbit;
            CTX(collBuffer)[i+384] |= cbit;
        }
    } else {
        i = 7 * 2;
        CTX(collBuffer)[i] |= cbit;
        CTX(collBuffer)[i + 384] |= cbit;
        i = (8 + 160) * 2;
        CTX(collBuffer)[i] |= cbit;
        CTX(collBuffer)[i + 384] |= cbit;
    }
    if (CTX(extendTop) != 0)
        i = 16;
    else
        i = CTX(delayV);
    if(scanline<i || scanline>=104) // top and bottom border
	{
		for(i=0; i<352; i++)
		{
			CTX(scanBuffer)[i] = color;
			CTX(scanBuffer)[i+384] = color;
		}
	}
	else // left and right border
	{
		for(i=0; i<16+(16*CTX(extendLeft)); i++)
		{
			CTX(scanBuffer)[i] = color;
			CTX(scanBuffer)[i+336] = color;
			CTX(scanBuffer)[i+384] = color;
			CTX(scanBuffer)[i+384+336] = color;
		}
	}
}

void drawBackgroundFGBG(int scanline)
{
	int i; 
	int row, col; // row offset and column of current card
	int cardrow;  // which of the 8 rows of the current card to draw
	int card;     // BACKTAB card info
	unsigned int bgcolor;
	unsigned int fgcolor;
	int gaddress; // card graphic address
	int gdata;    // current card graphic byte
	int cbit = 1<<8;   // bit 8 - collision bit for Background
	int x = CTX(delayH); // current pixel offset 

	// Tiled background is 20x12, cards are 8x8
	row = scanline / 8; // Which tile row? (Background is 96 lines high)
	row = row * 20; // find BACKTAB offset 

	cardrow = scanline % 8; // which line of this row of cards to draw

	// Draw cards
	for (col=0; col<20; col++) // for each card on the current row...
	{
		card = CTX(Memory)[0x200+row+col]; // card info from BACKTAB

		fgcolor = colors[card & 0x07];
		bgcolor = colors[((card>>9)&0x03) | ((card>>11)&0x04) | ((card>>9)&0x08)]; // bits 12,13,10,9
		
        gaddress = 0x3000 + (card & 0x09f8);
		
		gdata = CTX(Memory)[gaddress + cardrow]; // fetch current line of current card graphic

		for(i=7; i>=0; i--) // draw one line of card graphic
		{
			if(((gdata>>i)&1)==1)
			{
				// draw pixel
				CTX(scanBuffer)[x] = fgcolor;
				CTX(scanBuffer)[x+1] = fgcolor;
				CTX(scanBuffer)[x+384] = fgcolor;
				CTX(scanBuffer)[x+384+1] = fgcolor;
				// write to collision buffer 
				CTX(collBuffer)[x] |= cbit;
				CTX(collBuffer)[x+384] |= cbit;
			}
			else
			{
				// draw background
				CTX(scanBuffer)[x] = bgcolor;
				CTX(scanBuffer)[x+1] = bgcolor;
				CTX(scanBuffer)[x+384] = bgcolor;
				CTX(scanBuffer)[x+384+1] = bgcolor;
			}		
			x+=2;
		}
	}
}

void drawBackgroundColorStack(int scanline)
{
    int i;
    unsigned int color1, color2;
    int cbit1, cbit2;
    int row, col; // row offset and column of current card
    int cardrow;  // which of the 8 rows of the current card to draw
    int card;     // BACKTAB card info
    unsigned int bgcolor;
    unsigned int fgcolor;
    int gaddress; // card graphic address
    int gdata;    // current card graphic byte
    int advcolor; // Flag - Advance CTX(CSP)
    int cbit = 1<<8;   // bit 8 - collision bit for Background
    int x = CTX(delayH); // current pixel offset
    
    // Tiled background is 20x12, cards are 8x8
    row = (scanline / 8); // Which tile row? (Background is 96 lines high)
    row = row * 20; // find BACKTAB offset
    
    cardrow = scanline % 8; // which line of this row of cards to draw
    
    if(row==0 && cardrow==0) { CTX(CSP) = 0x28; } // reset CTX(CSP) on display of first card on screen
    
    // Draw cards
    for (col=0; col<20; col++) // for each card on the current row...
    {
        card = CTX(Memory)[0x200+row+col]; // card info from BACKTAB
        
        if(((card>>11)&0x03)==2) // Color Squares Mode
        {
            if (cardrow == 0)
                CTX(bgcard)[col] = colors[CTX(Memory)[CTX(CSP)] & 0x0F];
            // set colors
            colors[7] = CTX(bgcard)[col]; // color 7 is top of color stack
            color1 = card & 0x07;
            color2 = (card>>3) & 0x07;
            if(cardrow>=4) // switch to lower squares colors
            {
                color1 = (card>>6) & 0x07;	                 // color 3
                color2 = ((card>>11)&0x04)|((card>>9)&0x03); // color 4
            }
            // color 7 does not interact with sprites
            cbit1 = cbit2 = cbit;
            if(color1==7) { cbit1=0; }
            if(color2==7) { cbit2=0; }
            color1 = colors[color1]; // set to rgb24 color
            color2 = colors[color2];
            colors[7] = color7; // restore color 7
            // draw squares
            for(i=0; i<8; i += 2)
            {
                CTX(scanBuffer)[x] = color1;
                CTX(scanBuffer)[x+1] = color1;
                CTX(scanBuffer)[x+8] = color2;
                CTX(scanBuffer)[x+9] = color2;
                CTX(scanBuffer)[x+384] = color1;
                CTX(scanBuffer)[x+384+1] = color1;
                CTX(scanBuffer)[x+384+8] = color2;
                CTX(scanBuffer)[x+384+9] = color2;
                CTX(collBuffer)[x] |= cbit1;
                CTX(collBuffer)[x+8] |= cbit2;
                CTX(collBuffer)[x+384] |= cbit1;
                CTX(collBuffer)[x+384+8] |= cbit2;
                x+=2;
            }
            x+=8;
            
        }
        else // Color Stack Mode
        {
            if(cardrow == 0) // only advance CTX(CSP) once per card, cache card colors for later scanlines
            {
                advcolor = (card>>13) & 0x01; // do we need to advance the CTX(CSP)?
                CTX(CSP) = (CTX(CSP)+advcolor) & 0x2B; // cycles through 0x28-0x2B
                CTX(fgcard)[col] = colors[(card&0x07)|((card>>9)&0x08)]; // bits 12, 2, 1, 0
                CTX(bgcard)[col] = colors[CTX(Memory)[CTX(CSP)] & 0x0F];
            }
            
            fgcolor = CTX(fgcard)[col];
            bgcolor = CTX(bgcard)[col];
            
            if (((card >> 11) & 0x01) != 0) // Limit GRAM to 64 cards
                gaddress = 0x3000 + (card & 0x09f8);
            else
                gaddress = 0x3000 + (card & 0x0ff8);
            
            gdata = CTX(Memory)[gaddress + cardrow]; // fetch current line of current card graphic
            for(i=7; i>=0; i--) // draw one line of card graphic
            {
                if(((gdata>>i)&1)==1)
                {
                    // draw pixel
                    CTX(scanBuffer)[x] = fgcolor;
                    CTX(scanBuffer)[x+1] = fgcolor;
                    CTX(scanBuffer)[x+384] = fgcolor;
                    CTX(scanBuffer)[x+384+1] = fgcolor;
                    // write to collision buffer 
                    CTX(collBuffer)[x] |= cbit;
                    CTX(collBuffer)[x+384] |= cbit;
                }
                else
                {
                    // draw background
                    CTX(scanBuffer)[x] = bgcolor;
                    CTX(scanBuffer)[x+1] = bgcolor;
                    CTX(scanBuffer)[x+384] = bgcolor;
                    CTX(scanBuffer)[x+384+1] = bgcolor;
                }
                x+=2;
            }
        }
    }
}

void drawSprites(int scanline) // MOBs
{
	int i, j, k, x;
	int fgcolor;    // Foreground Color - (Ra bits 12, 2, 1, 0)
	int Rx, Ry, Ra; // sprite/MOB registers
	int gaddress;   // address of card / sprite data
	int gdata;      // current byte of sprite data
	int gdata2;     // current byte of sprite data (second row for half-height sprites)
	int card;       // card number - Ra bits 10-3
	int sizeX;      // 0-normal, 1-double width (Rx bit 10)
	int sizeY;      // 0-half height, 1-normal, 2-double, 3-quadrupal (Ry bits 9, 8)
	int flipX;      // (Ry bit 10)
	int flipY;      // (Ry bit 11)
	int posX;       // (Rx bits 7-0)
	int posY;       // (Ry bits 6-0)
	int yRes;       // 0-normal, 1-two tiles high (Ry bit 7)
	int priority;   // 0-normal, 1-behind background cards (Ra bit 13)
	int cbit;       // collision bit for collision buffer for collision detection

	int gfxheight;  // sprite is either 8 or 16 bytes (1 or 2 tiles) tall
	int spriterow;  // row of sprite data to draw

	if(scanline>104) { return; } // one line extra for bottom border collision

	for(i=7; i>=0; i--) // draw sprites 0-7 in reverse order
	{
		Rx = CTX(Memory)[0x00+i]; // 14 bits ; -- -SVI xxxx xxxx ; Size, Visible, Interactive, X Position
		Ry = CTX(Memory)[0x08+i]; // 14 bits ; -- YX42 Ryyy yyyy ; Flip Y, Flip X, Size 4, Size 2, Y Resolution, Y Position
		Ra = CTX(Memory)[0x10+i]; // 14 bits ; PF Gnnn nnnn nFFF ; Priority, FG Color Bit 3, GRAM, n Card #, FG Color Bits 2-0

		posX  = Rx & 0xFF;
		posY  = Ry & 0x7F;

		// if sprite x coordinate is 0 or >167, it's disabled
		// if it's not visible and not interactive, it's disabled
		if(posX==0 || posX>=167 || ((Rx>>8)&0x03)==0 || posY>=104) { continue; }

        cbit = 1<<i; // set collision bit

        card = Ra & 0x0ff8;
        yRes  = (Ry>>7) & 0x01;
        if(yRes==1)
        {
            // for double-y resolution sprites, the card number is always even
            card = card & 0xFFF0;
        }

        // Limit card number to 64 if in GRAM or in Foreground/Background mode
        if(CTX(STICMode)==0 || ((Ra>>11) & 0x01) == 1) { card = card & 0x09f8; }
        gaddress = 0x3000 + card;
        
        fgcolor = colors[((Ra>>9)&0x08)|(Ra&0x07)];
        sizeX = (Rx>>10) & 0x01;
        sizeY = (Ry>>8) & 0x03;
        flipX = (Ry>>10) & 0x01;
        flipY = (Ry>>11) & 0x01;
        priority = (Ra>>13) & 0x01;
        
        // sprite height varies by sizeY and yRes.  When yRes is set, the size doubles.
		// sizeY will be 0,1,2,3, corresponding to heights of 4,8, 16, and 32
		// we can find this by left-shifting 4 by sizeY as 4<<0==4, ..., 4<<3==32 
		gfxheight = (4<<sizeY)<<yRes; // yres=0: 4,8,16,32 ; yres=1: 8,16,32,64
		
		if( (scanline>=posY) && (scanline<(posY+gfxheight)) ) // if sprite is on current row
		{ 	
			// find sprite graphics data for current row
			spriterow = (scanline - posY); 
			if(sizeY==0)
			{
				spriterow = spriterow * 2;
			}
			else
			{
				spriterow = spriterow >> (sizeY-1);
			}

			if(flipY)
			{
				spriterow = (7+(8*yRes)) - spriterow;
				gaddress = gaddress + spriterow; 
				gdata  = CTX(Memory)[gaddress] & 0xFF;
				gdata2 = CTX(Memory)[gaddress - (sizeY==0)] & 0xFF;
			}
			else
			{
				gaddress = gaddress + spriterow; 
				gdata  = CTX(Memory)[gaddress] & 0xFF;
				gdata2 = CTX(Memory)[gaddress + (sizeY==0)] & 0xFF;
			}

			if(flipX)
			{
				gdata  = reverse[gdata];
				gdata2 = reverse[gdata2];
			}

			// draw sprite row //
			x = (CTX(delayH)-16) + (posX * 2); // pixels are 2x2 to accomodate half-height pixels

			for(j=0; j<2; j++)
			{
				for(k=7; k>=0; k--, x+=2+(2*sizeX))
				{
					if(((gdata>>k) & 1)==0) // skip ahead if pixel is not visible
					{
						continue;
					} 
					
					// set collision and collision buffer bits //
					if((Rx>>8)&1) // if sprite is interactive
					{
						CTX(collBuffer)[x] |= cbit;
						CTX(collBuffer)[x+2*sizeX] |= cbit; // for double width
					}
					
					if(priority && ((CTX(collBuffer)[x]>>8)&1)) // don't draw if sprite is behind background
					{
						continue;
					} 
					
					// draw sprite //
					if((Rx>>9)&1) // if sprite is visible
					{
						CTX(scanBuffer)[x] = fgcolor;
						CTX(scanBuffer)[x+1] = fgcolor;
						CTX(scanBuffer)[x+2*sizeX] = fgcolor; // for double width
						CTX(scanBuffer)[x+3*sizeX] = fgcolor;
					}
                }
				gdata = gdata2;  // for second half-pixel row  //
				x = (CTX(delayH)-16) + 384 + (posX * 2); // for second half-pixel row //
			}
		}
	}
}

void STICDrawFrame(int enabled)
{
	int row, offset;
	int i, j;

    offset = 0;
    if (enabled == 0) {
        for (row = 0; row < 112; row++)
        {
            int color = colors[CTX(Memory)[0x2C] & 0x0f]; // border color
            
            for(i=0; i<352; i++)
            {
                CTX(scanBuffer)[i] = color;
                CTX(scanBuffer)[i+384] = color;
            }
            memcpy(&CTX(frame)[offset], &CTX(scanBuffer)[0], 352 * sizeof(unsigned int));
            memcpy(&CTX(frame)[offset + 352], &CTX(scanBuffer)[384], 352 * sizeof(unsigned int));
            offset += 352 * 2;
        }
    } else {
        CTX(extendTop) = (CTX(Memory)[0x32]>>1)&0x01;
        
        CTX(extendLeft) = (CTX(Memory)[0x32])&0x01;
        
        CTX(delayV) = 8 + ((CTX(Memory)[0x31])&0x7);
        CTX(delayH) = 8 + ((CTX(Memory)[0x30])&0x7);
        
        CTX(delayH) = CTX(delayH) * 2;
        
        for(row=0; row<112; row++)
        {
            memset(&CTX(collBuffer)[0], 0, sizeof(CTX(collBuffer)));
            
            // draw backtab
            if(row>=CTX(delayV) && row<(96+CTX(delayV)))
            {
                if(CTX(STICMode)==0) // Foreground/Background Mode
                {
                    drawBackgroundFGBG(row-CTX(delayV));
                }
                else // Color Stack Modes
                {
                    drawBackgroundColorStack(row-CTX(delayV));
                }
            }
            
            if (row>=CTX(delayV) - 1 && row<(97 + CTX(delayV))) {
                // draw MOBs
                drawSprites((row-CTX(delayV))+8);
            }
            
            // draw border and set final collision bits
            drawBorder(row);
            // clear collisions in column 167 //
            CTX(collBuffer)[167 * 2] = 0;
            CTX(collBuffer)[167 * 2 + 384] = 0;
            
            for (i = 14; i < 169 * 2; i += 2) {
                if (CTX(collBuffer)[i] == 0)
                    continue;
                for (j = 0; j < 8; j++) {
                    if (((CTX(collBuffer)[i] >> j) & 1) != 0) {
                        CTX(Memory)[0x18 + j] |= CTX(collBuffer)[i] & ~(1 << j);
                    }
                }
            }
            memcpy(&CTX(frame)[offset], &CTX(scanBuffer)[0], 352 * sizeof(unsigned int));
            memcpy(&CTX(frame)[offset + 352], &CTX(scanBuffer)[384], 352 * sizeof(unsigned int));
            offset += 352 * 2;
        }
    }
}
