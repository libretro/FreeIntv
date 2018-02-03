/* this code so totally sucks it's not even funny. 
   i'm amazed it works at all. */

#define MAXDURA (2047)
#define MAXDURB (15)
#define WMASK   (0xFF)


/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License       */
/*  along with this program; if not, write to the Free Software             */
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/* ======================================================================== */
/*                   Copyright (c) 2000, Joseph Zbiciak                     */
/* ======================================================================== */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAXNOTE   (8)
#define MAXACTIVE (32)
#define PAD (19)

int xpose[32] = 
{ 
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

int volume[32] = 
{ 
    256, 256, 256, 256, 256, 256, 256, 256, 
    256, 256, 256, 256, 256, 256, 256, 256,
    256, 256, 256, 256, 256, 256, 256, 256, 
    256, 256, 256, 256, 256, 256, 256, 256
};

int always_zero = 1;
int zero_merge = 1;
int allow_bref = 1;
int slot_offset = 0;
int slot_mult = 1;
int offset = 0;
int allow_sloppy = 1;
int always_volume = 0;
int always_fullword = 0;
int fullword_if_nonzero = 0;
int allow_finetune = 0;
int differentiate_channels = 1;
int space = 0;
int tempo = 545455;
int division = 192;
unsigned dont_care = 0x0688;    /* No noise or envelope support */
int brefs = 0;
unsigned orgs[16] = { 0xD000, 0xF000, 0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

typedef struct note_t
{
    char    note[MAXNOTE];
    int period;
    char    pref[MAXACTIVE];
    int hits;
} note_t;

/*
    | |   | | |
   | | | | | | | |
   c d e f g a b c
*/
char *name[12] = 
{ 
    "A", "A#", "B", 
        "B#", "C#", "D", 
        "D#", "E", "F", 
        "F#", "G", "G#" 
};

note_t note_tbl[120];

void init_note(void)
{
    double clock = 3597545. / 32.;
    double freq;
    int period;
    int i;
    int o;
    int n;
    char note[MAXNOTE];

    for (o = i = 0; o < 10; o++)
    {
        for (n = 0; n < 12; n++, i++)
        {
            sprintf(note, "%s%d", name[n], o);
            freq = (55./4.) * pow(2.0, ((double)i)/12.);
            period = clock / freq + 0.5;

/*          printf("%6s = %10.3f Hz = %d = %.8X\n",
                note, freq, period, period);*/

            note_tbl[i].period = period;
            strncpy(note_tbl[i].note, note, MAXNOTE);
            note_tbl[i].hits = i;
        }
    }
}
/*
    |   | |   | | |
   | | | | | | | | | | |
   a b c d e f g a b c
*/
        /* A  B  C  D  E  F  G  */
int gettbl[7] = { 12,14, 3, 5, 7, 8, 10 };


int getnoteidx(char *note, int chan)
{
        int n = 0, o = 0, i = 0, l = 0;

    l = note[0] - 'A';
    if (l < 0) l = 0;
    if (l > 11) l = 11;
        n = gettbl[l];
        if      (note[1] == '#') { n++; o = note[2] - '0'; }
        else if (note[1] == 'b') { n--; o = note[2] - '0'; }
        else                     {      o = note[1] - '0'; }

        n -= xpose[chan];

        i = 12 * o + n;

/*      printf("note = %-4s  n = %2d o = %2d i = %3d --> %-4s\n",
                note, n, o, i, note_tbl[i].note);*/
//printf("note = %-4s  n = %2d o = %2d i = %3d ch = %2d xpose = %2d --> %-4s\n",
//note, n, o, i, chan, xpose[chan], note_tbl[i].note); fflush(stdout);

    if (i < 0) i = 0;
    if (i > 119) i = 119;
 
    //note_tbl[i].hits++;
        return i;
}

int getnote(char *note, int chan)
{
    return note_tbl[getnoteidx(note, chan)].period;
}

typedef struct active_t
{
    char    note[MAXNOTE];
    int chan;
    int movable;
    int vel;
    int time;
} active_t;

active_t prev_active[MAXACTIVE];
active_t curr_active[MAXACTIVE];

typedef struct streamrec_t
{
    char    note[MAXACTIVE][MAXNOTE];
    int     chan[MAXACTIVE];
    int     per [MAXACTIVE];
    int vol [MAXACTIVE];
    int vel [MAXACTIVE];
    int dur;
    int time;
    struct streamrec_t *next;
} streamrec_t;

streamrec_t stream_dummy;
streamrec_t *stream_head = &stream_dummy, *stream_tail = &stream_dummy;


void init_active(void)
{
    int i;

    for (i = 0; i < MAXACTIVE; i++)
        curr_active[i].vel = curr_active[i].time = 
        curr_active[i].note[0] = 0;
    memcpy(prev_active, curr_active, sizeof(prev_active));
}

void advance_active(void)
{
    int i;
    memcpy(prev_active, curr_active, sizeof(prev_active));
    for (i = 0; i < MAXACTIVE; i++)
    {
        if (curr_active[i].vel == 0) 
            curr_active[i].note[0] = 0;
        curr_active[i].movable = 0;
    }
}

int find_match(char *note, int chan, int *movable)
{
    unsigned inactive = 0;
    int pref;
    int i;
    int n;
    int redundant = 0, last_inactive = -1;

    n = getnoteidx(note, chan);
    *movable = 0;

    for (i = MAXACTIVE - 1; i >= 0; i--)
    {
        if ( !strncmp(curr_active[i].note, note, MAXNOTE))
        {
            if (chan == curr_active[i].chan)
            {
                note_tbl[n].pref[i]++;
                return i;
            } else
                redundant = 1;
        }
        if (curr_active[i].vel == 0)
        {
            inactive |= 1<<i;
            last_inactive = i;
            continue;
        }
    }

    /* No inactive channels?  Return error. */
    if (inactive == 0)
        return -1;

    pref = -1;

    /* find preferred channel for note */
    if (!redundant)
    {
        for (i = 0; i < MAXACTIVE; i++)
        {
            if ((inactive >> i) & 1) 
            {
                pref = i; 
                break;
            }
        }
    } else
    {
        pref = last_inactive;
    }

    if (pref == -1) return -1; /* error */

    for (i = pref + 1; i < MAXACTIVE; i++)
    {
        if (!((inactive >> i) & 1)) continue;
        if (note_tbl[n].pref[i] > note_tbl[n].pref[pref])
            pref = i;
        break;
    }

    *movable = 1;
    return pref;
}

const double ratefix = 15000.;
int time_to_ticks(int Time)
{
    return ((double)Time * tempo + ratefix/2.0) / (ratefix * division); 
}
double ticks_to_time(int Time)
{
    return ((double)Time * (ratefix * division)) / tempo;
}

void handle_note(char *note, int vel, int Time, int chan)   
{
    int slot, movable = 0;

    slot = find_match(note, chan, &movable);

    //printf ("Note %6s=%-6d Time=%-6d chan=%-6d Slot=%-2d\n", note, vel, Time,chan, slot); fflush(stdout);
//printf ("Note %6s=%-6d Time=%-6d chan=%-6d Slot=%-2d\n", note, vel, Time,chan, slot); fflush(stdout);

    if (slot < 0)
    {
        fprintf(stderr, "Too many active voices.\n");
        exit(1);
    }

    strncpy(curr_active[slot].note, note, MAXNOTE);
    curr_active[slot].chan    = chan;
    curr_active[slot].vel     = vel;
    curr_active[slot].time    = Time;
    curr_active[slot].movable = movable && vel > 0;
}

void emit_stream_record(int *last_time, int curr_time)
{
    int i,j,k,vel,moves = 0;
    streamrec_t *ptr = NULL;

    for (i = 1; i < MAXACTIVE - 1; i++)
    {
        int n1, n2;

        if (curr_active[i].vel == 0 || !curr_active[i].movable) 
            continue;

        n1 = getnoteidx(curr_active[i].note, curr_active[i].chan);
        for (j = 0; j < i; j++)
        {
            if (curr_active[j].vel == 0) continue;

            if (!(curr_active[i].movable|curr_active[j].movable))
                continue;

            n2 = getnoteidx(curr_active[j].note, 
                    curr_active[j].chan);
            /* If these are an octave apart, move later
             * one all the way to the end.
             */
            if ((n1 - n2) % 12 == 0)
            {
                for (k = i + 1; k < MAXACTIVE; k++)
                {
                    if (curr_active[k].vel == 0)
                    {
                        curr_active[k] = curr_active[i];
                        curr_active[i].vel = 0;
                        moves++;
                        goto next;
                    }
                }
                /* if we get here, we couldn't move it.
                 * oh well.
                 */
            }
        }
        next:;
    }

    if (moves)
    {
        /* compact our set of active channels */
        for (i = 0; i < MAXACTIVE - 1; i++)
        {
            if (curr_active[i].vel) continue;
            for (j = i + 1; j < MAXACTIVE; j++)
            {
                if (curr_active[j].vel &&
                    curr_active[j].movable)
                {
                    curr_active[i] = curr_active[j];
                    curr_active[j].vel = 0;
                    break;
                }
            }
            if (j == MAXACTIVE) break;
        }
    }

    for (i = 0; i < MAXACTIVE; i++)
    {
        if (!prev_active[i].vel && !curr_active[i].vel)
            continue;

        if (prev_active[i].vel == curr_active[i].vel &&
            !strcmp(prev_active[i].note, curr_active[i].note))
            continue;

        if (!ptr && (ptr = calloc(1,sizeof(streamrec_t))) == NULL)
        {
            fprintf(stderr, "Out of memory in emit()\n");
            exit(1);
        }
        
        strncpy(ptr->note[i], curr_active[i].note, MAXNOTE);
        vel = (curr_active[i].vel * volume[curr_active[i].chan]) >> 8;
        ptr->vel[i] = vel;
        ptr->chan[i] = curr_active[i].chan;
        if (vel > 0)
        {
            ptr->vol[i] = 3.1 * log(vel);
        } else
            ptr->vol[i] = ptr->per[i] = 0;

        if (vel > 0 || !always_zero)
            ptr->per[i] = getnote(ptr->note[i], ptr->chan[i]);
        
    }
    if (ptr)
    {
        ptr->time = curr_time;
    /*  ptr->dur = time_to_ticks(curr_time)-time_to_ticks(*last_time);*/
        stream_tail->next = ptr;
        stream_tail = ptr;
        *last_time = curr_time;
    }
}

double calc_abs_error(int t)
{
    int orig_tempo = tempo;
    streamrec_t *ptr = stream_head->next;
    double err = 0.0;

    tempo = t;

    while (ptr)
    {
        err += abs(ptr->time-ticks_to_time(time_to_ticks(ptr->time)));
        ptr = ptr->next;
    }

    tempo = orig_tempo;
    return err;
}

void set_record_durations(void)
{
    streamrec_t *ptr = stream_head->next;
    int last_time = 0;
    int lo_tempo, hi_tempo, best_tempo;
    int t;
    double abs_error, best_abs_error;

    if (allow_finetune)
    {
        lo_tempo = 0.97 * tempo;
        hi_tempo = 1.03 * tempo;

        best_tempo = tempo;
        best_abs_error = calc_abs_error(tempo);
        
        for (t = lo_tempo; t <= hi_tempo; t++)
        {
            if (t == tempo) continue;
            abs_error = calc_abs_error(t);
            if (abs_error < best_abs_error)
            {
                best_tempo = t;
                best_abs_error = abs_error;
            }
        }
        if (best_tempo != tempo)
        {
            printf("; Tempo finetuned to %d\n", best_tempo);
            tempo = best_tempo;
        }
    }

    while (ptr)
    {
        ptr->dur = time_to_ticks(ptr->time)-time_to_ticks(last_time);
        last_time = ptr->time;
        ptr = ptr->next;
    }
}
        

int parse_line(char *line)
{   
    char note[MAXNOTE], *s, *n;
    int x;
    int vel = 0;
    int Time = 0, chan = 1;
    static int last_time = 0;
    static int curr_time = 0;
    static int time_ofs = -1;
    
    if (!line)
    {
        emit_stream_record(&last_time, last_time);
        return 0;
    }

    if (line[0] == 't' && strncmp(line, "tempo", 5) == 0)
    {
        sscanf(line, "tempo=%d division=%d", &tempo, &division);
        return 0;
    }

    if (line[0] == 's' && strncmp(line, "space", 5) == 0)
    {
        sscanf(line, "space=%d", &space);
        return 0;
    }

    if (line[0] == 'v' && strncmp(line, "volume", 6) == 0)
    {
        int n, x, y;
        n = sscanf(line, "volume=%d,%d", &x, &y);
        if (n == 2)
        {
            volume[x] = y;
        } else if (n == 1)
        {
            for (y = 0; y < 32; y++)
                volume[y] = x;
        } else
        {
            fprintf(stderr,"Bad volume\n");
            exit(1);
        }
        return 0;
    }

    if (line[0] == 'd' && strncmp(line, "differentiate_channels", 22) == 0)
    {
        sscanf(line, "differentiate_channels=%d",
                 &differentiate_channels);
        return 0;
    }

    if (line[0] == 's' && strncmp(line, "slot_mult", 9) == 0)
    {
        sscanf(line, "slot_mult=%d", &slot_mult);
        return 0;
    }

    if (line[0] == 't' && strncmp(line, "transpose", 9) == 0)
    {
        int n, x, y;
        n = sscanf(line, "transpose=%d,%d", &x, &y);
        if (n == 2)
        {
            xpose[x] = y;
        } else if (n == 1)
        {
            for (y = 0; y < 32; y++)
                xpose[y] = x;
        } else
        {
            fprintf(stderr,"Bad transpose\n");
            exit(1);
        }
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "allow_finetune", 14) == 0)
    {
        sscanf(line, "allow_finetune=%d", &allow_finetune);
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "always_zero", 11) == 0)
    {
        sscanf(line, "always_zero=%d", &always_zero);
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "always_volume", 13) == 0)
    {
        sscanf(line, "always_volume=%d", &always_volume);
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "always_fullword", 15) == 0)
    {
        sscanf(line, "always_fullword=%d", &always_fullword);
        return 0;
    }

    if (line[0] == 'f' && strncmp(line, "fullword_if_nonzero", 19) == 0)
    {
        sscanf(line, "fullword_if_nonzero=%d", &fullword_if_nonzero);
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "allow_bref", 10) == 0)
    {
        sscanf(line, "allow_bref=%d", &allow_bref);
        return 0;
    }

    if (line[0] == 'a' && strncmp(line, "allow_sloppy", 12) == 0)
    {
        sscanf(line, "allow_sloppy=%d", &allow_sloppy);
        return 0;
    }

    if (line[0] == 'z' && strncmp(line, "zero_merge", 10) == 0)
    {
        sscanf(line, "zero_merge=%d", &zero_merge);
        return 0;
    }

    if (line[0] == 'o' && strncmp(line, "offset", 6) == 0)
    {
        sscanf(line, "offset=%x", &offset);
        return 0;
    }

    if (strncmp(line, "Chn", 3) || !strstr(line, "Note O"))
        return -2;

    if (differentiate_channels) sscanf(line + 4, "%d", &chan);
    s = line;
    x = 0;
    while (*s && x < 4) x += *s++ == ' ';

    if (!*s) return -1;

    x = 1;
    n = note;
    while (*s && *s != '=' && x++ < MAXNOTE) *n++ = *s++;
    while (*s && *s != '=') s++;
    *n = 0;

    if (!*s++ || !*s) return -1;

    x = sscanf(s, "%d Time=%d", &vel, &Time);

    if (x != 2) return -1;
    if (strstr(line, "Note Off")) vel = 0;

    if (time_ofs == -1) time_ofs = Time; else Time -= time_ofs;
    

        if (vel == 0) Time -= space;
        if (Time < last_time) Time = last_time;


    curr_time = Time;

    if (curr_time != last_time)
    {
        emit_stream_record(&last_time, curr_time);
        advance_active();
    }

    handle_note(note, vel, Time, chan);

    return 0;
}


