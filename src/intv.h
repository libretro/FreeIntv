#ifndef INTV_H
#define INTV_H
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

#define AUDIO_FREQUENCY     44100

extern int SR1; // SR1 line for interrupt

extern int intv_halt;

void LoadGame(const char *path);

void loadExec(const char *path);

void loadGrom(const char *path);

void Run(void);

void Init(void);

void Reset(void);

#endif
