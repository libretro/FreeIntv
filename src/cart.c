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
#include "memory.h"
#include "cp1610.h"
#include "cart.h"
#include "osd.h"

int isIntellicart(void);
int loadIntellicart(void);
int isROM(void);
int loadROM(void);
int getLoadMethod(void);
void load0(void);
void load1(void);
void load2(void);
void load3(void);
void load4(void);
void load5(void);
void load6(void);
void load7(void);
void load8(void);
void load9(void);

int data[0x20000]; // rom data loaded from file

int size = 0; // size of file read

int pos = 0; // current position in data

int LoadCart(const char *path)
{
	unsigned char word[1];
	FILE *fp;

    printf("[INFO] [FREEINTV] Attempting to load cartridge ROM from: %s\n", path);		

	size = 0;

	if((fp = fopen(path,"rb"))!=NULL)
	{
		while(fread(word,sizeof(word),1,fp) && size<0x20000)
		{
			data[size] = word[0];
			size++;
		}
        fclose(fp);
        if (feof(fp))
        {
            printf("[INFO] [FREEINTV] Successful cartridge load: EOF indicator set\n");
        }
        if (ferror(fp))
        {
            printf("[ERROR] [FREEINTV] Cartridge load error indicator set\n");
        }
        
		OSD_drawText(8, 7, "SIZE:");
		OSD_drawInt(14, 7, size, 10);

        if(isIntellicart()) // intellicart format
        {
			OSD_drawText(8, 8, "INTELLICART");
            printf("[INFO] [FREEINTV] Intellicart cartridge format detected\n");		
            return loadIntellicart();
        }
        else
        {
			if(isROM())
			{
				OSD_drawText(8, 8, "INTELLICART");
				OSD_drawText(8, 9, "MISSING A8!");
				printf("[INFO] [FREEINTV] Possible Intellicart cartridge format detected\n");
				return loadROM();
			}
			else
			{
				// check cartinfo database for load method
				printf("[INFO] [FREEINTV] Raw ROM image. Determining load method via database.\n");		
				switch(getLoadMethod())
				{
						case 0: load0(); break;
						case 1: load1(); break;
						case 2: load2(); break;
						case 3: load3(); break;
						case 4: load4(); break;
						case 5: load5(); break;
						case 6: load6(); break;
						case 7: load7(); break;
						case 8: load8(); break;
						case 9: load9(); break;
						default: printf("[INFO] [FREEINTV] No database match. Using default cartridge memory map.\n"); load0();
				}
			}
        }
        return 1; // loaded okay
	}
    else
    {
        printf("[ERROR] [FREEINTV] Failed to load cartridge ROM file.\n");		
        return 0;
    }
}

int readWord(void)
{
   int val;

	pos = pos * (pos<size);
	val = (data[pos]<<8) | data[pos+1];
	pos+=2;
	return val;
}

void loadRange(int start, int stop)
{
	while(start<=stop && pos<size) // load segment
	{
		CTX(Memory)[start] = readWord();
		start++;
	}
}

// http://spatula-city.org/~im14u2c/intv/jzintv-1.0-beta3/doc/rom_fmt/IntellicartManual.booklet.pdf
int isIntellicart() // check for intellicart format rom
{
	// check magic number (used for intellicart baud rate detection)
	return (data[0]==0xA8); 
}

int isROM() // some Intellicart roms don't start with A8 for no apparent reason
{
	// the third byte should be the 1's compliment of the second byte
	return data[1] == (data[2]^0xFF);
}

int loadIntellicart() // load intellicart format rom
{
	int start;
	int stop;
	int i, t;
	int segments;

	pos = 0;
	segments = readWord() & 0xFF; // number of non-contiguous rom segments (drop magic number)
	pos++; // 1's compliment of segments (ignore)

	for(i=0; i<segments; i++)
	{
		t = readWord(); // high bytes of segment start and stop addresses
		start = t & 0xFF00;
		stop = ((t<<8) & 0xFF00) | 0xFF;
		loadRange(start, stop);
		t = readWord(); // CRC for segment (ignored)
	}
	// Enable tables (ignored)
	return 1;
}