void generate_regs(streamrec_t *rec, 
           unsigned char reg[14]) 
{
    int i, j;

    for (i = 0; i < 3; i++)
    {
        j = i * slot_mult + slot_offset;
        if (rec->note[j][0])
        {
            reg[i   ] = rec->per[j] & 0xFF;
            reg[i+ 4] =(rec->per[j] >> 8) & 0xFF;
            reg[i+11] = rec->vol[j];
        }
    }
}

int regs_semantically_equiv(unsigned char reg1[14], unsigned char reg2[14])
{
    int i;
    if (allow_sloppy == 0) return memcmp(reg1, reg2, 14) == 0;

    for (i = 0; i < 3; i++)
    {
        /* Check volume */
        if (reg1[11+i] != reg2[11+i]) return 0;

        /* Check period if volume != 0 */
        if (reg1[11+i] &&
            (reg1[i] != reg2[i] || reg1[i+4] != reg2[i+4]))
            return 0;
    }

    /* Check other miscellaneous */
    if (reg1[ 8] != reg2[ 8])
        return 0;

    return 1;
}

int decles = 0;

//#define WMASK (0xFF)
unsigned window[65536]; /* Only lower 8 bits significant on brefs */
int w_ptr = 0;
#define W(x) window[((x) & WMASK)]

