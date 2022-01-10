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

#include <string.h>
#include "osd.h"

unsigned int DisplayWidth = 0;
unsigned int DisplayHeight = 0;
unsigned int DisplayColor[] = {0, 0xFFFFFF};
unsigned int DisplaySize = 0;
unsigned int *Frame;

// Paused Message

int pauseImage[572] = 
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,0,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,1,1,1,0,0,1,1,1,1,1,0,1,1,1,1,0,0,1,1,0,1,0,
0,1,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,
0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,
0,1,0,1,1,1,1,0,0,1,1,1,1,1,0,1,0,0,0,1,0,0,1,1,1,0,0,1,1,1,0,0,0,1,0,0,0,1,0,1,1,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1,1,1,0,0,0,1,1,1,0,0,1,1,1,1,1,0,1,1,1,1,0,0,1,1,0,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

void OSD_drawPaused()
{
	int i, j, k;
	int offset = 506;  
	k = 0;
	for(i=0; i<13; i++)
	{
		for(j=0; j<44; j++)
		{
			Frame[offset+j] = pauseImage[k]*0xFFFFFF;
			k++;
		}
		offset+=352;
	}
}

// Left / Right input Images

int leftImage[377] = // 29x13
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,1,1,1,0,0,1,1,1,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

int rightImage[455] = // 35x13
{
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,0,1,1,1,1,0,0,1,1,1,1,1,0,0,1,1,1,1,0,1,0,0,0,1,0,1,1,1,1,1,0,1,0,
0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
0,1,0,1,1,1,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,1,1,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
0,1,0,1,0,0,0,1,0,1,1,1,1,1,0,0,1,1,1,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,
0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

void OSD_drawLeftRight()
{
	int i, j, k1, k2;
	int offset = 73920; //210*352
	k1 = 0;
	k2 = 0;
	for(i=0; i<13; i++)
	{
		for(j=0; j<29; j++)
		{
			Frame[offset+j] = leftImage[k1]*0xFFFFFF;
			k1++;
		}
		for(j=0; j<35; j++)
		{
			Frame[offset+317+j] = rightImage[k2]*0xFFFFFF;
			k2++;
		}
		offset+=352;
	}
}

void OSD_drawRightLeft()
{
	int i, j, k1, k2;
	int offset = 73920; //210*352
	k1 = 0;
	k2 = 0;
	for(i=0; i<13; i++)
	{
		for(j=0; j<35; j++)
		{
			Frame[offset+j] = rightImage[k1]*0xFFFFFF;
			k1++;
		}
		for(j=0; j<29; j++)
		{
			Frame[offset+323+j] = leftImage[k2]*0xFFFFFF;
			k2++;
		}
		offset+=352;
	}
}

int letters[590] = // 32 - 90 59x10
{
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //0  space
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //1  !
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //2  "
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //3  #
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //4  $
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //5  %
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //6  &
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //7  '
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //8  (
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //9  )
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //10 *
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //11 +
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 8,  //12 ,  
	0, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0, 0,  //13 -
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0, 0,  //14 .
	0, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0, 0,  //15 /
	0, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  //16 0
	0, 0x18, 0x28, 0x08, 0x08, 0x08, 0x08, 0x3E, 0, 0,  //17 1
	0, 0x3C, 0x42, 0x02, 0x04, 0x18, 0x20, 0x7E, 0, 0,  //18 2
	0, 0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x3C, 0, 0,  //19 3
	0, 0x40, 0x44, 0x44, 0x7E, 0x04, 0x04, 0x04, 0, 0,  //20 4
	0, 0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x7C, 0, 0,  //21 5
	0, 0x3E, 0x40, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0, 0,  //22 6
	0, 0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0, 0,  //23 7
	0, 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0, 0,  //24 8
	0, 0x3C, 0x42, 0x42, 0x3E, 0x02, 0x02, 0x3C, 0, 0,  //25 9
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //26 :
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //27 ;
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //28 <
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //29 =
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //30 >
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //31 ?
	0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0,  //32 @
	0, 0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x42, 0, 0,  //33 A
	0, 0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0, 0,  //34 B
	0, 0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0, 0,  //35 C
	0, 0x7C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7C, 0, 0,  //36 D
	0, 0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0, 0,  //37 E
	0, 0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40, 0, 0,  //38 F
	0, 0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C, 0, 0,  //39 G
	0, 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0, 0,  //40 H
	0, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7C, 0, 0,  //41 I
	0, 0x7E, 0x04, 0x04, 0x04, 0x04, 0x44, 0x38, 0, 0,  //42 J
	0, 0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0, 0,  //43 K
	0, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0, 0,  //44 L
	0, 0x42, 0x66, 0x5A, 0x42, 0x42, 0x42, 0x42, 0, 0,  //45 M
	0, 0x42, 0x62, 0x52, 0x52, 0x4A, 0x46, 0x42, 0, 0,  //46 N
	0, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  //47 O
	0, 0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40, 0, 0,  //48 P
	0, 0x3C, 0x42, 0x42, 0x42, 0x4A, 0x46, 0x3D, 0, 0,  //49 Q
	0, 0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x42, 0, 0,  //50 R
	0, 0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C, 0, 0,  //51 S
	0, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0, 0,  //52 T
	0, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0, 0,  //53 U
	0, 0x42, 0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0, 0,  //54 V
	0, 0x42, 0x42, 0x42, 0x42, 0x5A, 0x66, 0x42, 0, 0,  //55 W
	0, 0x42, 0x42, 0x24, 0x18, 0x24, 0x42, 0x42, 0, 0,  //56 X
	0, 0x42, 0x42, 0x24, 0x18, 0x08, 0x08, 0x08, 0, 0,  //57 Y
	0, 0x7E, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7E, 0, 0   //58 Z
};

void OSD_setDisplay(unsigned int frame[], unsigned int width, unsigned int height)
{
	Frame = frame;
	DisplayWidth = width;
	DisplayHeight = height;
	DisplaySize = width*height;
}

void OSD_setColor(unsigned int color)
{
	DisplayColor[1] = color;
}

void OSD_setBackground(unsigned int color)
{
	DisplayColor[0] = color;
}

/* Utility functions */
void OSD_HLine(int x, int y, int len)
{
	int i, offset;

	if(x<0 || y<0 || (y*DisplayWidth+x+len)>DisplaySize)
      return;
	
	offset = (y*DisplayWidth)+x;
	for(i = 0; i <= len; i++)
	{
		Frame[offset] = DisplayColor[1];
		offset = offset + 1;
	}
}
void OSD_VLine(int x, int y, int len)
{
   int i, offset;

	if(x<0 || y<0 || ((y+len)*DisplayWidth+x)>DisplaySize)
      return;
	
	offset = (y*DisplayWidth)+x;

	for(i = 0; i <= len; i++)
	{
		Frame[offset] = DisplayColor[1];
		offset = offset + DisplayWidth;
	}
}

void OSD_Box(int x1, int y1, int width, int height)
{
	OSD_HLine(x1, y1, width);
	OSD_HLine(x1, y1+height, width);
	OSD_VLine(x1, y1, height);
	OSD_VLine(x1+width, y1, height);
}

void OSD_FillBox(int x1, int y1, int width, int height)
{
	int i;
	for(i=0; i<height; i++)
	{
		OSD_HLine(x1, y1+i, width);
	}
}

void OSD_drawLetter(int x, int y, int c)
{
	int i, j;
	unsigned int t = DisplayColor[0];
	int offset     = (DisplayWidth*y)+x;
	
	c = (c-32);
	if(c<0 || c>58)
      return;

	c = c * 10;

	for(i=0; i<10; i++)
	{
		for(j=0; j<8; j++)
		{
			if((offset+j)<DisplaySize)
			{
				DisplayColor[0] = Frame[offset+j];
				Frame[offset+j] = DisplayColor[((letters[c]>>(7-j))&0x01)];
			}
		}
		offset+=DisplayWidth;
		c++;
	}
	DisplayColor[0] = t;
}

void OSD_drawTextFree(int x, int y, const char *text)
{
	int len = strlen(text);
	int i = 0;
	int c = 32;
	while(i<len)
	{
		c = text[i];
		if(c<32) { break; }
		if(c>90) { c = 32; }
		i++;
		OSD_drawLetter(x, y, c);
		x+=8;
	}
}

void OSD_drawText(int x, int y, const char *text)
{
	x = x * 8;
	y = y * 10;
	OSD_drawTextFree(x, y, text);
}

void OSD_drawTextBG(int x, int y, const char *text)
{
	unsigned int t1 = DisplayColor[1];

	int len = (strlen(text)*8)+1;

	x = x * 8;
	y = y * 10;

	DisplayColor[1] = DisplayColor[0];
	OSD_FillBox(x, y, len, 10);
	DisplayColor[1] = t1;

	OSD_drawTextFree(x+1, y+1, text);
}

void OSD_drawTextCenterBG(int y, const char *text)
{
	unsigned int t1 = DisplayColor[1];

	int len = (strlen(text)*8)+1;
	int x = (DisplayWidth-len) / 2;
	y = y * 10;

	if(x>=0)
	{
		DisplayColor[1] = DisplayColor[0];
		OSD_FillBox(x, y, len, 10);
		DisplayColor[1] = t1;

		OSD_drawTextFree(x+1, y+1, text);
	}
}

void OSD_drawInt(int x, int y, int num, int base)
{
	char buffer[2] = {0,0};
	int r = 0;
	int t = num;

	if(base>2) { base = 10; }

	if(num<0)
	{
		num = 0-num;
		buffer[0] = '-';
		OSD_drawText(x, y, buffer);
		x++;
	}
	if(num==0)
	{
		buffer[0] = '0';
		OSD_drawText(x, y, buffer);
	}
	else
	{
		x--;
		while(t>0)
		{
			r = t % base;
			t = (t-r) / base;
			x++;
		}
		while(num>0)
		{
			r = num % base;
			num = (num-r) / base;
			if(r>9)
			{
				buffer[0] = 'A' + r;
			}
			else
			{
				buffer[0] = '0' + r;
			}
			OSD_drawText(x, y, buffer);
			x--;
		}
	}
}