int loadROM() // load ROM formatted cart
{
	return loadIntellicart();
}

// http://atariage.com/forums/topic/203179-config-files-to-use-with-various-intellivision-titles/

void load0() // default - handles majority of carts
{
	loadRange(0x5000, 0x6FFF);
	loadRange(0xD000, 0xDFFF);
	loadRange(0xF000, 0xFFFF);
}

void load1()
{
	loadRange(0x5000, 0x6FFF);
	loadRange(0xD000, 0xFFFF);
}

void load2()
{
	loadRange(0x5000, 0x6FFF);
	loadRange(0x9000, 0xBFFF);
	loadRange(0xD000, 0xDFFF);
}

void load3()
{
	loadRange(0x5000, 0x6FFF);
	loadRange(0x9000, 0xAFFF);
	loadRange(0xD000, 0xDFFF);
	loadRange(0xF000, 0xFFFF);
}

void load4()
{
	loadRange(0x5000, 0x6FFF);
	// [memattr] $D000 - $D3FF = RAM 8 // automatic 
}

void load5()
{
	loadRange(0x5000, 0x7FFF);
	loadRange(0x9000, 0xBFFF);
}

void load6()
{
	loadRange(0x6000, 0x7FFF);
}

void load7()
{
	loadRange(0x4800, 0x67FF);
}

void load8()
{
	loadRange(0x5000, 0x5FFF);
	loadRange(0x7000, 0x7FFF);
}

void load9()
{
	loadRange(0x5000, 0x6FFF);
	loadRange(0x9000, 0xAFFF);
	loadRange(0xD000, 0xDFFF);
	loadRange(0xF000, 0xFFFF);
	// [memattr] $8800 - $8FFF = RAM 8 // is this automatic too??? 
}