unsigned WP(int x) 
{ 
    int y = x >> 1, z = x & 1; 
    unsigned v = W(y);
    return 0xFF & (z ? (v >> 8) : v);
}

unsigned packed_byte  = 0;
int      packed_count = 0;
char    *packed_cmt1  = NULL;
char    *packed_cmt2  = NULL;

void emit_comment(char *cmt) 
{ 
    printf("                        ;        %s\n", cmt);
}

void flush_packed_bytes(void)
{
    if (packed_count == 0) return;

    printf("        DECLE   $%.4X   ; $%.4X: %s\n", 
           packed_byte & 0xFFFF, decles, packed_cmt1 ? packed_cmt1 : "");
    if (packed_cmt2 && strlen(packed_cmt2) > 0)
        printf("                        ;        %s\n", packed_cmt2);

    W(w_ptr++) = packed_byte & 0xFFFF;
    decles++;

    if (packed_cmt1) { free(packed_cmt1); packed_cmt1 = NULL; }
    if (packed_cmt2) { free(packed_cmt2); packed_cmt2 = NULL; }
    packed_byte  = 0;
    packed_count = 0;
}


void emit_packed_byte(unsigned byte, char *cmt)
{
    packed_byte = packed_byte | ((0xFF & byte) << ((packed_count++) << 3));
   
    if (cmt || strlen(cmt) > 0)
        *(packed_count == 1? &packed_cmt1 : &packed_cmt2) = strdup(cmt);

    if (packed_count == 2) flush_packed_bytes();
}

