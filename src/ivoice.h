/*
 * ============================================================================
 *  Title:    Intellivoice Emulation
 *  Author:   J. Zbiciak
 * ============================================================================
 *  This is just a dummy object right now.
 * ============================================================================
 */
#ifndef IVOICE_H_
#define IVOICE_H_

#define SCBUF_SIZE   (4096)             /* Must be power of 2               */
#define SCBUF_MASK   (SCBUF_SIZE - 1)

typedef struct lpc12_t
{
    int     rpt, cnt;       /* Repeat counter, Period down-counter.         */
    uint32_t per, rng;       /* Period, Amplitude, Random Number Generator   */
    int     amp;
    int16_t f_coef[6];      /* F0 through F5.                               */
    int16_t b_coef[6];      /* B0 through B5.                               */
    int16_t z_data[6][2];   /* Time-delay data for the filter stages.       */
    uint8_t  r[16];          /* The encoded register set.                    */
    int     interp;
} lpc12_t;


typedef struct ivoice_t
{
    uint64_t    now;

    int         silent;     /* Flag:  Intellivoice is silent.               */

    int16_t     scratch[SCBUF_SIZE];    /* Scratch buffer for audio.        */
    uint32_t    sc_head;    /* Head/Tail pointer into scratch circular buf  */
    uint32_t    sc_tail;    /* Head/Tail pointer into scratch circular buf  */
    uint64_t    sound_current;
    int         sample_frc, sample_int;

    int         window[24]; /* Sliding Window. Enough for 96khz.            */
    int         wind_sum;   /* Window sum.                                  */
    int         wind_ptr;   /* Window pointer.                              */
    int         rate, wind; /* Sample rate, Window size.                    */
    int         pal_mode;   /* PAL vs. NTSC                                 */
    double      time_scale; /* For --macho                                  */
    double      skipping;   /* part of time-scale                           */

    lpc12_t     filt;       /* 12-pole filter                               */
    int         lrq;        /* Load ReQuest.  == 0 if we can accept a load  */
    int         ald;        /* Address LoaD.  < 0 if no command pending.    */
    int         pc;         /* Microcontroller's PC value.                  */
    int         stack;      /* Microcontroller's PC stack.                  */
    int         fifo_sel;   /* True when executing from FIFO.               */
    int         halted;     /* True when CPU is halted.                     */
    uint32_t    mode;       /* Mode register.                               */
    uint32_t    page;       /* Page set by SETPAGE                          */

    uint32_t    fifo_head;  /* FIFO head pointer (where new data goes).     */
    uint32_t    fifo_tail;  /* FIFO tail pointer (where data comes from).   */
    uint32_t    fifo_bitp;  /* FIFO bit-pointer (for partial decles).       */
    uint16_t    fifo[64];   /* The 64-decle FIFO.                           */

    int         cur_len;    /* Fullness of current sound buffer.            */
    int16_t    *cur_buf;    /* Current sound buffer.                        */
    const uint8_t *rom[16]; /* 4K ROM pages.                                */
} ivoice_t;

struct ivoiceSerialized {
    ivoice_t main;
    int ivoiceBufferSize;
    int16_t ivoiceBuffer[AUDIO_FREQUENCY / 60 * 2];
};

void ivoiceSerialize(struct ivoiceSerialized *);
void ivoiceUnserialize(const struct ivoiceSerialized *);

uint32_t ivoice_tk(uint32_t);
uint32_t ivoice_rd(uint32_t);
void ivoice_wr(uint32_t, uint32_t);
void ivoice_reset(void);
void ivoice_dtor(void);
void ivoice_frame(void);

/* ======================================================================== */
/*  IVOICE_INIT  -- Makes a new Intellivoice                                */
/* ======================================================================== */
int ivoice_init
(
    int             pal_mode,
    double          time_scale
);

extern int ivoiceBufferSize;
extern int16_t ivoiceBuffer[];

#endif
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
/*  You should have received a copy of the GNU General Public License along */
/*  with this program; if not, write to the Free Software Foundation, Inc., */
/*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             */
/* ======================================================================== */
/*                 Copyright (c) 1998-2000, Joseph Zbiciak                  */
/* ======================================================================== */