int fingerprints[] =
{
15702, 0, // 4-TRIS (2001) (Joseph Zbiciak)
11072, 0, // SDK-1600 Movable Object Demo (2002) (Joseph Zbiciak)
10253, 0, // ABPA Backgammon (1978) (Mattel)
9049,  0, // Advanced D&D - Tower of Mystery (1983) (Mattel)
9709,  0, // Advanced D&D - Treasure of Tarmin (1982) (Mattel)
11212, 0, // Advanced Dungeons and Dragons (1982) (Mattel)
11087, 0, // Adventure (AD&D - Cloudy Mountain) (1982) (Mattel)
11426, 0, // Air Strike (1982) (Mattel)
9766,  0, // All-Star Major League Baseball (1983) (Mattel)
10847, 0, // Armor Battle (1978) (Mattel)
9371,  0, // Astrosmash (1981) (Mattel)
9764,  0, // Astrosmash Competition (1981) (Mattel)
10908, 7, // Atlantis (1981) (Imagic)
10218, 0, // Auto Racing (1979) (Mattel)
10894, 0, // B-17 Bomber (1981) (Mattel)
11349, 0, // Baseball (1978) (Mattel)
// (missing Baseball II proto, method 3)
9904,  0, // BeamRider (1983) (Activision)
10538, 7, // Beauty and the Beast (1982) (Imagic)
9337,  0, // Blockade Runner (1983) (Interphase)
10735, 1, // Blowout (1983) (Mattel)
9654,  2, // Body Slam - Super Pro Wrestling (1988) (Intv Corp)
10921, 0, // Bomb Squad (1982) (Mattel)
11614, 0, // Bouncing Pixels (1999) (JRMZ Electronics)
9978,  0, // Boxing (1980) (Mattel)
9379,  0, // Brickout! (1981) (Mattel)
8612,  0, // Bump 'N' Jump (1982-83) (Mattel)
11137, 0, // BurgerTime! (1982) (Mattel)
11765, 0, // Buzz Bombers (1982) (Mattel)
8689,  0, // Carnival (1982) (Coleco-CBS)
11007, 0, // Castle Trailer (2003) (Arnauld Chevallier)
11666, 6, // Centipede (1983) (Atarisoft)
11477, 1, // Championship Tennis (1985) (Mattel)
7102,  0, // Checkers (1979) (Mattel)
// missing Choplifter method 5
// missing Choplifter method 1
9602,  2, // Chip Shot - Super Pro Golf (1987) (Intv Corp)
8402,  2, // Commando (1987) (Mattel)
12615, 5, // Congo Bongo (1983) (Sega)
10329, 0, // Crazy Clones (1981) (PD)
10496, 2, // Deep Pockets-Super Pro Pool and Billiards (1990) (Realtime)
12619, 5, // Defender (1983) (Atarisoft)
10665, 7, // Demon Attack (1982) (Imagic)
12174, 5, // Dig Dug (1987) (Intv Corp)
10444, 2, // Diner (1987) (Intv Corp)
18826, 3, // DK Arcade (1982) (IntelligentVision)
9395,  0, // Donkey Kong (1982) (Coleco)
10004, 0, // Donkey Kong Jr (1982) (Coleco)
14879, 0, // Doom (2002) (Joseph Zbiciak)
10144, 0, // Dracula (1982) (Imagic)
10083, 0, // Dragonfire (1982) (Imagic)
6406,  0, // Dreadnaught Factor, The (1983) (Activision)
6474,  0, // Dreadnaught Factor, The (1983) (Activision)
12407, 0, // Duncan's Thin Ice (1983) (Mattel)
11910, 0, // Easter Eggs (1981) (Mattel)
9499,  0, // Eggs 'n' Eyes by Scott Nudds (1996) (PD)
9775,  0, // Electric Company - Math Fun (1978) (CTW)
9066,  0, // Electric Company - Word Fun (1980) (CTW)
10109, 0, // Fathom (1983) (Imagic)
13433, 0, // Frog Bog (1982) (Mattel)
9676,  0, // Frogger (1983) (Parker Bros)
11101, 0, // GROM, The (1978) (General Instruments)
12756, 9, // Game Factory (Prototype) (1983) (Mattel)
11614, 0, // Go For the Gold (1981) (Mattel)
9926,  0, // Grid Shock (1982) (Mattel)
10621, 0, // Groovy! (1999) (JRMZ Electronics)
6362,  0, // Happy Trails (1983) (Activision) [o1]
6362,  0, // Happy Trails (1983) (Activision)
10224, 0, // Hard Hat (1979) (Mattel)
10009, 0, // Horse Racing (1980) (Mattel)
10552, 2, // Hover Force (1986) (Intv Corp)
10186, 2, // Hover Force 3D (Prototype) (1984) (Mattel)
9109,  0, // Hypnotic Lights (1981) (Mattel)
10266, 0, // Ice Trek (1983) (Imagic)
9533,  0, // Illusions (1983) (Mattel)
10266, 0, // Istar (1983) (Imagic)
11042, 0, // Jetsons, The - Ways With Words (1983) (Mattel)
9273,  1, // King of the Mountain (1982) (Mattel)
8809,  0, // Kool-Aid Man (1983) (Mattel)
9378,  0, // Lady Bug (1983) (Coleco)
11981, 4, // Land Battle (1982) (Mattel)
8281,  0, // Las Vegas Blackjack and Poker (1979) (Mattel)
11001, 0, // Las Vegas Roulette (1979) (Mattel)
6185,  0, // League of Light (Prototype) (1983) (Activision)
15735, 2, // Learning Fun I - Math Master Factor Fun (1987) (Intv Corp)
14363, 2, // Learning Fun II - Word Wizard Memory Fun (1987) (Intv Corp)
10644, 0, // Lock 'N' Chase (1982) (Mattel)
10090, 0, // Lock 'N' Chase (Improved) (1982) (Mattel)
10842, 0, // Loco-Motion (1982) (Mattel)
10304, 0, // Magic Carousel (Prototype) (1983) (Intv Corp)
12439, 0, // Masters of the Universe-The Power of He-Man! (1983) (Mattel)
9802,  0, // Maze Demo #1 (2000) (JRMZ Electronics)
11067, 0, // Maze Demo #2 (2000) (JRMZ Electronics)
10242, 0, // Melody Blaster (1983) (Mattel)
11108, 7, // Microsurgeon (1982) (Imagic)
10643, 0, // Mind Strike! (1982) (Mattel)
11207, 0, // Mine Hunter (2004) (Ryan Kinnen)
11576, 0, // Minotaur (1981) (Mattel)
6217,  0, // Minotaur V1.1 (1981) (Mattel)
6217,  0, // Minotaur (Treasure of Tarmin Hack) (1982) (Mattel)
6498,  0, // Minotaur V2 (1981) (Mattel)
9244,  0, // Mission X (1982) (Mattel)
10085, 0, // MOB Collision Test (2002) (Arnauld Chevallier)
11500, 0, // Motocross (1982) (Mattel)
10251, 0, // Mountain Madness - Super Pro Skiing (1987) (Intv Corp)
9275,  0, // Mouse Trap (1982) (Coleco)
10506, 0, // Mr. Basic Meets Bits 'N Bytes (1983) (Mattel)
11349, 8, // MTE201 Intellivision Test Cartridge (1978) (Mattel)
10637, 0, // NASL Soccer (1979) (Mattel)
9563,  0, // NBA Basketball (1978) (Mattel)
10626, 0, // NFL Football (1978) (Mattel)
9318,  0, // NHL Hockey (1979) (Mattel)
9301,  0, // Night Stalker (1982) (Mattel)
10884, 0, // Nova Blast (1983) (Imagic)
12646, 0, // Number Jumble (1983) (Mattel)
8088,  0, // PBA Bowling (1980) (Mattel)
10071, 0, // PGA Golf (1979) (Mattel)
5049,  5, // Pac-Man (1983) (Atarisoft)
5049,  5, // Pac-Man (1983) (Intv Corp)
11430, 0, // Pinball (1981) (Mattel)
6302,  0, // Pitfall! (1982) (Activision)
10490, 2, // Pole Position (1986) (Intv Corp)
10523, 0, // Pong (1999) (Joseph Zbiciak)
10115, 0, // Popeye (1983) (Parker Bros)
6902,  0, // Q-bert (1983) (Parker Bros)
8238,  0, // Reversi (1984) (Mattel)
10980, 0, // River Raid (1982-83) (Activision)
13155, 0, // Robot Rubble (1983) (Activision)
10270, 0, // Rockey & Bullwinckle (1983) (Mattel)
10090, 0, // Royal Dealer (1981) (Mattel)
9836,  0, // Safecracker (1983) (Imagic)
9923,  0, // Santa's Helper (1983) (Mattel)
// missing Scarfinger method 2
//10547, ?, // Scar Finger (Unfinished) (1983) (Mattel)
10420, 0, // Scooby Doo's Maze Chase (1983) (Mattel)
11292, 0, // Sea Battle (1980) (Mattel)
10760, 0, // Sears Super Video Arcade BIOS (1978) (Sears)
11784, 0, // Sewer Sam (1983) (Interphase)
10336, 0, // Shark! Shark! (1982) (Mattel) [a1]
10335, 0, // Shark! Shark! (1982) (Mattel) 
10738, 0, // Sharp Shot (1982) (Mattel)
9888,  2, // Slam Dunk - Super Pro Basketball (1987) (Intv Corp)
11858, 0, // Slap Shot - Super Pro Hockey (1987) (Intv Corp)
11002, 0, // Snafu (1981) (Mattel)
10692, 0, // Soccer (Prototype) (1979) (Mattel)
10566, 0, // Space Armada (1981) (Mattel)
10018, 0, // Space Battle (1979) (Mattel)
10656, 0, // Space Cadet (1982) (Mattel)
9487,  0, // Space Hawk (1981) (Mattel)
// missing Spaceshuttle method 1
//12490, ?, // Space Shuttle (1981) (Mattel)
10228, 0, // Space Spartans (1981) (Mattel)
11692, 2, // Spiker! - Super Pro Volleyball (1988) (Intv Corp)
11879, 0, // Spirit V1.0 (2003) (Arnauld Chevallier)
10447, 2, // Stadium Mud Buggies (1988) (Intv Corp)
5776,  0, // Stampede (1982) (Activision)
10470, 0, // Star Strike (1981) (Mattel)
9680,  0, // Star Wars - The Empire Strikes Back (1983) (Parker Bros)
8789,  0, // Street (1981) (Mattel)
11675, 0, // Stonix Beta1.1 (2003) (Arnauld Chevallier)
11406, 0, // Stonix Beta1.2 (2003) (Arnauld Chevallier)
10230, 0, // Stonix (2004) (Arnauld Chevallier)
11615, 0, // Sub Hunt (1981) (Mattel)
11402, 0, // Super Cobra (1983) (Konami)
10849, 0, // Super Masters! (1982) (Mattel)
12534, 2, // Super Pro Decathlon (1988) (Intv Corp)
11689, 2, // Super Pro Football (1986) (Intv Corp)
10973, 0, // Super Soccer (1983) (Mattel)
10000, 0, // Swords and Serpents (1982) (Imagic)
10000, 0, // Tag-Along Todd (Ver. 3.13 Beta) (2002) (Joseph Zbiciak)
10575, 0, // TRON - Deadly Discs (1981) (Mattel)
10970, 0, // TRON - Deadly Discs - Deadly Dogs (1987) (Intv Corp)
11495, 0, // TRON - Maze-A-Tron (1981) (Mattel)
11094, 0, // TRON - Solar Sailer (1982) (Mattel)
11888, 0, // Takeover (1982) (Mattel)
9825,  0, // Tennis (1980) (Mattel)
11267, 0, // Tetris (2000) (Joseph Zbiciak)
11832, 0, // Thin Ice (Prototype) (1983) (Intv Corp)
13276, 0, // Thunder Castle (1982) (Mattel)
9992,  3, // Tower of Doom (1986) (Intv Corp)
8777,  0, // Triple Action (1981) (Mattel)
9893,  9, // Triple Challenge (1986) (Intv Corp)
10422, 0, // Tropical Trouble (1982) (Imagic)
10249, 0, // Truckin' (1983) (Imagic)
11365, 0, // Turbo (1983) (Coleco)
9949,  0, // Tutankham (1983) (Parker Bros)
10084, 0, // U.S. Ski Team Skiing (1980) (Mattel)
9871,  4, // USCF Chess (1981) (Mattel)
9784,  0, // Utopia (1981) (Mattel)
10602, 0, // Vectron (1982) (Mattel)
9894,  0, // Venture (1982) (Coleco)
10015, 0, // White Water! (1983) (Imagic)
11411, 0, // World Cup Football (1985) (Nice Ideas)
15043, 1, // World Series Major League Baseball (1983) (Mattel)
10887, 0, // Worm Whomper (1983) (Activision)
11187, 0, // Yogi's Frustration (1983) (Mattel) (unreleased)
11566, 0  // Zaxxon (1982) (Coleco)
};

int getLoadMethod() // lazy, but it works
{
	int i;
	int fingerprint = 0;
	// find fingerprint
	for(i=0; i<256; i++)
	{
		fingerprint = fingerprint + data[i];
	}
	printf("[INFO] [FREEINTV] Cartridge fingerprint code: %i\n", fingerprint);
	
	// find load method
	for (i=0; i<380; i+=2)
	{
		if(fingerprint==fingerprints[i])
		{
			printf("[INFO] [FREEINTV] Cartridge database match: memory map %i\n", fingerprints[i+1]);
			if(fingerprint==11349)
			{
				// Baseball or MTE Test Cart?
				if(size>8192) { return 8; } // load method 8 for MTE Test Cart
				return 0; // default method for BaseBall
			}
			return fingerprints[i+1];
		}
	}
	return -1;
}