void emit_decle(unsigned decle, char *cmt) 
{ 
    printf("        DECLE   $%.4X   ; $%.4X: %s\n", 
           decle & 0xFFFF, decles, cmt);
    W(w_ptr++) = decle & 0xFFFF;
    if (decle & ~0xFFFF)
        fprintf(stderr, "Warning: %.8X exceeds DECLE range\n", decle);
    decles++;
}

void emit_byte (unsigned byte, char *cmt)  
{ 
    printf("        BYTE    $%.2X     ; $%.4X: %s\n", byte & 0xFF, decles, cmt);
    W(w_ptr++) = byte  & 0xFF; 
    if (byte & ~0xFF)
        fprintf(stderr, "Warning: %.8X exceeds BYTE range\n", byte);
    decles++;
}
void emit_word(unsigned word, char *cmt)
{
    printf("        WORD    $%.4X   ; $%.4X: %s\n", word & 0xFFFF, decles, cmt);
    W(w_ptr++) = word & 0xFF;
    W(w_ptr++) = (word >> 8) & 0xFF;
    if (word & ~0xFFFF)
        fprintf(stderr, "Warning: %.8X exceeds WORD range\n", word);
    decles += 2;
}

void emit_solid_line(void)
{
    printf(";--------------------------------------"
        "--------------------------------------\n");
}
void emit_dashed_line(void)
{
    printf("; - - - - - - - - - - - - - - - - - - -"
        " - - - - - - - - - - - - - - - - - - -\n");
}

int find_compatible_bref
(
    unsigned *control, 
    unsigned prev_control, 
    unsigned char reg[14],
    unsigned char regs[14],
    int all_zero,
    int *pctl,
    int *sloppy
)
{
    int l_bound = 0;
    int u_bound = 0;
    int i, j, k, c_bits;
    int byte;
    unsigned maybe_control, tmp;
    int d_bits, l, min_d_bits = 14;

    for (c_bits = 0, tmp = *control; tmp; c_bits++) tmp &= tmp - 1;

    *sloppy = 0;

    u_bound = w_ptr - (c_bits >> 1);
    l_bound = w_ptr - WMASK;

    if (u_bound <= 1) return -1;
    if (l_bound < 0) l_bound = 0;

    /* First, if we are able to do a PCTL, just see if the
     * byte pattern matches our needs. 
     */
    if ((*control & ~(prev_control | dont_care)) == 0 && !all_zero)
    {
        for (i = u_bound; i >= l_bound; i--)
        { 
//printf("trying PCTL = 1 @ %d\n", i);
            memcpy(regs, reg, 14);
            for (j = 0, k = (i << 1); j < 14; j++)
            {
                if (!(1 & (prev_control >> j)))
                    continue;
                
                if ((k >> 1) >= w_ptr) goto nopers2;

                regs[j] = all_zero ? 0 : WP(k++);
            }
            if (!regs_semantically_equiv(reg, regs))
//{
//for(j=0;j<14;j++)
//printf("reg[%.2d] = $%.2X  vs  $%.2X\n", j, reg[j], regs[j]);
//printf("nopers2\n");
                goto nopers2;
//}
            *control = prev_control;
            *pctl = 2;
            return w_ptr - i - 1;
            nopers2:;
        }
    }

    for (l = 0; l < 14; l = min_d_bits++)
    for (i = u_bound + 2; i >= l_bound; i--)
    {
//printf("trying PCTL = 0 @ %.4X\n", i);
        /* Next see if there is a compatible control word */
        maybe_control = W(i);

        if ((maybe_control & 0xC000) == 0 &&
            (*control & ~(maybe_control | dont_care)) == 0 &&
                    i + 1 <= w_ptr)
        { 
            tmp = maybe_control ^ *control;

//printf("; maybe_control = %.8X  control = %.8X\n", maybe_control, *control);
            memcpy(regs, reg, 14);
            for (j = 0, k = (i + 1) << 1; j < 14; j++)
            {
                if (!(1 & (maybe_control >> j)))
                    continue;
                
                if ((all_zero && reg[j]))
                {
                    //printf("; nope1\n");
                    goto nopers;
                }
                if (all_zero == 0 && (k >> 1) >= w_ptr)
                {
                    //printf("; nope2\n");
                    goto nopers;
                }
                regs[j] = all_zero ? 0 : WP(k++);
            }
            if (!regs_semantically_equiv(reg, regs))
            {
//for(j=0;j<14;j++)
//printf("reg[%.2d] = $%.2X  vs  $%.2X\n", j, reg[j], regs[j]);
                //printf("; nope3\n"); 
                goto nopers;
            }
            for (d_bits = 0; tmp; d_bits++) tmp &= tmp - 1;
            if (d_bits > l) 
            { 
                if (d_bits < min_d_bits) min_d_bits = d_bits;
                goto nopers; 
            }
            
            if (memcmp(reg, regs, 14) != 0) 
            {
                *sloppy = 1;
                memcpy(reg, regs, 14);
            } else
            {
                *sloppy = 0;
            }
            *control = maybe_control;
            *pctl = 0;
            return w_ptr - i - 1;
        }
    nopers:;

    }

    return -1;
}

