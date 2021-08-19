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

extern int stic_phase;
extern int stic_vid_enable;
extern int stic_reg;
extern int stic_gram;
extern int phase_len;
extern int delayV;
extern int delayH;

extern int DisplayEnabled; // determines if frame should be updated or not

extern unsigned int frame[352*224]; // frame buffer

struct STICserialized {
    unsigned int STICMode;

    int stic_phase;
    int stic_vid_enable;
    int stic_reg;
    int stic_gram;
    int phase_len;

    int DisplayEnabled;

    int delayH;
    int delayV;

    int extendTop;
    int extendLeft;

    unsigned int CSP;
    unsigned int fgcard[20];
    unsigned int bgcard[20];

    unsigned int frame[352*224];
};

void STICSerialize(struct STICserialized *);
void STICUnserialize(const struct STICserialized *);

void STICDrawFrame(int);
void STICReset(void);

#endif