const char *psg_name[14] = 
{
        "ChnA Per LSB",
        "ChnB Per LSB",
        "ChnC Per LSB",
        "Evlp Per LSB",
        "ChnA Per MSB",
        "ChnB Per MSB",
        "ChnC Per MSB",
        "Envp Per MSB",
        "Channel Enables",
        "Noise Period",   
        "Envelope Trigger",   
        "ChnA Volume", 
        "ChnB Volume", 
        "ChnC Volume"
};

const char *ctl_name[14] = 
{
    "AL", "BL", "CL", "EL",
    "AH", "BH", "CH", "EH",
    "Ch", "NP", "ET", 
        "AV", "BV", "CV"
};

int total_records = 0;

void ctrl_cmt(unsigned ctrl, char *cmt_buf)
{
    int i;

    cmt_buf[43] = 0;
    cmt_buf[0]  = '[';
    cmt_buf[42] = ']';

    for (i = 0; i < 13; i++)
        cmt_buf[3 + i*3] = '|';

    for (i = 0; i < 14; i++)
    {
        if ((ctrl >> i) & 1)
        {
            cmt_buf[40 - i*3] = ctl_name[i][0];
            cmt_buf[41 - i*3] = ctl_name[i][1];
        } else
        {
            cmt_buf[40 - i*3] = cmt_buf[41 - i*3] = '-';
        }
    }
}

int output_record1(unsigned char reg[14], unsigned char oreg[14], 
                   int dur, int *rdur)
{
    unsigned char regs[14];
    unsigned int control = 0, zcontrol = 0;
    int i, all_zero = 4, do_ctrl, do_zctrl, pctl;
    int num_zero = 0, zcost, num_ctrl = 0, do_xctrl, do_zero = 0;
    int bref, need_link = 0;
    static unsigned prev_control = ~0U, bound = 8192, links = 0;
    char cmt_buf[64];

    if (dur == -1) { prev_control = ~0U; return 0; }

    *rdur = dur;

    /* If previous control word had link bit set, force new control word */
    if (prev_control & 0x4000) prev_control = ~0U;

    memcpy(regs, reg, 14);

    /* If we're near one of the magic ROM boundaries, force us
         * to emit a link record.
     */
    if (bound > (decles + offset) && bound - (decles + offset) < PAD) 
    {
        control |= 0x4000;
        need_link = 1;
    }

    if (regs_semantically_equiv(reg, oreg))
    {
        all_zero = 4;
        for (i = 0; i < 14; i++)
        {
            if ((prev_control & (1<<i)) && reg[i])
                break;
        }
    zcontrol = control = need_link ? 0x4000 : i == 14 ? prev_control : 0;
    } else for (i = 0; i < 14; i++)
    {
//printf("reg[%2d] = %.2X  oreg[%2d] = %.2X\n", i, reg[i], i, oreg[i]);
        if (reg[i] != oreg[i])
        {
            control |= 1 << i;
            if (reg[i]) all_zero = 0; 
            else 
            {
                zcontrol |= 1 << i;
                num_zero++;
            }
        }
    }

    if (all_zero==0 && always_volume) for (i = 11; i < 14; i++)
    {
        if (reg[i]) control |= 1 << i;
    }

    if (all_zero==0 && always_fullword) for (i = 0; i < 3; i++)
    {
        if (reg[i + 11] && (control & (0x11 << i)))
            control |= 0x11;
    }


    if (all_zero==0 && fullword_if_nonzero) for (i = 0; i < 3; i++)
    {
        if (reg[i + 11] && reg[i+4] && (control & (0x01 << i)))
            control |= 0x11;
        if (reg[i + 11] && reg[i  ] && (control & (0x10 << i)))
            control |= 0x11;
    }


//printf("control = %.8X  zcontrol = %.8X  dont_care = %.8X\n", control, zcontrol, dont_care);
    control  &= ~dont_care;
    zcontrol &= ~dont_care;
    /*if (!control) return 0;*/

    num_ctrl = do_ctrl = ((prev_control ^ control) & ~dont_care) != 0;

    if (all_zero && do_ctrl && (control & ~(prev_control|dont_care)) == 0)
    {
        unsigned dc_bits = dont_care;
        unsigned remain;

        for (i = 0; i < 3; i++)
            if (reg[i + 11] == 0 || (control & (0x800 << i)))
                dc_bits |= (0x0811) << i;

        remain = prev_control & ~(control | dc_bits);
        for (i = 0; i < 14; i++)
            if (((remain >> i) & 1) && reg[i]) break;
        if (i == 14) 
        {
            do_ctrl = 0;
            for (i = 0; i < 14; i++)
                if ((prev_control >> i) & 1)
                    reg[i] = 0;
        }
    }

    pctl = 2 * !do_ctrl;
    if (pctl) control = prev_control;

    if (!need_link && allow_bref && (!pctl || !all_zero))
    {
        int sloppy = 0;

        bref = find_compatible_bref(&control, prev_control, reg, regs,
                        all_zero, &pctl, &sloppy);
        if (bref >= 0)
        {
            if (dur > MAXDURB) dur = MAXDURB;
            *rdur = dur;
            brefs++;
            if (pctl)
                sprintf(cmt_buf, "Dur = %.2X%s BREF PCTL "
                    "(ctl=$%.4X)", dur,
                    all_zero ? " ZERO":"", prev_control);
            else
                sprintf(cmt_buf, "Dur = %.2X%s BREF "
                    "(ctl=$%.4X)", dur,
                    all_zero ? " ZERO":"", control);
            emit_decle((bref << 8)|(dur << 4) | all_zero | pctl | 1, cmt_buf);
            sprintf(cmt_buf, "Back Ref: $%.4X%s%s", 
                decles - bref - 2, 
                sloppy ? " (sloppy)":"", 
                control & dont_care ? " (sends dc)" : "");
            /*emit_decle(bref, cmt_buf);*/
            emit_comment(cmt_buf);
            prev_control = control;
            total_records++;

            for (i = 0; i < 14; i++)
                if (1 & (control >> i))
                    oreg[i] = regs[i];

            return 0;
        }
    }

    do_zctrl = 1;
    do_xctrl = ((prev_control ^ (control & ~zcontrol)) & ~dont_care) != 0;
    zcost = 4;
    if (!all_zero && num_zero > zcost)
    {
        do_zero = 1;

        if (do_zctrl && need_link) zcontrol |= 0x4000;

        control  &= ~zcontrol;
        num_ctrl = do_ctrl = prev_control != control;
        pctl = 2 * !do_ctrl;
        num_ctrl += do_zctrl;
    }


    /* see if prev_control and control differ by only one bit. */
    if (0 && do_ctrl)
    {
        unsigned diff = prev_control & ~control;
        unsigned xor = prev_control ^ control;
        if (xor == diff)
        {

            for (i = 0; i < 14; i++)
            {
                if ((1 << i) == diff) break;
            }
            if ((1 << i) == diff) /* yes they do. */
            {
                /* make sure not to upset all_zero */
                if (all_zero==0 || reg[i]==0)
                {
                    control = prev_control;
                    do_ctrl = 0;
                    pctl = 2;
                    num_ctrl--;
                }
            }
        }
    }

    if (pctl)
        sprintf(cmt_buf, "Dur = %.2X%s PCTL (ctl=$%.4X)", 
            do_zero ? 0 : dur, all_zero ?" ZERO":"", prev_control);
    else
        sprintf(cmt_buf, "Dur = %.2X%s ", 
                        do_zero ? 0 : dur, all_zero ?" ZERO":"");

    emit_decle((do_zero ? 0 : (dur << 4)) | all_zero | pctl, cmt_buf);

    if (do_ctrl) 
    {
        ctrl_cmt(control, cmt_buf);
        //emit_word(control, cmt_buf);
        emit_decle(control, cmt_buf);
    }

    if (all_zero == 0)
    {
        for (i = 0; i < 14; i++)
        {
            if ((control >> i) & 1) 
            {
                sprintf(cmt_buf, "REG #%-2d  %s", 
                    i, psg_name[i]);
                emit_packed_byte(reg[i], cmt_buf);
                oreg[i] = reg[i];
            }
        }
        flush_packed_bytes();
    }  
    else
        for (i = 0; i < 14; i++)
            if ((control >> i) & 1) 
                oreg[i] = 0;

    prev_control = control;

    if (do_zero)
    {
        /* do a zero record w/ zero hold */
        emit_dashed_line();
        printf("        ; Postzero %d, cost %d\n", num_zero, zcost);
        if (do_zctrl)
            sprintf(cmt_buf, "Dur = %.2X ZERO (postzero)", dur);
        else
            sprintf(cmt_buf, "Dur = %.2X ZERO PCTL"
                                " (ctl=$%.4X) (postzero)", dur, prev_control);
        emit_decle((dur << 4) | 4 | (2*!do_zctrl), cmt_buf);
        if (do_zctrl) 
        {
            ctrl_cmt(zcontrol, cmt_buf);
            //emit_word(zcontrol, cmt_buf);
            emit_decle(zcontrol, cmt_buf);
        }
        prev_control = zcontrol;
        for (i = 0; i < 14; i++)
            if ((zcontrol >> i) & 1) 
                oreg[i] = 0;
    }

    if (need_link)
    {
        decles++;
        printf("        DECLE   @@link_%d\n", links);
        assert(bound >= (offset + decles));
        emit_dashed_line();
        while (bound - (offset + decles))
            emit_byte(0, "filler");
        emit_dashed_line();
        printf("        ORG     $%.4X\n", orgs[links]);
        printf("@@link_%d:\n", links++);

        if (orgs[links] == 0x9000) bound += 8192;
        else                       bound += 4096;
        w_ptr = 0;
        printf("\n");
    }


    total_records++;
    return num_ctrl;
}

int output_record(unsigned char *reg, unsigned char *oreg, int dur)
{
    int x = 0;
    int rdur;

    emit_dashed_line();
    while (dur > MAXDURA)
    {
        x += output_record1(reg, oreg, MAXDURA, &rdur);
        dur -= rdur;
        emit_dashed_line();
    }

    do {
        x += output_record1(reg, oreg, dur, &rdur);
        dur -= rdur;
        if (dur > 0) emit_dashed_line();
    } while (dur > 0);

    emit_solid_line(); printf("\n\n");
    emit_solid_line();

    return x;
}

void dump_song_records(void)
{
    streamrec_t *rec;
    int i, j, time = 0;
    streamrec_t current;

    emit_solid_line();

    printf("tempo=%d division=%d\n", tempo, division);
    printf("always_zero=%d\n", always_zero);
    printf("zero_merge=%d\n", zero_merge);
    printf("allow_bref=%d\n", allow_bref);
//  printf("slot_offset=%d\n", slot_offset);
//  printf("slot_mult=%d\n", slot_mult);
    printf("offset=%d\n", offset);
    printf(";space=%d\n", space);
    printf("allow_sloppy=%d\n", allow_sloppy);
    printf("always_volume=%d\n", always_volume);
    printf("allow_finetune=%d\n", allow_finetune);
    printf("differentiate_channels=%d\n", differentiate_channels);

    rec = stream_head->next;

    j = 1;
    time = 0;
    memset(&current, 0, sizeof(current));
    while (rec)
    {
        if (j) emit_dashed_line();
        j = 0;
        for (i = 0; i < MAXACTIVE; i++)
        {
            if (rec->note[i][0] && current.note[i][0] &&
                strcmp(rec->note[i], current.note[i]))
            {
                printf("Chn %d Note On %s=%*d Time=%-8d "
                                       "; SLOT %d\n",
                    current.chan[i], current.note[i],
                    strlen(current.note[i])-MAXNOTE, 0,
                                        time, i);
                current.note[i][0] = 0;
            }
        }
        for (i = 0; i < MAXACTIVE; i++)
        {
            if (rec->note[i][0])
            {
                printf("Chn %d Note On %s=%*d Time=%-8d "
                                       "; SLOT %d\n",
                    rec->chan[i], rec->note[i], 
                    strlen(rec->note[i])-MAXNOTE, 
                                        rec->vel[i], time, i);
                if (rec->vel[i])
                    strcpy(current.note[i], rec->note[i]);
                else
                    current.note[i][0] = 0;
                current.chan[i] = rec->chan[i];
                j = 1;
            }
        }
        time = rec->time;
        rec = rec->next;
    }
    
    emit_solid_line();
}   

int main(int argc, char *argv[])
{
    char buf[1024];
    streamrec_t *rec, *prec;
    int tot, dur;
    unsigned char reg[14], oreg[14], mreg[14];
    unsigned char reg2[14], oreg2[14];
    int merges = 0, all_zeros = 0, num_ctrl = 0;
    int semantic_merges = 0, zero_merges = 0, was_merge = 0;
    int lineno = 0, parse;
    char *merge = NULL;
    int i, song2 = 0, dump_song = 0;

    if (argc == 2)
    {
        if (argv[1][0] == '-') song2 = 1;
        if (argv[1][0] == 's') dump_song = 1;
    }
        

    init_note();
    init_active();

    while (fgets(buf, 1024, stdin) != NULL)
    {
        lineno++;
        parse = parse_line(buf);
        if (parse == -1)
        {
            fprintf(stderr, "parse error line %d: %s\n",lineno,buf);
            exit(1);
        }
        if (0 && parse == -2)
        {
            fprintf(stderr, "ignored %6d: %s\n", lineno,buf);
        }
    }
    parse_line(NULL);

    if (dump_song)
    {
        dump_song_records();
        exit(0);
    }

    /* Set all the durations on the stream records */
    set_record_durations();

    /* Scan through looking for records which won't play anything. */
    /* Optimize them away. */
    prec = stream_head->next;
    rec = prec->next;
    memset( reg , 0, sizeof( reg )); memset(oreg , 0, sizeof( reg ));
    memset( reg2, 0, sizeof( reg2)); memset(oreg2, 0, sizeof( reg2));
    slot_offset = 0; generate_regs(prec,reg );
    slot_offset = 3; generate_regs(prec,reg2);
    memcpy(oreg, reg, sizeof(oreg));
    while (rec)
    {
        slot_offset = 0; generate_regs(rec,reg );
        slot_offset = 3; generate_regs(rec,reg2);
        
        if (regs_semantically_equiv(reg , oreg ) &&
            (song2 == 0 || regs_semantically_equiv(reg2, oreg2)))
        {
            prec->dur += rec->dur;
            prec->next = rec->next;
        } else
        {
            prec = rec;
            memcpy(oreg , reg , sizeof(oreg ));
            memcpy(oreg2, reg2, sizeof(oreg2));
        }
            
        rec = rec->next;
    }


    slot_mult=1;
    slot_offset=0;
    rec = stream_head->next;
    tot = 0;
    dur = 0;
    memset( reg, 0, sizeof( reg));
    memset(oreg, 1, sizeof(oreg));
    emit_solid_line();
    printf("; TICK %10d  DUR %5d\n", 0, 1);
    printf(";   SETUP ALL\n");
    memset(reg, 0, sizeof(reg));
    reg[8] = 0x38;
    num_ctrl += output_record(reg, oreg, 0);
    memcpy(oreg, reg, sizeof(reg));
    memcpy(mreg, reg, sizeof(reg));
    w_ptr = 0;
    was_merge = 0;
    while (rec)
    {
        int all_zero = 0;

        generate_regs(rec, reg);

        if (!was_merge && regs_semantically_equiv(reg, mreg))
        {
            dur += rec->dur;
            merges++;
            semantic_merges++;
            puts("; (smerge)");
            merge = NULL;
            goto next1;
        } else if (dur && !was_merge)
        {
            num_ctrl += output_record(mreg, oreg, dur);
            /*memcpy(oreg, mreg, 14);*/
            dur = 0;
        }
        merge = NULL;
        was_merge = 0;

        if (zero_merge)
        {
            all_zero = 1;
            for (i = 0; i < 14; i++)
                if (reg[i] != mreg[i]) all_zero &= reg[i] == 0;
        } else
            all_zero = 0;

        dur += rec->dur;
        if (dur > all_zero)
        {
            /*num_ctrl += output_record(reg, oreg, dur);*/
        } else
        {
            merges++;
            if (dur > 0) zero_merges++;
            merge = dur > 0 ? "; (zmerge)" : "; (merge)";
            was_merge = 1;
        }
        memcpy(mreg, reg, sizeof(reg));
next1:
        printf("; TICK %10d  DUR %5d  TIME %10d\n", tot, rec->dur, rec->time);
        tot += rec->dur;
        for (i = 0; i < MAXACTIVE; i++)
            if (rec->note[i][0])
            {
                printf(";   SLOT %-2d  %6s @ %-3d --> "
                    " PER $%.3X  VOL $%.3X\n",
                    i, rec->note[i], rec->vel[i],
                    rec->per[i],rec->vol[i]);
            }
        if (merge) puts(merge);
        all_zeros += all_zero;
        rec = rec->next;
    }
    if (!merge && dur)
    {
        num_ctrl += output_record(mreg, oreg, dur);
        /*memcpy(oreg, mreg, 14);*/
        dur = 0;
    }
    if (!merge) emit_solid_line();
    printf("; TICK %10d  DUR %5d\n", tot, 1);
    printf(";   ZERO ALL\n");
    memset(reg, 0, sizeof(reg));
    reg[8] = 0x38;
    num_ctrl += output_record(reg, oreg, 1);
    printf("; SUMMARY:\n");
    emit_dashed_line();
    printf(";     TICKS:      %10d\n", tot);
    printf(";     RECORDS:    %10d\n", total_records);
    printf(";     ZEROS:      %10d\n", all_zeros);
    printf(";     MERGES:     %10d\n", merges);
    printf(";     ZMERGES:    %10d\n", zero_merges);
    printf(";     SMERGES:    %10d\n", semantic_merges);
    printf(";     BREF:       %10d\n", brefs);
    printf(";     CONTROL:    %10d\n", num_ctrl);
    emit_dashed_line();
    printf("; Total size: %10d decles\n", decles);
    emit_solid_line();


    if (!song2) return 0;
    offset += 3;

    output_record1(reg, oreg, -1, 0);

    printf("SONG2:\n");
    slot_mult=1;
    slot_offset=3;
    rec = stream_head->next;
    tot = 0;
    dur = 0;
    memset( reg, 0, sizeof( reg));
    memset(oreg, 1, sizeof(oreg));

    emit_solid_line();
    printf(";   SETUP ALL\n");
    memset(reg, 0, sizeof(reg));
    reg[8] = 0x38;
    num_ctrl += output_record(reg, oreg, 0);
    memcpy(oreg, reg, sizeof(reg));
    memcpy(mreg, reg, sizeof(reg));
    w_ptr = 0;
    was_merge = 0;
    while (rec)
    {
        int all_zero = 0;

        generate_regs(rec, reg);


        if (!was_merge && regs_semantically_equiv(reg, mreg))
        {
            dur += rec->dur;
            merges++;
            semantic_merges++;
            puts("; (smerge)");
            merge = NULL;
            goto next2;
        } else if (dur && !was_merge)
        {
            num_ctrl += output_record(mreg, oreg, dur);
            /*memcpy(oreg, mreg, 14);*/
            dur = 0;
        }
        merge = NULL;
        was_merge = 0;

        if (zero_merge)
        {
            all_zero = 1;
            for (i = 0; i < 14; i++)
                if (reg[i] != oreg[i]) all_zero &= reg[i] == 0;
        } else
            all_zero = 0;

        dur += rec->dur;
        if (dur > all_zero)
        {
            /*num_ctrl += output_record(reg, oreg, dur);*/
        } else
        {
            merges++;
            if (dur > 0) zero_merges++;
            merge = dur > 0 ? "; (zmerge)" : "; (merge)";
            was_merge = 1;
        }
        memcpy(mreg, reg, sizeof(reg));
next2:
        printf("; TICK %10d  DUR %5d  TIME %10d\n", tot, rec->dur, rec->time);
        tot += rec->dur;
        for (i = 0; i < MAXACTIVE; i++)
            if (rec->note[i][0])
            {
                printf(";   SLOT %-2d  %6s @ %-3d --> "
                    " PER $%.3X  VOL $%.3X\n",
                    i, rec->note[i], rec->vel[i],
                    rec->per[i],rec->vol[i]);
            }
        if (merge) puts(merge);
        all_zeros += all_zero;
        rec = rec->next;
    }
    if (!merge && dur)
    {
        num_ctrl += output_record(mreg, oreg, dur);
        /*memcpy(oreg, mreg, 14);*/
        dur = 0;
    }
    if (!merge) emit_solid_line();
    printf("; TICK %10d  DUR %5d\n", tot, 1);
    printf(";   ZERO ALL\n");
    memset(reg, 0, sizeof(reg));
    reg[8] = 0x38;
    num_ctrl += output_record(reg, oreg, 1);

    printf("; SUMMARY:\n");
    emit_dashed_line();
    printf(";     TICKS:      %10d\n", tot);
    printf(";     RECORDS:    %10d\n", total_records);
    printf(";     ZEROS:      %10d\n", all_zeros);
    printf(";     MERGES:     %10d\n", merges);
    printf(";     ZMERGES:    %10d\n", zero_merges);
    printf(";     SMERGES:    %10d\n", semantic_merges);
    printf(";     BREF:       %10d\n", brefs);
    printf(";     CONTROL:    %10d\n", num_ctrl);
    emit_dashed_line();
    printf("; Total size: %10d decles\n", decles);
    emit_solid_line();

    return 0;
}

