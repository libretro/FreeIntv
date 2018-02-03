;;==========================================================================;;
;; Joe Zbiciak's 4-TRIS, A "Falling Tetrominoes" Game for Intellivision.    ;;
;; Copyright 2000, Joe Zbiciak, im14u2c@primenet.com.                       ;;
;; http://www.primenet.com/~im14u2c/intv/                                   ;;
;;==========================================================================;;

;* ======================================================================== *;
;*  This program is free software; you can redistribute it and/or modify    *;
;*  it under the terms of the GNU General Public License as published by    *;
;*  the Free Software Foundation; either version 2 of the License, or       *;
;*  (at your option) any later version.                                     *;
;*                                                                          *;
;*  This program is distributed in the hope that it will be useful,         *;
;*  but WITHOUT ANY WARRANTY; without even the implied warranty of          *;
;*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *;
;*  General Public License for more details.                                *;
;*                                                                          *;
;*  You should have received a copy of the GNU General Public License       *;
;*  along with this program; if not, write to the Free Software             *;
;*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               *;
;* ======================================================================== *;
;*                   Copyright (c) 2000, Joseph Zbiciak                     *;
;* ======================================================================== *;

        ROMW    16              ; Just for the heck of it.
        ORG     $5000           ; Standard Mattel cartridge memory map

;------------------------------------------------------------------------------
; Magic Constants
;------------------------------------------------------------------------------
EDGEL   EQU     15              ; Left edge column
EDGER   EQU     24              ; Right edge column
EDGEB   EQU     21              ; Bottom edge row
MOVEINC EQU     5               ; Per-downward-move score increment
DANGER  EQU     6               ; Trigger point at which music goes 2x
COPYR   EQU     $13A            ; Character # for Copyright circle-C

NXTPCX  EQU     33              ; X coord where next piece is shown
NXTPCY  EQU     11              ; Y coord where next piece is shown

LVLROW  EQU     4               ; Row 'Level' banner is displayed on
LVLCOL  EQU     2               ; Column 'Level' banner is displayed on
LVLLOC  EQU     $200 + 20*(LVLROW - 1) + LVLCOL

LINROW  EQU     7               ; Row 'Lines' banner is displayed on
LINCOL  EQU     2               ; Column 'Lines' banner is displayed on
LINLOC  EQU     $200 + 20*(LINROW - 1) + LINCOL

NXTROW  EQU     (NXTPCY - 3)/2  ; Row 'Next' banner is displayed on
NXTCOL  EQU     (NXTPCX - 3)/2  ; Column 'Next' banner is displayed on
NXTLOC  EQU     $200 + 20*(NXTROW - 1) + NXTCOL

;------------------------------------------------------------------------------
; Magic memory locations
;------------------------------------------------------------------------------
VBLANK  EQU     $20             ; Vertical-blank Handshake
COLSTK  EQU     $21             ; Color-stack/FGBG switch
CS0     EQU     $28             ; Color Stack 0
CS1     EQU     $29             ; Color Stack 1
CS2     EQU     $2A             ; Color Stack 2
CS3     EQU     $2B             ; Color Stack 3
CB      EQU     $2C             ; Color for border
ISRVEC  EQU     $100            ; ISR jump vector

;               $130 .. $136    ; Save area used by DRAWPIECE/TESTPIECE

TMPPC   STRUCT  $137            ; Temporary storage
@@C     EQU     $ + 0           ; (R0) Color of current piece  UNUSED
@@X     EQU     $ + 1           ; (R1) Pivot X coordinate for current piece
@@Y     EQU     $ + 2           ; (R2) Pivot Y coordinate for current piece
@@N     EQU     $ + 3           ; (R3) Piece number for current piece.
        ENDS

CURPC   STRUCT  $13B            ; Current active piece
@@C     EQU     $ + 0           ; (R0) Color of current piece
@@X     EQU     $ + 1           ; (R1) Pivot X coordinate for current piece
@@Y     EQU     $ + 2           ; (R2) Pivot Y coordinate for current piece
@@N     EQU     $ + 3           ; (R3) Piece number for current piece.
        ENDS

PXQ_L0  EQU     $140            ; Putpixel Queue length -- not committed
PXQ_L1  EQU     $141            ; Putpixel Queue length -- committed
PXQ_XY  EQU     $142 ;..$161    ; Putpixel Queue XY data, interleaved.

VOLSV   EQU     $170 ;..$172    ; Volume save-area during pause
PAUSED  EQU     $173            ; Flag saying we paused.

OVRFLO  EQU     $1A0            ; Number of overflows observed
WASDOWN EQU     $1A1            ; Flag: This movement was 'down'
WASHAND EQU     $1A2            ; Flag: This movement was 'hand'
SLEVEL  EQU     $1A8            ; Current game level number
LEVEL   EQU     $1A9            ; Current game level number
SFXPRIO EQU     $1AA            ; Priority of currently playing sound
SCRPOS  EQU     $1AB            ; Position onscreen to display the score
SCRDIG  EQU     $1AC            ; 10 - Number of digits to display in score
SHOWNXT EQU     $1AD            ; Show next piece
NXTPC   EQU     $1AE            ; Next Piece
REPEAT  EQU     $1AF            ; Flag:  Set to keypress we're repeating
LHAND   EQU     $1B0            ; Last hand-controller input
LHKPD   EQU     $1B1            ; Last input was pad if non-zero
MUTE    EQU     $1B2            ; Last input was pad if non-zero
CSAVE   EQU     $1B3            ; Global 'c' save area
TSKDQ   EQU     $1C0 ;..$1DF    ; $1C0..$1DF:  Task data queue
XSAVE   EQU     $1E0            ; Global 'x' save area
YSAVE   EQU     $1E1            ; Global 'y' save area
               ;$1E2 .. $1E6, reserved
DIDNXT  EQU     $1EB            ; Next Piece was displayed some time this piece
HEIGHT  EQU     $1EC            ; Height of blocks in well.
TSKACT  EQU     $1ED            ; Number of highest numbered task + 1
TSKQHD  EQU     $1EE            ; Head pointer for task queue (0..15)
TSKQTL  EQU     $1EF            ; Tail pointer for task queue (0..15)
PSG0    EQU     $1F0 ;..$1FD    ; PSG base address
CTRL0   EQU     $1FE            ; Right hand controller
CTRL1   EQU     $1FF            ; Left hand controller

MAXTSK  EQU     4               ; Right now allow 4 active tasks
TSKQ    EQU     $320 ;..$32F    ; $320..$32F:  Task queue
TSKTBL  EQU     $330 ;..$33F    ; $330..$33F:  Task table  (four tasks)
SNDSTRM EQU     $340 ;..$347    ; $340..$347:  Sound stream table
PREVBL  EQU     $348            ; Pre-VBLANK routine address
HANDFN  EQU     $34A            ; Keypad dispatch function table.
SCRCOL  EQU     $34B            ; Color that score is displayed in
WTIMER  EQU     $34C            ; Countdown timer for WAIT
REGSV   EQU     $34D ;..$351    ; $34D..$351:  Register save area for SLEEP
PSCORL  EQU     $352            ; Player's score  (lo 16 bits)
PSCORH  EQU     $353            ; Player's score  (hi 16 bits)
DSCORL  EQU     $354            ; Displayed score (lo 16 bits)
DSCORH  EQU     $355            ; Displayed score (hi 16 bits)
LINES   EQU     $356            ; Number of lines cleared so far
MSCORE  EQU     $358            ; Total of per-move scores
TMP     EQU     $35D
RANDLO  EQU     $35E            ; Low word of random number generator
RANDHI  EQU     $35F            ; High word of random number generator

SPEED   EQU     TSKTBL + 3      ; Piece movement speed (IN TASK TABLE!)

GROM    EQU     $3000
GRAM    EQU     $3800


;------------------------------------------------------------------------------
ROMHDR:
        WORD    $0000           ; Movable object data (ignored)
        DECLE   $01,$00         ; RTAB (ignored)
        WORD    START           ; Program start address (ignored)
        WORD    $0000           ; Background graphics
        WORD    ROMHDR + 2      ; Card table -- stored above in header. :-)
        WORD    T1TLE           ; Title string:  '4-TRIS'

        DECLE   $3C0            ; run title code, clicks off, INTY2 on, no ECS
        DECLE   $00             ; -> to STIC $32
        DECLE   $00             ; 0 = color stack, 1 = f/b mode
        DECLE   0, 0, 0, 0      ; color stack elements 1 - 4
        DECLE   $00             ; border color
;------------------------------------------------------------------------------

;;==========================================================================;;
;;  SNDTBL                                                                  ;;
;;  Contains pointers and parameters for all sound effects and music.       ;;
;;==========================================================================;;
SNDTBL  PROC
@@bip   DECLE   BIP,    $2744   ; Played when player moves a piece
@@byump DECLE   BYUMP,  $2744   ; Played when the player places a piece
@@bomb  DECLE   BOMB,   $27CC   ; Played when clearing a line
@@bomb4 DECLE   BOMB4,  $3FFF   ; Played when clearing FOUR lines
@@boom2 DECLE   BOOM2,  $3FFF   ; Played when well overflows
@@ding3 DECLE   DING3,  $35EE   ; Played to signal changing level

@@title DECLE   NUTMARCH, $3FFF ; Title screen music
@@game  DECLE   CHINDNCE, $3FFF ; In-game music
@@over  DECLE   BEHAPPY,  $3FFF ; Game-over music

@@silence
        DECLE   SILENCE, $3FFF  ; Silence!
        ENDP

FXBIP   EQU     SNDTBL.bip   - SNDTBL
FXBOMB  EQU     SNDTBL.bomb  - SNDTBL
FXBOMB4 EQU     SNDTBL.bomb4 - SNDTBL
FXBOOM2 EQU     SNDTBL.boom2 - SNDTBL
FXDING3 EQU     SNDTBL.ding3 - SNDTBL
FXBYUMP EQU     SNDTBL.byump - SNDTBL

M_TITLE EQU     SNDTBL.title - SNDTBL
M_GAME  EQU     SNDTBL.game  - SNDTBL
M_OVER  EQU     SNDTBL.over  - SNDTBL

FX_OFF  EQU     SNDTBL.silence - SNDTBL

;;==========================================================================;;
;;  TITLE / START                                                           ;;
;;                                                                          ;;
;;  This contains the title string and the startup code.  We pre-empt the   ;;
;;  EXEC's initialization sequence by setting the "Special Copyright" bit   ;;
;;  in location $500C.  This causes the code at 'START' to run before the   ;;
;;  built-in title screen is completely displayed.                          ;;
;;                                                                          ;;
;;  The Startup code does very little.  Mainly, it sets the Interrupt       ;;
;;  Service Routine vector to point to our _real_ initialization routine,   ;;
;;  INIT.  This is done because we can only get to GRAM and STIC registers  ;;
;;  during the vertical retrace, and vertical retrace is signaled by an     ;;
;;  interrupt.  (Actually, we can have access to GRAM/STIC for longer       ;;
;;  if we don't hit the STIC 'handshake' at location $20, but then the      ;;
;;  display blanks.  During INIT, the display does blank briefly.)          ;;
;;==========================================================================;;
TITLE:  BYTE    100, "*-TRIS", 0 ; Title: 4-TRIS, Copyright 2000
        ; Intercept/preempt EXEC initialization and just do our own.
        ; We call no EXEC routines in this game.
START:
        MVI     $1FF,   R2
        AND     $1FE,   R2

        CLRR    R4              ; Prepare to zero all system RAM, PSG0,& STIC.
        MVII    #$20,   R1      ; $00...$1F. (The STIC)
        JSRD    R5,     FILLZERO

        ADDI    #8,     R4      ; $28...$32. (The rest of the STIC)
        MVII    #11,    R1
        CALL    FILLZERO
                         
        MVII    #$F0,   R4      ; $F0...$35D. We spare the rand seed values
        MVII    #$26D,  R1      ; in $35E..$35F to add some randomness.
        CALL    FILLZERO

        COMR    R2
        MVO     R2,     LHAND   ; Set up initial hand-controller state

        MVII    #INIT,   R0     ; Our initialization routine
        MVII    #ISRVEC, R4     ; ISR vector
        MVO@    R0,     R4      ; Write low half
        SWAP    R0              ;
        MVO@    R0,     R4      ; Write high half
        MVI     RANDLO, R0      ; Get whatever garbage data is in memory in the
        MVI     RANDHI, R1      ; random number generator fields

        MOVR    PC,     R5      ; Loop starting at the next instruction.
@@spin:
        INCR    R0              ; Seed random number generator while
        DECR    R1              ; we wait for our first interrupt.
        MVO     R0,     RANDLO
        MVO     R1,     RANDHI
        EIS
        ; This falls through to STUB which causes
        ; our loop (saves a couple DECLEs.)

;;==========================================================================;;
;;  STUB                                                                    ;;
;;  Null routine used in dispatchers where no behavior is defined/desired.  ;;
;;==========================================================================;;
STUB:   JR      R5              ; Stub routine


;;==========================================================================;;
;;  INIT                                                                    ;;
;;  Initializes the ISR, etc.  Gets everything ready to run.                ;;
;;  This is called via the ISR dispatcher, so it's safe to bang GRAM from   ;;
;;  here, too.                                                              ;;
;;                                                                          ;;
;;   -- Zero out memory to get started                                      ;;
;;   -- Set up variables that need to be set up here and there              ;;
;;   -- Set up GRAM image                                                   ;;
;;   -- Drop into the main game state-machine.                              ;;
;;==========================================================================;;
INIT:   PROC
        DIS
        MVII    #$2F0,  R6              ; Reset the stack pointer

        MVI     COLSTK, R1              ; Force display to color-stack mode

        ; Make sure random number generator is non-zero.
        MOVR    PC,     R0              ; The PC is guaranteed to be non-zero.
        XOR     RANDLO, R0
        BEQ     @@randok                ; If the XOR result is zero, we're ok,
        MVO     R0,     RANDLO          ; otherwise make RANDLO non-zero.
@@randok:

        ; Stub out all of the task hooks
        MVII    #STUB,  R0              ; Prepare to stub out some hooks.
        MVO     R0,     PREVBL          ; Initial pre-VBL task is NULL

        MVII    #MAINISR, R0            ; Point ISR vector to our ISR
        MVO     R0,     ISRVEC          ; store low half of ISR vector
        SWAP    R0                      ;
        MVO     R0,     ISRVEC+1        ; store high half of ISR vector

        ; Do the face?
        MVII    #$D9,   R0
        CMP     $1FE,   R0
        BEQ     DOFACE
        CMP     $1FF,   R0
        BEQ     DOFACE

        ; Default the GRAM image to be same as GROM.
        MVII    #GROM,  R5              ; Point R5 at GROM
        MVII    #GRAM,  R4              ; Point R4 at GRAM
        MVII    #$200,  R0
@@gromcopy:
        MVI@    R5,     R1
        MVO@    R1,     R4
        DECR    R0
        BNEQ    @@gromcopy

        ; Copy our GRAM font into GRAM overtop of default.
        CALL    LOADFONT
        DECLE   FONT

        ; Ok, everything's ready to roll now.
        EIS

        ;;==================================================================;;
        ;;  Game phases:                                                    ;;
        ;;   -- Title screen                                                ;;
        ;;   -- (optional) Sound Test                                       ;;
        ;;   -- In Game                                                     ;;
        ;;   -- Game over                                                   ;;
        ;;                                                                  ;;
        ;;  The INIT code cycles between these gae phases by first          ;;
        ;;  calling the subroutine which sets up the tasks for a given      ;;
        ;;  game phase, and then calling the main event loop which          ;;
        ;;  runs the game.  This assumes that all tasks are stopped when    ;;
        ;;  the MAINLOOP exits.  Calling SCHEDEXIT accomplishes a clean     ;;
        ;;  exit from MAINLOOP by scheduling an exit while stopping all     ;;
        ;;  other tasks.  (Neat, eh?)                                       ;;
        ;;==================================================================;;
@@loop:
        CALL    TITLESCREEN     ; Set up 'Title Screen'
        CALL    MAINLOOP        ; Do title screen.
        MVI     SLEVEL, R0      ; If starting level == 0, do 'Sound Test'
        TSTR    R0
        BEQ     @@soundtest

        CALL    INGAME          ; Set up 'In Game'
        CALL    MAINLOOP        ; Do the Game.

        CALL    GAMEOVER        ; Set up the 'Game Over' sequence.
        CALL    MAINLOOP        ; Let it run.

        B       @@loop          ; Start over at the title screen.

@@soundtest:
        CALL    SOUNDTEST       ; Sound-test requested.  Set that up.
        CALL    MAINLOOP        ; Run the sound-test.
        B       @@loop          ; Go back to title screen when done.
        ENDP

;;==========================================================================;;
;;  LOADFONT -- Load a compressed FONT into GRAM.                           ;;
;;                                                                          ;;
;;  Note:  This should be called only when the STIC is already in CPU-      ;;
;;  controlled mode, such as from an interrupt handler.  Loading a large    ;;
;;  font may cause the screen to blank for one or more frames.              ;;
;;                                                                          ;;
;;  Font data is broken up into spans of characters that are copied         ;;
;;  into GRAM.  Each span is defined as follows.                            ;;
;;                                                                          ;;
;;    Span Header:  2 Decles                                                ;;
;;        DECLE   Skip Length (in bytes of GRAM memory)                     ;;
;;        DECLE   Span Length (in bytes of GRAM memory)                     ;;
;;    Span Data -- up to Span Length Decles.                                ;;
;;                                                                          ;;
;;  Span Data is run-length encoded using the upper two bits of the         ;;
;;  decle to specify the run length.  Valid run lengths are 0..3,           ;;
;;  with 0 meaning "just copy this byte to the GRAM", and 3 meaning         ;;
;;  "copy this byte to GRAM and make three more copies in the locations     ;;
;;  afterwards".  To see what I mean, look at the font data in              ;;
;;  "font.asm".                                                             ;;
;;                                                                          ;;
;;  The run length encoding does not change the value used for              ;;
;;  'span length'.  The span length is always given in terms of             ;;
;;  # of GRAM locations, and not number of decles in the FONT data.         ;;
;;                                                                          ;;
;;  The font is terminated with a span of length 0.                         ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Points to word (1 decle in 16-bit ROM) containing ptr to font     ;;
;;        info.  Code returns after this word.                              ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0, R1, R4, R5 trashed.                                                 ;;
;;  GRAM is updated according to the font specification.                    ;;
;;                                                                          ;;
;;==========================================================================;;
LOADFONT PROC
        MVI@    R5,     R0
        PSHR    R5
        MOVR    R0,     R5

        MVII    #GRAM,  R4              ; Point R4 at GRAM

@@gramloop:
        MVI@    R5,     R0              ; Get skip & span len. (in GRAM bytes)
        TSTR    R0                      ; Quit if skip/span == 0.
        BEQ     @@gramdone

        MOVR    R0,     R1
        ANDI    #$7F8,  R0              ; Extract span length.
        XORR    R0,     R1              ; Clear away span bits from word
        SWAP    R1                      ; Extrack skip value.
        ADDR    R1,     R4              ; Skip our output pointer.
        SLR     R0,     1               ; Divide count by 2.

@@charloop:
        MVI@    R5,     R1              ; Get two bytes
        MVO@    R1,     R4              ; Put the first byte
        SWAP    R1                      ; Put the other byte into position
        MVO@    R1,     R4              ; Put the second byte
        DECR    R0                      ; Sheesh, do I have to spell this out?
        BNEQ    @@charloop              ; inner loop
        B       @@gramloop              ; outer loop

@@gramdone:
        PULR    PC
        ENDP

;;==========================================================================;;
;;  MAINISR                                                                 ;;
;;  This is the main interrupt service routine.  It has to perform the      ;;
;;  following tasks:                                                        ;;
;;                                                                          ;;
;;   -- Run any miscellaneous "pre-VBLANK" task requested by main program.  ;;
;;   -- Hit the vertical blank handshake to keep screen enabled.            ;;
;;   -- Update any active sound streams.                                    ;;
;;   -- Drain and pixels queued up in the Pixel Queue.                      ;;
;;   -- Count down task timers and schedule tasks when timers expire.       ;;
;;   -- Count down the 'busy-wait' timer if it is set.                      ;;
;;   -- Detect a "Pause" request and pause the game if needed.              ;;
;;                                                                          ;;
;;  This particular program currently does not sequence GRAM at all after   ;;
;;  the initial bootup.  Therefore, we can hit the VBLANK handshake almost  ;;
;;  immediately after entering the ISR.  We do support a pre-VBLANK         ;;
;;  routine though for other purposes, such as setting color-stack regs.    ;;
;;                                                                          ;;
;;  Our code does not rely on any EXEC routines at all, except the ISR      ;;
;;  dispatch routine which saves and restores all the registers.  (On a     ;;
;;  real Intellivision, we have no choice.  On an emulator, that routine    ;;
;;  can be replaced with a separate non-EXEC routine which performs a       ;;
;;  similar dispatch, if we want to distribute this program without an      ;;
;;  EXEC ROM image but with an emulator.)                                   ;;
;;                                                                          ;;
;;==========================================================================;;
MAINISR PROC

        PSHR    R5                      ; Save return address

        ;; Call out to user-defined pre-VBLANK routine.
        ;; This routine should point to the stub if no routine is to
        ;; be called.  (simplifies the code w/out really slowing it down.)
        MVII    #@@rl0, R5      ; Point to @@rl0
        MVI     PREVBL, PC      ; Call subroutine
@@rl0:
        ;; Hit the VBLANK handshake now
        MVO     R0,     $20     ; Allow STIC to display this frame.

        ;; Update sound streams.  All sound streams update at 60Hz.
        ;; Stream #0 can update at "60Hz * 2" if double-speed mode is on.
        CALL    SNDPRIO         ; Make sure music & sfx play nice

        ; Update music
        CALL    DOSNDSTREAM     ; Update the sound stream
        DECLE   SNDSTRM         ; Point to the music stream

        ; Update sfx
        CALL    DOSNDSTREAM     ; Update the sound stream
        DECLE   SNDSTRM+4       ; Point to the sfx stream

        ; If well is too tall, play music fast.
        MVII    #DANGER,  R0
        CMP     HEIGHT,   R0
        BLT     @@slowmusic
        MVII    #SNDSTRM, R4
        CALL    DOSNDSTREAM     ; Update the sound stream
        DECLE   SNDSTRM         ; Point to the music stream
@@slowmusic:

        MVII    #RANDHI,R1
        MVI     SNDSTRM+1, R2
        XOR@    R1,     R2
        MVO@    R2,     R1      ; Throw some bits into the random bucket

        ;; Drain any pixels that were queued and committed.
        CALL    DRAINPXQ

        ;; Count down task timers and schedule tasks.  We have up to MAXTSK
        ;; active tasks at one time.  We decrement all counters by two.
        ;;   -- If the count is initially <= 0, the task is inactive and
        ;;      is skipped.
        ;;   -- If the count goes to zero, the task is triggered and its
        ;;      count is reinitialized.
        ;;   -- If the count goes negative, the task is triggered and its
        ;;      count is not reinitialized.
        ;; This allows us to have one-shot and repeating tasks pretty
        ;; easily.  Repeating tasks clear bit 0 of their period count,
        ;; and one-shot tasks set bit 0.
        ;;
        ;; When tasks are triggered, their procedure address is written
        ;; to the task queue which is drained by the non-ISR main task.
        ;; The task queue is a small circular buffer in 16-bit memory with
        ;; head and tail pointers in 8-bit memory.  The circular buffer is
        ;; 16 entries large.  I hope this is enough.  Overflow can cause
        ;; strange effects.
        ;;
        ;; Task control table entry layout:
        ;;
        ;;   Word 0:  Function Ptr
        ;;   Word 1:  Instance data
        ;;   Word 2:  Current down-count
        ;;   Word 3:  Reinit count

        MVI     TSKACT, R0      ; Iterate over active tasks
        TSTR    R0
        BEQ     @@notasks
        MVII    #TSKTBL+2, R3   ; Point to the task control table

@@taskloop:
        MVI@    R3,     R1      ; Get tasks's current tick count
        TSTR    R1              ; Is it <= 0?
        BLE     @@nexttask      ; ... yes? Skip it.

        MOVR    R1,     R2
        ADD     RANDLO, R2
        MVO     R2,     RANDLO  ; Throw some bits into the random bucket

        SUBI    #2,     R1      ; Count it down.
        BNEQ    @@noreinit      ; If it went to zero, reinit count

        INCR    R3
        MVI@    R3,     R1      ; Get new period for task
        DECR    R3
        SUBR    R2,     R2      ; Z = 1, S = 0 (causes the BGT to not be taken)
@@noreinit:
        MVO@    R1,     R3      ; Store new period.
        BGT     @@nexttask      ; If this count didn't expire, ...
                                ;  ... go to the next task
@@q:
        ; Schedule this task by adding it to the task queue
        MOVR    R3,     R4
        SUBI    #2,     R4
        MVI@    R4,     R2      ; Get task function pointer
        MVI     TSKQHD, R1      ; Get task queue head
        INCR    R1              ; Move to next slot
        ANDI    #$F,    R1      ; Stay within 16-entry circ buffer
        CMP     TSKQTL, R1
        BEQ     @@overflow      ; Drop this task if queue overflows. (!)

        MOVR    R1,     R5
        ADDR    R1,     R5      ; R5 is for task data queue
        ADDI    #TSKDQ, R5      ; Point to task data queue

        MVO     R1,     TSKQHD  ; Store updated task queue head

        ADDI    #TSKQ,  R1      ; Point to actual task queue
        MVO@    R2,     R1      ; Store the function pointer
        MVI@    R4,     R2      ; Get task instance data
        MVO@    R2,     R5      ; Store low half of instance data
        SWAP    R2,     1
        MVO@    R2,     R5      ; Store high half of instance data

@@nexttask:
        ADDI    #4,     R3      ; Point to next task struct
        DECR    R0
        BNEQ    @@taskloop      ; Loop until done with all tasks
        B       @@notasks

@@overflow:
        MVI     OVRFLO, R0
        INCR    R0
        MVO     R0,     OVRFLO

@@notasks:

@@wtimer:
        ;; Count down the wait-timer, if there is one
        MVI     WTIMER, R5
        DECR    R5
        BMI     @@expired
        MVO     R5,     WTIMER
@@expired:

        ;; Check to see if the user has requested a pause.
        MVII    #$5A,   R0      ; Keypad 1 + Keypad 9
        CMP     $1FE,   R0      ; Pause on first controller?
        BEQ     @@pausing
        CMP     $1FF,   R0      ; Pause on second controller?
        BEQ     @@pausing

        PULR    PC              ; Return

@@pausing:
        DIS                     ; Disable interrupts while paused.
        ; Grab PSG volume settings aside and force volume to zero.
        MVII    #VOLSV, R2
        MVII    #$1FB,  R3
        MOVR    R2,     R5
        MOVR    R3,     R4

        MVO     R4,     PAUSED  ; Set 'we paused' flag
        MVI@    R4,     R0      ; Channel a
        MVO@    R0,     R5
        MVI@    R4,     R0      ; Channel b
        MVO@    R0,     R5
        MVI@    R4,     R0      ; Channel c
        MVO@    R0,     R5

        CLRR    R1
        MOVR    R3,     R4
        MVO@    R1,     R4
        MVO@    R1,     R4
        MVO@    R1,     R4

        CLRC
        CALL    @@waithand      ; Wait for both controllers to be released.
        SETC
        CALL    @@waithand      ; Wait for either controller to be pressed
        CLRC
        CALL    @@waithand      ; Wait for both controllers to be released.

        MOVR    R3,     R5
        MOVR    R2,     R4
        MVI@    R4,     R0      ; Channel a
        MVO@    R0,     R5
        MVI@    R4,     R0      ; Channel b
        MVO@    R0,     R5
        MVI@    R4,     R0      ; Channel c
        MVO@    R0,     R5
        EIS
        PULR    PC

@@waitpress:
        BNEQ    @@waitdone
@@waithand:
        MVII    #100,   R1      ; Debounce factor
@@waitloop:
        MVII    #$1FE,  R4
        SDBD
        MVI@    R4,     R0
        COMR    R0
        BC      @@waitpress
        BNEQ    @@waithand
@@waitdone:
        DECR    R1
        BNEQ    @@waitloop
        JR      R5

        ENDP

;;==========================================================================;;
;;  MAINLOOP                                                                ;;
;;  The main loop runs asynchronously from the MAINISR, running tasks as    ;;
;;  they get scheduled.  There is also a background task that runs when     ;;
;;  nothing else is in the queue.                                           ;;
;;                                                                          ;;
;;   -- Grab task from task queue, if any                                   ;;
;;       -- run that task                                                   ;;
;;       -- look for next task                                              ;;
;;   -- run background task once if no tasks available                      ;;
;;                                                                          ;;
;;  The background task scans hand controllers.  I do this because          ;;
;;  hand-controller scanning could be fairly expensive.  Also, scanning     ;;
;;  hand controllers may cause new tasks to get scheduled (eg. the triggers ;;
;;  for various hand controller events) and I don't want to overrun the     ;;
;;  task queue.                                                             ;;
;;                                                                          ;;
;;  The background task also updates the random number generator state.  I  ;;
;;  do this to make the random numbers a bit more random since the game     ;;
;;  loading varies according to what's going on.                            ;;
;;==========================================================================;;
MAINLOOP        PROC

        PSHR    R5              ; Save return address, since any
                                ; task is allowed to exit the main
                                ; game loop, say, when switching
                                ; between game phases.
                                ;
                                ; There are three game phases:
                                ;  -- Title Screen
                                ;  -- In Game
                                ;  -- Game Over.

        MVII    #EDGEB, R1      ; Set the well height so that music
        MVO     R1,     HEIGHT  ; plays correctly
        MVO     R1,     MUTE    ; Also, un-mute music.

@@loop:
        MOVR    PC,     R5      ; Set our return address to @@loop
        DECR    R5              ; This is 2 decles vs. 4 for SDBD/MVII

        ; Run next scheduled task from queue
        DIS                     ; Shut off interrupts
        MVI     TSKQTL, R1      ; Get the tail of the queue
        CMP     TSKQHD, R1      ; Are there tasks in the queue?
        BEQ     @@bktsk         ; No:  Do background task

        INCR    R1              ; Pop task from queue by
        ANDI    #$F,    R1      ; ... moving the queue tail
        MVO     R1,     TSKQTL  ; Store new task-queue tail
        MOVR    R1,     R4
        ADDR    R4,     R4
        ADDI    #TSKDQ, R4
        SDBD
        MVI@    R4,     R2      ; Load instance data for task
        ADDI    #TSKQ,  R1      ; Point to task queue
        MVI@    R1,     R0      ; Get function pointer from queue

        ; Dispatch to user's task.  Make sure interrupts are enabled again
        EIS                     ; Done with critical section
        JR      R0              ; Jump to subroutine
        ; Note that this implicitly loops back to @@oloop here

@@bktsk:
        EIS                     ; Done with critical section
        CALL    NEXTRAND        ; Update random number state
        CALL    SCANHAND        ; Scan the hand controllers.

EC_LOC  EQU     $CF00
EC_MAG  EQU     $69
EC_POLL EQU     $CF01

        MVI     EC_LOC, R0
        CMPI    #EC_MAG,R0
        BNEQ    @@loop
        CALL    EC_POLL

        B       @@loop          ; Run around the inner loop

        ENDP

;;==========================================================================;;
;;  QTASK                                                                   ;;
;;  Add a task to the task queue.                                           ;;
;;  NOTE: CALL THIS WITH INTERRUPTS OFF!!!!  (eg. use JSRD).  Interrupts    ;;
;;  will be enabled before return.                                          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Task function pointer                                             ;;
;;  R1 -- Task instance data                                                ;;
;;  R5 -- Return address.                                                   ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- intact.                                                           ;;
;;  R1 -- swapped                                                           ;;
;;  R2, R4 -- trashed.                                                      ;;
;;==========================================================================;;
QTASK   PROC
        MVI     TSKQHD, R2      ; Get head of task queue
        INCR    R2              ; Move to next queue slot
        ANDI    #$F,    R2      ; Stay in circular buffer
        CMP     TSKQTL, R2      ; Are we overflowing the queue?
        BEQ     @@overflow      ; Yes ... don't overflow it!
        MVO     R2,     TSKQHD  ; Store new task queue head

        MOVR    R2,     R4      ; Generate instance data pointer
        ADDR    R2,     R4      ; (remember mult by two for data ptr)
        ADDI    #TSKQ,  R2      ; Point into task queue
        MVO@    R0,     R2      ; Store function pointer to task q
; moved addi for STIC interruptibility fix
        ADDI    #TSKDQ, R4      ; Point into task data queue
        MVO@    R1,     R4      ; Store instance data to data q
        SWAP    R1              ; ....  cont'd
        MVO@    R1,     R4      ; ....  cont'd
        EIS                     ; Re-enable ints after crit section
        JR      R5              ; Return.

@@overflow:
        MVI     OVRFLO, R2
        INCR    R2
        MVO     R2,     OVRFLO

        EIS                     ; Re-enable ints after crit section
        JR      R5              ; Return.

        ENDP

;;==========================================================================;;
;;  SCHEDEXIT                                                               ;;
;;  Flushes the task queue and schedules the exit task.                     ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Return address.                                                   ;;
;;==========================================================================;;
SCHEDEXIT:      PROC
        PSHR    R5              ; Push return address.
        JSRD    R5,STOPALLTASKS ; Flush the pending task queue
        MVII    #@@exit, R0     ; Schedule the exit task ...
        PULR    R5
        B       QTASK           ; ... and chain the return.
@@exit: PULR    PC
        ENDP

;;==========================================================================;;
;;  STOPALLTASKS                                                            ;;
;;  Stops all tasks by setting the task count to zero and by                ;;
;;  flushing the task queue (sets head/tail to 0).  Returns old task count. ;;
;;  NOTE: CALL THIS WITH INTERRUPTS OFF!!!!  (eg. use JSRD)  Interrupts     ;;
;;  are NOT re-enabled by this function.                                    ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Previous TSKACT                                                   ;;
;;  R1 -- Cleared                                                           ;;
;;  All tasks stopped.                                                      ;;
;;==========================================================================;;
STOPALLTASKS    PROC
        MVI     TSKACT, R0
        CLRR    R1
        MVO     R1,     TSKACT
        MVO     R1,     TSKQHD
        MVO     R1,     TSKQTL
        DECR    R1
        MVO     R1,     TSKTBL+2        ; period == -1 for task 0
        MVO     R1,     TSKTBL+6        ; period == -1 for task 1
        MVO     R1,     TSKTBL+10       ; period == -1 for task 2
        NOP                             ; (interruptible, for STIC)
        MVO     R1,     TSKTBL+14       ; period == -1 for task 3
        JR      R5

        ENDP

;;==========================================================================;;
;;  STARTTASK                                                               ;;
;;  Puts a task into a task slot for general timer-based scheduling         ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Invocation record:                                                ;;
;;    DECLE -- Task number                                                  ;;
;;    DECLE -- Task function pointer                                        ;;
;;    DECLE -- Task initial period * 2. If bit 0==1, this is one-shot task. ;;
;;    DECLE -- Task period re-init. Useful if first delay!=recurring delay. ;;
;;  R2 -- Task instance data (optional)                                     ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  Task record is set up.  (User still needs to update TSKACT to make      ;;
;;  the task records active, if necessary.)                                 ;;
;;                                                                          ;;
;;  Registers R0, R4 are trashed.                                           ;;
;;==========================================================================;;
STARTTASK       PROC
        MVI@    R5,     R0
        SLL     R0,     2       ; R0 = R0 * 4 (four words/entry)
        MVII    #TSKTBL,R4      ; R4 = &TSKTBL[0]
        ADDR    R0,     R4      ; R4 = &TSKTBL[n]
        DIS                     ; Entering critical section.
        MVI@    R5,     R0      ; Get Function pointer
        MVO@    R0,     R4      ; ... and write it
        MVO@    R2,     R4      ; Write task instance data
        MVI@    R5,     R0      ; Get task period
        MVO@    R0,     R4      ; ... and write it
        MVI@    R5,     R0      ; Get task period reinit
        MVO@    R0,     R4      ; ... and write it
        EIS
        JR      R5
        ENDP

;;==========================================================================;;
;;  STOPTASK                                                                ;;
;;  Stops a given task by setting its period to -1.  Task can be            ;;
;;  retriggered/restarted by calling RETRIGGERTASK.                         ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R3 -- Task number.                                                      ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  Task is disabled.                                                       ;;
;;  R0 == -1, R3 points to task delay count.                                ;;
;;==========================================================================;;
STOPTASK        PROC
        SLL     R3,     2       ; R3 = R3 * 4 (four words/entry)
        ADDI    #TSKTBL+2, R3   ; R3 = &TSKTBL[n].delay
        CLRR    R0
        DECR    R0              ; R0 == -1.
        MVO@    R0,     R3      ; TSKTBL[n].delay = -1
        JR      R5
        ENDP

;;==========================================================================;;
;;  RETRIGGERTASK                                                           ;;
;;  Restarts a task by copying its delay re-init field to its delay field.  ;;
;;  Can be used on one-shot tasks or tasks which have been stopped with     ;;
;;  'StopTask'.                                                             ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R3 -- Task number.                                                      ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Task delay                                                        ;;
;;  R3 -- points to task delay count                                        ;;
;;==========================================================================;;

RETRIGGERTASK:  PROC
        SLL     R3,     2       ; R3 = R3 * 4 (four words/entry)
        ADDI    #TSKTBL+3, R3   ; R3 = &TSKTBL[n].reload
        DIS
        MVI@    R3,     R0      ; Get reload value
@@chain:
        DECR    R3              ; Offset 2 is delay count
        MVO@    R0,     R3
        EIS
        JR      R5

        ENDP

;;==========================================================================;;
;;  WAIT                                                                    ;;
;;  Busy-waits for the number of ticks specified in R0.                     ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Number of ticks to wait                                           ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- cleared                                                           ;;
;;==========================================================================;;
WAIT    PROC
        MVI@    R5,     R0
        MVO     R0,     WTIMER
        CLRR    R0
@@loop:
        CMP     WTIMER, R0
        BNEQ    @@loop
        JR      R5
        ENDP


;;==========================================================================;;
;;  SLEEP                                                                   ;;
;;  Causes a one-shot process to sleep by scheduling a second one-shot      ;;
;;  process to reawake it.  This is VERY DIFFICULT TO USE, and can be       ;;
;;  used by ONE PROCESS ONLY at a time.                                     ;;
;;                                                                          ;;
;;  SPAWN                                                                   ;;
;;  Same as sleep, only the sleeping task isn't a one-shot.  It keeps       ;;
;;  retriggering automatically at the designated interval, or can be        ;;
;;  retriggered manually if the period is odd.                              ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Invocation record, in following format:                           ;;
;;        Period/sleep time  1 Decle  (Must be ODD for SLEEP)               ;;
;;        Process # to use   1 Decle                                        ;;
;;  Top of Stack -- Return address from _this_ process back to scheduler.   ;;
;;  Task will wake at address after invocation record.                      ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0, R1, R2, R3, R4 restored                                             ;;
;;  R5 points to your return address                                        ;;
;;  Top of stack has return address for scheduler                           ;;
;;==========================================================================;;
SLEEP:  PROC
SPAWN:
        MVO     R4,     REGSV+4 ; Save R4
        MVII    #REGSV, R4
        MVO@    R0,     R4      ; Save R0
        MVO@    R1,     R4      ; Save R1
        MVO@    R2,     R4      ; Save R2
        MVO@    R3,     R4      ; Save R3

        MVI@    R5,     R2      ; Get sleep time
        MVI@    R5,     R3      ; Get PID

        SLL     R3,     2       ; R3 = R3 * 4 (four words/entry)
        MVII    #TSKTBL,R4      ; R4 = &TSKTBL[0]
        ADDR    R3,     R4      ; R4 = &TSKTBL[n]
        MVII    #@@wake,R0
        MVO@    R0,     R4      ; Address to wake up at
        MVO@    R5,     R4      ; Instance data (R2 restore)
        MVO@    R2,     R4
        MVO@    R2,     R4

        PULR    PC

@@wake:
        PSHR    R5              ; Remember return address
        MOVR    R2,     R5
        MVII    #REGSV, R4      ; Point to register save area
        MVI@    R4,     R0      ; Restore R0
        MVI@    R4,     R1      ; Restore R1
        MVI@    R4,     R2      ; Restore R2
        MVI@    R4,     R3      ; Restore R3
        MVI@    R4,     R4      ; Restore R4

        JR      R5              ; Go there

        ENDP

;;==========================================================================;;
;;  DOFACE                                                                  ;;
;;==========================================================================;;
DOFACE  PROC
        DIS
        MVII    #$2F0,  SP

        MVII    #@@loadface, R0
        MVO     R0,     PREVBL
        SWAP    R0
        MVO     R0,     PREVBL+1

        CLRR    R0
        MVII    #$F0,   R1
        MVII    #$200,  R4
        CALL    FILLMEM

        MVII    #$200 + 8 + 20, R4
        MVII    #FACE,  R5
        MVII    #9,     R0
        MVO     R0,     CB
        MVO     R0,     CS0
@@f_oloop:
        MVII    #6,     R2
@@f_iloop:
        MVI@    R5,     R1
        MVO@    R1,     R4
        DECR    R2
        BNEQ    @@f_iloop
        ADDI    #14,    R4
        DECR    R0
        BNEQ    @@f_oloop

        CALL    DRAWSTRING3
        DECLE   0,      $200 + 20*10
                ;01234567890123456789
        BYTE    $4A, $6F, $65, $20, $5A, $62, $69, $63, $69, $61, $6B
        BYTE    $20, $73, $61, $79, $73, $20, $48, $49, $21, $0A, $00

        EIS
        DECR    PC

@@loadface:
        MVII    #@@shunt, R0
        MVO     R0,     PREVBL
        SWAP    R0
        MVO     R0,     PREVBL+1

        MVII    #9,     R0
        MVO     R0,     CS0
        MVO     R0,     CB
        
        CALL    LOADFONT
        DECLE   FACEFONT
@@shunt:
        MVO     R0,     $20
        PULR    PC
        ENDP

;;==========================================================================;;
;;  Colored squares pixel-manipulation routines.                            ;;
;;  J. Zbiciak, July 1999                                                   ;;
;;==========================================================================;;

; Data tables for pixel routines
MSKTBL:
        DECLE   $0007, $0038, $01C0, $2600

CLRTBL:
        DECLE   $0000, $0249, $0492, $06DB
        DECLE   $2124, $236D, $25B6, $27FF


;;==========================================================================;;
;;  PIXCALC -- Pixel Calculation:  Convert x,y into addr,mask               ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R1 -- Pixel mask                                                        ;;
;;  R2 -- Address in display memory.                                        ;;
;;  R3, R4 -- Clobbered.                                                    ;;
;;==========================================================================;;
PIXCALC         PROC
        SLL     R2,     1       ; y' = y << 1
        MOVR    R2,     R4      ; mskidx = y << 1
        ANDI    #2,     R4      ; mskidx = (y << 1) & 2
        SUBR    R4,     R2      ; y' = (y & ~1) << 1
        MOVR    R2,     R3      ; save y'
        SLL     R2,     2       ; y'' = (y & ~1) << 3
        ADDR    R3,     R2      ; y''' = y' + y'' = (y >> 1) * 20
        SARC    R1,     1       ; c = x & 1;  x' = x >> 1
        ADCR    R4              ; mskidx = ((y << 1) & 2) + (x & 1)
        ADDR    R1,     R2      ; ofs = x' + y''' = (x >> 1) + (y >> 1) * 20
        ADDI    #$200,  R2      ; ptr = ofs + $200 = &BACKTAB[ofs] ==> R2
        ADDI    #MSKTBL,R4      ; mskptr = MSKTBL + mskidx' = &MSKTBL[mskidx]
        MVI@    R4,     R1      ; mask = *mskptr ==> R1
        JR      R5              ; return

        ENDP

;;==========================================================================;;
;;  PUTPIXEL -- Put pixel at x, y coordinates with color 'c' (0-7)          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R1 -- Word stored at R2                                                 ;;
;;  R2 -- Display memory address                                            ;;
;;  R0, R3, R4, R5 -- Clobbered.                                            ;;
;;==========================================================================;;

PUTPIXEL        PROC
        PSHR    R5              ; Save return address
        MVII    #PUTPIXELR+1,R5 ; Return to actual pixel draw below
        B       PIXCALC         ; Convert x,y into mask,addr

;;==========================================================================;;
;;  PUTPIXELR -- Put pixel 'raw': accepts address, mask, and color          ;;
;;               (Alternate entry point for PUTPIXEL.)                      ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- Pixel mask                                                        ;;
;;  R2 -- Display memory address                                            ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R1 -- Word stored at R2                                                 ;;
;;  R2 -- Display memory address                                            ;;
;;  R0,R5 -- Clobbered.                                                     ;;
;;==========================================================================;;
PUTPIXELR:
        PSHR    R5              ; Save return address
        MOVR    R0,     R5
        ADDI    #CLRTBL,R5      ; clrptr = CLRTBL + c' = &CLRTBL[c]
        MVI@    R5,     R0      ; color = CLRTBL[c]
        ANDR    R1,     R0      ; color = color & mask  (select desired pix)
        COMR    R1              ; invert mask
        AND@    R2,     R1      ; pix = BACKTAB & ~mask (clear pixel)
        ADDR    R0,     R1      ; pix' = pix + color    (merge pixels)
        MVO@    R1,     R2      ; BACKTAB[ofs] = pix'   (write pixels)
        PULR    PC              ; return.

        ENDP

;;==========================================================================;;
;;  PIXELCLIP                                                               ;;
;;  Returns with C==1 if pixel is onscreen                                  ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  Carry == 1 if onscreen, 0 if offscreen.                                 ;;
;;==========================================================================;;

PIXELCLIP       PROC
        TSTR    R1              ; Off left?
        BMI     @@offscr        ; Yes:  Set carry and exit
        TSTR    R2              ; Off top?
        BMI     @@offscr        ; Yes:  Set carry and exit
        CMPI    #39,    R1      ; Off right?
        BGT     @@offscr        ; Yes:  Set carry and exit
        CMPI    #23,    R2      ; Off bottom?
        BGT     @@offscr        ; Yes:  Set carry and exit
        SETC                    ; No to all: Clear carry and exit
        JR      R5
@@offscr
        CLRC
        JR      R5
        ENDP

;;==========================================================================;;
;;  GETPIXELS -- Calls GETPIXEL, but saves/restores X, Y                    ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return addr.                                                      ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R3, R4, R5 -- Clobbered.                                                ;;
;;==========================================================================;;
GETPIXELS       PROC
        PSHR    R5
@@chain:
        MVO     R1,     XSAVE
        MVO     R2,     YSAVE
        CALL    GETPIXEL
        MVI     XSAVE,  R1
        MVI     YSAVE,  R2
        PULR    PC
        ENDP


;;==========================================================================;;
;;  PUTPIXELS -- Calls PUTPIXEL, but saves/restores X, Y                    ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return addr.                                                      ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R3, R4, R5 -- Clobbered.                                                ;;
;;==========================================================================;;
PUTPIXELS       PROC
        PSHR    R5
@@chain:
        MVO     R1,     XSAVE
        MVO     R2,     YSAVE
        MVO     R0,     CSAVE
        CALL    PUTPIXEL
        MVI     CSAVE,  R0
        MVI     XSAVE,  R1
        MVI     YSAVE,  R2
        PULR    PC
        ENDP

;;==========================================================================;;
;;  GETPIXELSC                                                              ;;
;;  Calls GETPIXELS or returns R0 unchanged if pixel is off screen          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Color to return if off-screen                                     ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R5 -- Return addr.                                                      ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R3, R4, R5 -- Clobbered.                                                ;;
;;==========================================================================;;
GETPIXELSC      PROC
        PSHR    R5              ; Save return address
        CALL    PIXELCLIP       ; See if pixel is onscreen
        ADCR    PC              ; If it is, then skip the return
        PULR    PC              ; Otherwise return.
        B       GETPIXELS.chain ; If onscreen chain over to GETPIXELS
        ENDP


;;==========================================================================;;
;;  GETPIXEL -- Get pixel at x, y address                                   ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Color                                                             ;;
;;  R2 -- Display memory address                                            ;;
;;  R1, R3, R4, R5 -- Clobbered.                                            ;;
;;==========================================================================;;
GETPIXEL        PROC
        PSHR    R5              ; Save return address
        MVII    #GETPIXELR+1,R5 ; Return to actual pixel read below
        B       PIXCALC         ; Convert x,y into mask,addr

;;==========================================================================;;
;;  GETPIXELR -- Get pixel 'raw': accepts address, mask                     ;;
;;               (Alternate entry point for getpixel)                       ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- Pixel mask                                                        ;;
;;  R2 -- Display memory address                                            ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Color                                                             ;;
;;  R2 -- Display memory address                                            ;;
;;  R1, R5 -- Clobbered.                                                    ;;
;;==========================================================================;;
GETPIXELR:
        PSHR    R5              ; Save return address
        MVI@    R2,     R0      ; pix = BACKTAB[ofs]
        ANDR    R1,     R0      ; pix' = pix & mask
        MOVR    R0,     R1      ;
        SWAP    R0,     1       ;
        ANDI    #$20,   R0      ;
        ADDR    R1,     R0      ; merge oddball bit from lower right pixel
        SLL     R1,     2       ;
        SWAP    R1,     1       ;
        ADDR    R1,     R0      ; Fold upper, lower pairs of pixels
        MOVR    R0,     R1      ;
        SLR     R0,     1       ;
        SLR     R0,     2       ;
        ADDR    R1,     R0      ; Fold left, right pixels
        ANDI    #7,     R0      ; Select remaining pixel
        PULR    PC              ; return

        ENDP


;;==========================================================================;;
;;  PUTPIXELSCQ                                                             ;;
;;                                                                          ;;
;;  Queues a pixel for display if it's onscreen.  This routine does not     ;;
;;  draw the pixel directly, but rather puts it in a queue of pixels that   ;;
;;  are waiting to be drawn.  This queue is emptied during the interrupt    ;;
;;  service routine after the queue is "committed" for display with a call  ;;
;;  to COMMITPXQ.                                                           ;;
;;                                                                          ;;
;;  The pixel queue serves as a mechanism for reducing/eliminating the      ;;
;;  flicker associated with redrawing a game piece.  Without the queue,     ;;
;;  piece movements could result in a significant amount of flicker.        ;;
;;  To further reduce flicker, the queue insert routine attempts to         ;;
;;  optimize multiple 'PUTPIXELs' to the same location, with the last       ;;
;;  PUTPIXEL taking precedence.                                             ;;
;;                                                                          ;;
;;  Because this queue is intended for drawing game pieces only, it is      ;;
;;  not very large.  No checking is performed for queue overflow, either.   ;;
;;  Also, because double-buffering is not performed, this routine will      ;;
;;  block if there is a committed pixel queue waiting to be drawn.          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;  R0 -- Color                                                             ;;
;;  R1 -- X coordinate                                                      ;;
;;  R2 -- Y coordinate                                                      ;;
;;  R3, R4, R5 -- Clobbered.                                                ;;
;;==========================================================================;;
PUTPIXELSCQ     PROC
        PSHR    R5              ; Save return address
        CALL    PIXELCLIP       ; See if pixel is onscreen
        ADCR    PC              ; If it is, then skip the return
        PULR    PC              ; Otherwise return.

        ; Merge Y coordinate and color up-front, since we store
        ; them merged in the queue.
        MOVR    R0,     R3      ; Work on a copy of our color
        SWAP    R3
        SLR     R3,     2
        SLR     R3,     1       ; R3 = color << 5
        XORR    R3,     R2      ; Put color in bits 5..7 of R2

        DIS     ; Critical section: Queue management.
        ; First, check to see if any pixels are queued at all.  If there
        ; aren't any, just jump straight to the queue-insert.
        MVI     PXQ_L0, R3
        TSTR    R3
        BEQ     @@insert_eis

        ; Next, check to see if there are any committed pixels waiting
        ; to be drawn in the pixel queue.  Wait until the queue is drained.
        CLRR    R3
        CMP     PXQ_L1, R3      ; If there are no committed pixels, we
        EIS     ; Interrupts can be enabled now.
        BEQ     @@check         ; still need to check for redundant pixels.
@@drain:
        CMP     PXQ_L1, R3      ; Otherwise, let the queue drain and jump
        BNEQ    @@drain         ; straight to the insert since it's empty
        B       @@insert

@@check:
        ; If we reach here, we have a non-empty pixel queue, and no
        ; committed pixels.  Step through the queue to see if we have
        ; any redundant pixels.
        MVII    #PXQ_XY - 1, R4 ; Point at start of Queue minus 1
        MVI     PXQ_L0, R3      ; Get queue length

@@notx: INCR    R4              ; Where we branch when X didn't match.
@@noty: DECR    R3              ; Decrement loop count
        BMI     @@insert        ; Get out of here when it expires.
@@rloop:
        CMP@    R4,     R1      ; Compare X coords
        BNEQ    @@notx          ; X miscompared.

        MOVR    R2,     R5
        XOR@    R4,     R5
        ANDI    #$1F,   R5      ; Cmp Y coords (lower 5 bits of queue entry)
        BNEQ    @@noty          ; Y miscompared

        ; If we get here, the coordinates match, so just update the color
        ; and exit the queue insertion process entirely.
        DECR    R4
        B       @@styc          ; Store Y coordinate and color, and leave

@@insert_eis:
        EIS
@@insert:
        ; If we get here, then this is not a duplicate pixel, so we need
        ; to extend the queue.  Warning:  No bounds checking is done here.
        MVI     PXQ_L0, R3
        MOVR    R3,     R4      ; Copy the old queue length to R4
        ADDR    R3,     R4      ; Multiply it by 2
        INCR    R3
        MVO     R3,     PXQ_L0
        ADDI    #PXQ_XY, R4     ; Point R4 at pixel queue

@@stx:  MVO@    R1,     R4      ; Store the X coordinate
@@styc: MVO@    R2,     R4      ; Store the Y coordinate and color

        ANDI    #$1F,   R2      ; Strip the color from the Y + color.

        PULR    PC              ; Return.

        ENDP

;;==========================================================================;;
;;  DRAINPXQ                                                                ;;
;;  Drains the pixel queue (intended to be called from an ISR).             ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Return address                                                    ;;
;;  Pixels in the committed pixel queue                                     ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0..R5 clobbered.                                                       ;;
;;==========================================================================;;
DRAINPXQ        PROC
        PSHR    R5
        DIS                     ; Critical section: Grab committed pixels.
        MVI     PXQ_L1, R3
        TSTR    R3
        BEQ     @@leave
        CLRR    R1
        MVO     R1,     PXQ_L0  ; Clear count of queued pixels.
        MVO     R1,     PXQ_L1  ; Clear count of comitted pixels.
        EIS
        MVII    #PXQ_XY, R4
@@pixloop:
        MVI@    R4,     R1      ; Get X coordinate
        MVI@    R4,     R2      ; Get Y coordinate and color
        MOVR    R2,     R0      ; Separate Y coordinate and color
        SLL     R0,     2
        SLL     R0,     1
        SWAP    R0              ; Color is now in 3 lsbs.
        ANDI    #$7,    R0      ; Mask away undesired bits in color
        ANDI    #$1F,   R2      ; Mask away undesired bits in Y coord.
        PSHR    R3
        PSHR    R4
        CALL    PUTPIXEL        ; Display the pixel
        PULR    R4
        PULR    R3
        DECR    R3              ; Decrement our loop count
        BNEQ    @@pixloop       ; Keep looping until we've displayed them all
@@leave:
        EIS
        PULR    PC
        ENDP

;;==========================================================================;;
;;  COMMITPXQ                                                               ;;
;;  Commits queued pixels in the pixel queue to be displayed.               ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R5 -- Return address                                                    ;;
;;  Pixels in the pixel queue.                                              ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  Pixels are committed for display.                                       ;;
;;==========================================================================;;
COMMITPXQ       PROC
        PSHR    R5              ; Save return address on stack.
        DIS                     ; Disable interrupts (critical section)
        MVI     PXQ_L1, R5      ; See if there are already committed pixels
        TSTR    R5              ; ... waiting to be drawn.  (oops!)
        BNEQ    @@leave         ; Yes.. then leave.
        MVI     PXQ_L0, R5      ; No?  Get our count of non-committed pixels.
        MVO     R5,     PXQ_L1  ; Store it to our count of committed pixels.
@@leave:
        EIS                     ; Done with critical section.
        PULR    PC              ; Return to caller
        ENDP

;;==========================================================================;;
;;  PIECE                                                                   ;;
;;  The Piece Shape Table                                                   ;;
;;                                                                          ;;
;;  Shape encoding for the game pieces                                      ;;
;;                                                                          ;;
;;    The game pieces are encoded as a series of moves from the "center     ;;
;;    point".  Blocks are placed _after_ each move.  All pieces are         ;;
;;    composed of four moves except piece 6 (the T) which requires 5        ;;
;;    moves, and piece 5 which uses an extra move so that it is centered    ;;
;;    in the "next piece" box.                                              ;;
;;                                                                          ;;
;;    Each move is encoded in two bits.  The LS bit specifies the axis      ;;
;;    (up/down or left/right), and the MS bit specifies the sign of         ;;
;;    the direction on that axis (1==+1 and 0==-1).  Moves are processed    ;;
;;    from the LS bit pair up to the MS bit pair.  This encoding allows     ;;
;;    each piece to be described in exactly one decle.                      ;;
;;                                                                          ;;
;;    NOTE:  The fifth move may NOT be "up".  The rotation code             ;;
;;    distiguishes between four-move and five-move pieces by whether the    ;;
;;    upper two bits are zero.  If they're both zero, it's a four-move      ;;
;;    piece, otherwise the two bits specify the fifth move.  (Recall that   ;;
;;    00 corresponds to "up".)                                              ;;
;;                                                                          ;;
;;    Right now, rotations are performed in a brute-force manner:  We       ;;
;;    just encode all of the rotations in our data set here.  It's          ;;
;;    seems to be alot cheaper than writing the code to rotation code,      ;;
;;    although I can't be sure unless I do them both.  At the very least,   ;;
;;    the brute-force approach is alot easier.                              ;;
;;                                                                          ;;
;;  Notation:  pXrY is "Piece #X, Rotation #Y".  X == 0..6, Y == 0..3.      ;;
;;                                                                          ;;
;;             up#, dn#, lf# and rt# indicate a movement in a direction     ;;
;;             on a given move number.                                      ;;
;;                                                                          ;;
;;             Pieces are shown in the comments in their initial            ;;
;;             orientation.  The block marked with %% is the "pivot"        ;;
;;             block.                                                       ;;
;;==========================================================================;;
PIECE   PROC

; handy constants for defining the moves.
@@up0   EQU     0000000000b
@@dn0   EQU     0000000010b
@@lf0   EQU     0000000001b
@@rt0   EQU     0000000011b

@@up1   EQU     0000000000b
@@dn1   EQU     0000001000b
@@lf1   EQU     0000000100b
@@rt1   EQU     0000001100b

@@up2   EQU     0000000000b
@@dn2   EQU     0000100000b
@@lf2   EQU     0000010000b
@@rt2   EQU     0000110000b

@@up3   EQU     0000000000b
@@dn3   EQU     0010000000b
@@lf3   EQU     0001000000b
@@rt3   EQU     0011000000b

;;up4           not allowed
@@dn4   EQU     1000000000b
@@lf4   EQU     0100000000b
@@rt4   EQU     1100000000b

; Piece 0:  The Long Bar
;
;  ###
;  ###
;  %%%
;  %%%
;  ###
;  ###
;  ###
;  ###
;
@@p0r0  BYTE    @@up0 + @@dn1 + @@dn2 + @@dn3
@@p0r1  BYTE    @@lf0 + @@rt1 + @@rt2 + @@rt3
@@p0r2  BYTE    @@up0 + @@dn1 + @@dn2 + @@dn3
@@p0r3  BYTE    @@lf0 + @@rt1 + @@rt2 + @@rt3

; Piece 1:  The Square Block
;
;  ###%%%
;  ###%%%
;  ######
;  ######
;
@@p1r0  BYTE    @@dn0 + @@lf1 + @@up2 + @@rt3
@@p1r1  BYTE    @@dn0 + @@lf1 + @@up2 + @@rt3
@@p1r2  BYTE    @@dn0 + @@lf1 + @@up2 + @@rt3
@@p1r3  BYTE    @@dn0 + @@lf1 + @@up2 + @@rt3

; Piece 2:  The L shape
;
;  ###%%%###
;  ###%%%###
;  ###
;  ###
;
@@p2r0  BYTE    @@rt0 + @@lf1 + @@lf2 + @@dn3
@@p2r1  BYTE    @@dn0 + @@up1 + @@up2 + @@lf3
@@p2r2  BYTE    @@lf0 + @@rt1 + @@rt2 + @@up3
@@p2r3  BYTE    @@up0 + @@dn1 + @@dn2 + @@rt3

; Piece 3: The Reverse-L shape
;
;  ###%%%###
;  ###%%%###
;        ###
;        ###
;
@@p3r0  BYTE    @@lf0 + @@rt1 + @@rt2 + @@dn3
@@p3r1  BYTE    @@up0 + @@dn1 + @@dn2 + @@lf3
@@p3r2  BYTE    @@rt0 + @@lf1 + @@lf2 + @@up3
@@p3r3  BYTE    @@dn0 + @@up1 + @@up2 + @@rt3

; Piece 4: The S shape
;
;     %%%###
;     %%%###
;  ######
;  ######
;
@@p4r0  BYTE    @@rt0 + @@lf1 + @@dn2 + @@lf3
@@p4r1  BYTE    @@up0 + @@dn1 + @@rt2 + @@dn3
@@p4r2  BYTE    @@rt0 + @@lf1 + @@dn2 + @@lf3
@@p4r3  BYTE    @@up0 + @@dn1 + @@rt2 + @@dn3

; Piece 5: The Z shape
;
;  ###SSS          Note: SSS brick is the actual starting brick.
;  ###SSS
;     %%%###
;     %%%###
;
@@p5r0  DECLE   @@dn0 + @@rt1 + @@lf2 + @@up3 + @@lf4
@@p5r1  DECLE   @@dn0 + @@up1 + @@dn2 + @@lf3 + @@dn4
@@p5r2  DECLE   @@dn0 + @@rt1 + @@lf2 + @@up3 + @@lf4
@@p5r3  DECLE   @@dn0 + @@up1 + @@dn2 + @@lf3 + @@dn4

; Piece 6: The T shape
;
;  ###%%%###
;  ###%%%###
;     ###
;     ###
;
@@p6r0  DECLE   @@lf0 + @@rt1 + @@dn2 + @@up3 + @@rt4
@@p6r1  DECLE   @@up0 + @@dn1 + @@lf2 + @@rt3 + @@dn4
@@p6r2  DECLE   @@lf0 + @@rt1 + @@up2 + @@dn3 + @@rt4
@@p6r3  DECLE   @@up0 + @@dn1 + @@rt2 + @@lf3 + @@dn4

        ENDP


;;==========================================================================;;
;;  PUTPIECEREC                                                             ;;
;;  Writes a piece record to memory.                                        ;;
;;                                                                          ;;
;;  PUTPIECEREC.NC                                                          ;;
;;  Writes a piece record to memory, except color.                          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- piece color                                                       ;;
;;  R1 -- X coord of pivot                                                  ;;
;;  R2 -- Y coord of pivot                                                  ;;
;;  R3 -- piece number/rotation                                             ;;
;;  R4 -- Address of piece information record (+1 if PUTPIECEREC.NC)        ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0..R3, R5 -- not modified                                              ;;
;;  R4 -- points just past end of of piece information record               ;;
;;==========================================================================;;
PUTPIECEREC     PROC
        MVO@    R0,     R4      ; Put piece color
@@NC:
        MVO@    R1,     R4      ; Put piece pivot X coordinate
        MVO@    R2,     R4      ; Put piece pivot Y coordinate
        MVO@    R3,     R4      ; Put piece number/rotation

        JR      R5              ; Return
        ENDP

;;==========================================================================;;
;;  GETPIECEREC                                                             ;;
;;  Reads a piece record from memory.                                       ;;
;;                                                                          ;;
;;  GETPIECEREC.NC                                                          ;;
;;  Reads a piece record from memory, except color.                         ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R4 -- Address of piece information record (+1 if GETPIECEREC.NC)        ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- piece color (unless GETPIECEREC.NC)                               ;;
;;  R1 -- X coord of pivot                                                  ;;
;;  R2 -- Y coord of pivot                                                  ;;
;;  R3 -- piece number/rotation                                             ;;
;;==========================================================================;;
GETPIECEREC     PROC
        MVI@    R4,     R0      ; Get piece color
@@NC:
        MVI@    R4,     R1      ; Get piece pivot X coordinate
        MVI@    R4,     R2      ; Get piece pivot Y coordinate

        ; Sign-extend R1 and R2 since they're in 8-bit RAM.
        MVII    #$7F,   R3      ; Generate constant $FF80
        COMR    R3              ; R3 is "magic sign-extend constant"

        ADDR    R3,     R1      ; Propogate sign-bit info to upper-half
        XORR    R3,     R1      ; R1 is fully sign-extended here.

        ADDR    R3,     R2      ; Propogate sign-bit info to upper-half
        XORR    R3,     R2      ; R2 is fully sign-extended here.

        MVI@    R4,     R3      ; Get piece number/rotation

        JR      R5              ; Return
        ENDP

;;==========================================================================;;
;;  DRAWPIECE                                                               ;;
;;  Draws a piece on the screen in the desired color (0..7).                ;;
;;                                                                          ;;
;;  TESTPIECE                                                               ;;
;;  Determines if a piece can be drawn in a given location by tracing its   ;;
;;  path and doing GETPIXELSC calls.                                        ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- If DRAWPIECE, color of piece to draw (0..7)                       ;;
;;  R1 -- X coordinate of pivot                                             ;;
;;  R2 -- Y coordinate of pivot                                             ;;
;;  R3 -- Piece number                                                      ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- If TESTPIECE, Zero == OK, non-zero == not OK; else, trashed.      ;;
;;  R4 -- Minimum Y coordinate                                              ;;
;;  R5 trashed.                                                             ;;
;;                                                                          ;;
;;  $130..$135 used for temporary storage.                                  ;;
;;==========================================================================;;
DRAWPIECE       PROC
@@colsv EQU     $130            ; Piece color save area
@@pvxsv EQU     $131            ; Pivot X coordinate save area
@@pvysv EQU     $132            ; Pivot Y coordinate save area
@@pnrsv EQU     $133            ; Piece number/rotation save area
@@pcwsv EQU     $134            ; Loop count save area
@@cntsv EQU     $135            ; Loop count save area
@@miny  EQU     $136

        ADDR    R0,     R0      ; Shift color left by 1
        INCR    R0              ; Set the LSB on the color
        INCR    PC              ; Skip the CLRR R0
TESTPIECE:
        CLRR    R0              ; Start with color = 0, LSB = 0

        PSHR    R5              ; Save return address
        MVII    #@@colsv, R4    ; Point R4 at save area
        CALL    PUTPIECEREC

        MVO     R2,     @@miny  ; Start with Min Y == pivot point

        ; Convert piece number to control word.
        ADDI    #PIECE, R3      ; Point into PIECE array entry for piece
        MVI@    R3,     R3      ; Get the piece control word

        MVII    #3,     R4      ; At least four moves in piece

@@loop:
        MOVR    R3,     R0      ; Copy Piece Control Word
        ANDI    #2,     R0      ; Get sign for move (set == +1, clear == -1)
        DECR    R0              ; R0 is either +1/-1 according to move
        SARC    R3,     2       ; Shift move away from PCW. Carry==axis
        BC      @@xaxis         ; Carry Clear == Y-Axis, Carry Set == X-Axis
        ADDR    R0,     R2      ; Add move to Y axis
        ; See if this Y coordinate is our new min-Y coord
        CMP     @@miny, R2
        BGE     @@cont
        MVO     R2,     @@miny

        INCR    PC              ; Skip add-to-X (next instruction)
@@xaxis:
        ADDR    R0,     R1      ; Add move to X axis
@@cont:

        ; Note, at most 8 bits remain in PCW here, so we save in 8-bit loc.
        MVO     R3,     @@pcwsv ; Save remaining Piece Control Word (max 8 bit)
        MVO     R4,     @@cntsv ; Save our loop trip count
        MVI     @@colsv, R0     ; Get saved color
        SARC    R0,     1       ; If bit-0 set, this is DRAW, else TEST
        BC      @@draw          ;
@@test:
        ; Note R0 should be 0 here. This treats off-screen pixels as black,
        ; which is what we want.
        CMPI    #EDGEL, R1      ; Are we off left?
        BLT     @@notok         ; Yes:  Bad!
        CMPI    #EDGER, R1      ; Are we off left?
        BGT     @@notok         ; Yes:  Bad!
        CALL    GETPIXELSC      ; Get pixel, with clipping.
        CMPI    #7,     R0      ; Ignore color 7, since we use that
        BEQ     @@pixelok       ;    for the active piece.
        TSTR    R0              ; Otherwise, make sure it's black.
        BEQ     @@pixelok       ;
        B       @@notok         ; Didn't pass the tests?  It's not ok then.
@@draw:
        CALL    PUTPIXELSCQ     ; Put the pixel on the screen, with clipping.
@@pixelok:

        MVI     @@pcwsv, R3     ; Get our saved piece number
        MVI     @@cntsv, R4     ; Get our loop trip count.
        DECR    R4
        BPL     @@loop          ; Loop as long as R4 > 0

        CLRR    R4
        TSTR    R3              ; See if this is a 5-move piece.
        BNEQ    @@loop          ; Loop one last time if it is.

        MVI     @@colsv, R0     ; Restore color register (if TESTPIECE is ok,
        SLR     R0,     1       ; ...OR if this is DRAWPIECE).
        INCR    PC              ; (skip the MOVR.)

@@notok:                        ; Else, skip the restore if TESTPIECE and
        MOVR    PC,     R0      ; ...pixel is not ok, so force R0 non-zero

        MVII    #@@pvxsv, R4    ; Point to save area + 1
        CALL    GETPIECEREC.NC
        MVI     @@miny, R4      ; Get minimum Y value into R4

        PULR    PC              ; Return to the caller
        ENDP


;;==========================================================================;;
;;  GETPIECECOLOR                                                           ;;
;;  Gets Piece Color, given the piece number/rotation.                      ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R3 -- Piece number/rot.  Bits 0..1 are rotation, Bits 2..4 are piece #  ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Piece color                                                       ;;
;;==========================================================================;;
GETPIECECOLOR   PROC
        PSHR    R3              ; Save R3 so that it's not clobbered
        SLR     R3,     2       ; Generate index into color table
        ADDI    #@@color,R3     ; R3 = PCCOLOR[piece]
        MVI@    R3,     R0      ; Get the piece color into R0
        PULR    R3              ; Restore R3
        JR      R5              ; Return.

@@color BYTE    $2, $1, $3, $4, $5, $6, $1
        ENDP

;;==========================================================================;;
;;  PICKPIECE                                                               ;;
;;  Grabs the 'next piece' and makes it the current piece.  Then picks a    ;;
;;  new 'next piece'.                                                       ;;
;;==========================================================================;;
PICKPIECE       PROC
        PSHR    R5
@@chain:
@@randloop:
        MVII    #4,     R2      ; Generate some new random bits

        CALL    NEXTRANDX
        ANDI    #$1C,   R0      ; Mask to [0..7] * 4
        BEQ     @@randloop      ; If it comes up 0, try again

        SUBI    #4,     R0      ; Make into piece #0..#6

        CALL    NEXTCLEAR       ; Clear previous 'next-piece'
        MVI     NXTPC,  R3      ; Get previous 'next-piece'.
        MVO     R0,     NXTPC   ; Save new 'next-piece'.

        MVI     SHOWNXT, R1     ; Are we showing next piece?
        MVO     R1,     DIDNXT  ; Initialize our "showed next piece" flag
        TSTR    R1
        BEQ     @@noshow

        PSHR    R3              ; Save R3 -- new current piece number
        CALL    NEXTSHOW        ; Show next piece
        PULR    R3              ; Restore R3

@@noshow:
        CALL    GETPIECECOLOR   ; Get the current piece's color.
        MVO     R0,     CS1     ; Force color-stack to piece's color
        MVO     R0,     CS3     ; Force color-stack to piece's color

        MVII    #EDGEL+5, R1    ; Start out in middle of well...
        MVII    #3,       R2    ; ...just a little off top of screen
        COMR    R2              ; (Y = -3)
        MVII    #CURPC.C, R4    ; Write to "current piece" record.
        CALL    PUTPIECEREC     ; Make this the current piece.

        PULR    PC              ; Return.
        ENDP

;;==========================================================================;;
;;  CLEARWELL                                                               ;;
;;  Clears the well in a dramatic fashion                                   ;;
;;==========================================================================;;
CLEARWELL       PROC
@@bot   EQU     $1E2
        PSHR    R5

        CLRR    R0
        MVO     R0,     CURPC.C ; make sure current piece number is cleared.

@@wellloop:
        MVII    #EDGEB, R3
        CMP     HEIGHT, R3      ; R3 has actual well height.
        BLE     @@return        ; Do nothing if well is empty.

@@randloop:
        MVII    #5,     R2
        CALL    NEXTRANDX       ; Get a random number (advance by 5 bits)
        ANDI    #31,    R0
        ADD     HEIGHT, R0
        CMPR    R3,     R0      ; Make sure it's less than the well height.
        BGE     @@randloop

        MOVR    R0,     R2
        MVII    #EDGEL+1, R1
        CALL    GETPIXELS
        CMPI    #7,     R0
        BEQ     @@nukeit
@@fillit:

        CALL    NUKELINE
        B       @@wellloop

@@nukeit:
        MVO     R2,     @@bot

        CALL    WAIT            ; Wait a few ticks
        DECLE   2               ; 

        MVII    #@@wellloop, R5
        PSHR    R5
        B       DOSCORE.nukeit  ; Nuke the line, collapsing the well.
                                ; This will return to @@wellloop

@@return:
        PULR    PC              ; Return.
        ENDP

;;==========================================================================;;
;;  SCOREDROP                                                               ;;
;;  Loops over a four line window and marks rows for deletion.              ;;
;;  Returns line-clearing score in R0 and leaves lines to be cleared set    ;;
;;  to color #7.                                                            ;;
;;                                                                          ;;
;;  NUKELINE                                                                ;;
;;  Force-clears a line as if it were full.                                 ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R2 == Row number to start at.                                           ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 = Number of lines cleared.                                           ;;
;;  R2 = original value plus 4                                              ;;
;;  R3 = Number of lines cleared.                                           ;;
;;  R1, R4, R5 trashed                                                      ;;
;;                                                                          ;;
;;  $1E0..$1E3 used as temp storage.                                        ;;
;;==========================================================================;;
SCOREDROP       PROC
@@trip  EQU     $1E2
@@full  EQU     $1E3

        PSHR    R5                      ; Save return address
        CLRR    R3                      ; Clear count of lines to score
        MVO     R3,     @@full
        MVII    #3,     R4              ; Loop trip count

        B       @@doscore
NUKELINE:
        PSHR    R5
        CLRR    R4
        MVO     R4,     @@full
        MVO     R4,     @@trip
        B       @@nuke

@@doscore:
        MVO     R4,     @@trip

        MVII    #EDGEL, R1              ; Iterate over columns 15..24
@@scoreloop:
        CALL    GETPIXELS               ; Read a pixel
        TSTR    R0                      ; Is it zero?
        BEQ     @@sloop                 ; If so, stop -- line's not full.

        INCR    R1
        CMPI    #EDGER, R1              ; At the end of the well?
        BLE     @@scoreloop             ; If not, keep looping

@@nuke:
        MVI     @@full, R3
        INCR    R3
        MVO     R3,     @@full

        ;; Prepare to set entire row in well to color 7
        MVII    #EDGEL, R1              ; Start at left edge of well
        MVII    #7,     R0              ; Color = 7
@@floop:
        CALL    PUTPIXELS               ; Draw the pixel
        INCR    R1                      ; Move to next column
        CMPI    #EDGER, R1              ; Are we at the right edge?
        BLE     @@floop                 ; No: Keep looping.

@@sloop:
        MVI     @@trip, R4
        INCR    R2                      ; Go to next line
        DECR    R4                      ; Decrement loop count
        BPL     @@doscore               ; Keep going as long as R4 >= 0

@@sdone:                                ; Scoring complete
        MVI     @@full, R3

        ;; If this was a 4-line clear, play the special sound effect
        CMPI    #4,     R3
        BNEQ    @@return
        MVII    #2,     R1              ; High priority sound effect
        CALL    PLAY.sfxp
        DECLE   FXBOMB4
@@return:
        MOVR    R3,     R0              ; Return number of cleared lines.
        PULR    PC                      ; Return to caller.

        ENDP

;;==========================================================================;;
;;  DOSCORE                                                                 ;;
;;  Calls SCOREDROP to score a dropped piece and mark lines for deletion.   ;;
;;                                                                          ;;
;;  Calculates the score based on number of downward moves the player       ;;
;;  made, as well as the number of lines cleared.                           ;;
;;                                                                          ;;
;;  Collapses lines that have been marked for deletion by the scoring       ;;
;;  routines.  Processes the four lines ABOVE R2.  (This works pretty well, ;;
;;  since the scoring routines leave R2 pointing to the line AFTER the four ;;
;;  lines that were scored.)  Lines are scanned in the leftmost column,     ;;
;;  and lines containing a pixel of color 7 in this column are cleared.     ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R2 == Line at top of 4-line window to score                             ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0, R1, R2, R4, R5 trashed?                                             ;;
;;                                                                          ;;
;;  Locations $1E0..$1E3, $1E6 used as temporary storage.                   ;;
;;==========================================================================;;
SCORETABLE      DECLE   500, 1500, 3000, 6000
DOSCORE PROC
@@bot   EQU     $1E2
@@top   EQU     $1E3
@@clr   EQU     $1E6

        PSHR    R5


        MVII    #EDGEB-4, R3    ; Make sure we're not too close to
        CMPR    R3,     R2      ;  the bottom of the well.
        BLE     @@rowok
        MOVR    R3,     R2      ; If we are, move our window up some.
@@rowok:
        PSHR    R2

        ; First, add in any "move points"
        MVI     MSCORE, R0


        MVI     PSCORL, R1      ; Add this to our score
        MVI     PSCORH, R4
        ADDR    R0,     R1      ; Add moves to lower half of 32-bit score
        ADCR    R4              ; Propogate carry from lower to upper half
        MVO     R1,     PSCORL
        MVO     R4,     PSCORH

        CLRR    R2
        MVO     R2,     MSCORE  ; Clear the total.

        ; Wait one tick to make sure any queued pixels have been drawn.
        ; This works because the ISR always drains the committed pixel
        ; queue completely, and WAIT busywaits on a counter maintained
        ; by the ISR.  Cool, eh?
        CALL    WAIT
        DECLE   1

        PULR    R2
        ; Next, see if we need to clear any lines due to this drop
        CALL    SCOREDROP       ; Score the drop.

        MOVR    R0,     R5      ; If we didn't score anything, return
        BEQ     @@done          ; (chain return through PICKPIECE)
@@hmm
        ADDI    #SCORETABLE-1, R5 ; R5 = &scoretable[lines]

        MVI@    R5,     R0      ; Load base score for this # of lines

        MVI     LEVEL,  R3

        MVI     PSCORL, R1      ; Otherwise, add this to our score
        MVI     PSCORH, R4
@@scoreloop:
        ADDR    R0,     R1      ; (multiply by level thru repeated addition)
        ADCR    R4

        DECR    R3
        BPL     @@scoreloop     ; Go around 'level + 1' times.

        MVO     R1,     PSCORL
        MVO     R4,     PSCORH

@@notnuke:
        SUBI    #4,     R2      ; Find top of window
        MVO     R2,     @@top   ; Remember top row
        ADDI    #3,     R2      ; Move to bottom of window

        CALL    SPAWN           ; Fork the line clearing as new task.
        DECLE   13, 3           ; Period == 6 ticks (10Hz), task #3

    ;;--------------------------------------------------------------------;;
    ;;  NOTE:  THIS IS A NEW TASK HERE!!!  IT IS CALLED FOR EACH LINE!!!  ;;
    ;;  We do this to avoid having tasks pile up in the queue while we    ;;
    ;;  do our work.  Things such as score updates (most common) tend to  ;;
    ;;  pile up in the queue otherwise.                                   ;;
    ;;--------------------------------------------------------------------;;

@@scan:
        MVII    #EDGEL, R1      ; Scan along left edge
        MVO     R2,     @@bot   ; Remember current y position
        CALL    GETPIXEL        ; Grab a pixel.
        MVI     @@bot,  R2      ; Restore row position
        MVI     @@top,  R1      ; Restore top of window
        CMPI    #7,     R0      ; Is this pixel color 7?
        BEQ     @@collapseok    ; It is: go collapse this row.

        DECR    R2              ; Move up a row on the screen
        CMPR    R1,     R2      ; Are we out of our window?
        BGE     @@scan          ; If not, keep scanning

@@done:
        CALL    SLEEP
        DECLE   31,     3       ; Sleep for 1/4th second after drop.
        PULR    R5
        B       PICKPIECE       ; Chain return through PICKPIECE

@@collapseok:

        ;; When we collapse a line, the top of our scanning window
        ;; actually moves down a row, and our scan position does NOT move
        ;; up a row.  So, look at the scan row, and increment the top
        ;; row value.
        INCR    R1              ; Move top down one row
        MVO     R1,     @@top   ; Store new top of window

        ;; Increment our "cleared lines" count and queue up the "show lines"
        ;; task.
        MVI     LINES,  R1
        INCR    R1
        MVO     R1,     LINES

        MVII    #UPDATESTATS, R0
        JSRD    R5,     QTASK

        ;; We spawned as a one-shot task.  We retrigger only after we're
        ;; sure we might need it, so that we don't get over-triggered.
        ;; (We will get triggered up to 5 times, with the fifth trigger
        ;; doing the call to PICKPIECE.)
        MVII    #3,     R3
        CALL    RETRIGGERTASK

@@nukeit:
        MVII    #1,     R1
        CALL    PLAY.sfxp       ; Trigger the collapse soundeffect.
        DECLE   FXBOMB

@@nosfx:
        MVII    #EDGEL, R1      ; Start at left edge.

@@xloop:
        MVI     HEIGHT, R2      ; Start at top of screen
        MVO     R1,     XSAVE   ; Save our current column
        CLRR    R4              ; Propogate a black pixel from top

@@yloop:
        MVO     R4,     @@clr   ; Remember prev row's color
        MVO     R2,     YSAVE   ; Save our current row
        CALL    PIXCALC         ; Calculate mask/addr for this pixel
        MOVR    R1,     R4      ; Remember mask
        CALL    GETPIXELR       ; Get color to copy to next row
        MOVR    R4,     R1      ; Restore mask
        MOVR    R0,     R4      ; Remember color for next row
        MVI     @@clr,  R0      ; Restore previous rows color
        CALL    PUTPIXELR       ; Draw prev rows color in this row
        MVI     YSAVE,  R2      ; Restore row position
        MVI     XSAVE,  R1      ; Restore column position
        INCR    R2              ; Move down one row
        CMP     @@bot,  R2      ; Are we at the last row?
        BLE     @@yloop         ; No?  Keep looping.

        INCR    R1              ; Move to next column
        CMPI    #EDGER, R1      ; Are we at the right edge yet?
        BLE     @@xloop         ; No?  Keep looping.

        ; Decrement one from our well height here, so that music speed
        ; changes after we collapse a row, and our scan window changes as
        ; well for speed.
        MVI     HEIGHT, R1      ; Decrement the well height now.
        INCR    R1              ; (Note:  'HEIGHT' is actually
        MVO     R1,     HEIGHT  ; EDGEB - height.  Hence the INCR.)

        PULR    PC              ; Return... we will be retriggered
                                ; if there are more lines to scan.
        ENDP


;;==========================================================================;;
;;  UPDATESCORE                                                             ;;
;;  Shows current score, by incrementally updating a displayed score value. ;;
;;==========================================================================;;
UPDATESCORE     PROC
        MVII    #PSCORL,R4      ; Point to player/displayed score
        MVI@    R4,     R2      ; Get lo 16 of player's actual score
        MVI@    R4,     R3      ; Get hi 16 of player's actual score
        MVI@    R4,     R0      ; Get lo 16 of displayed score
        MVI@    R4,     R1      ; Get hi 16 of displayed score
        CMPR    R3,     R1      ; Compare upper halves.
        BEQ     @@next          ; Displayed = Actual:  Test lower halves
        BNC     @@doincr        ; Displayed < Actual:  Do the increment
        BNEQ    @@uhoh          ; Displayed > Actual?  UHOH!
@@next:
        CMPR    R2,     R0      ; Compare lower halves.
        BEQ     @@return        ; Displayed = Actual:  Do nothing.
        BNC     @@doincr        ; Displayed < Actual:  Do the increment
                                ; Displayed > Actual?  UHOH!
@@uhoh:
        ; If we get here, we accidentally overshot the player's actual
        ; score somehow.  Compensate by setting the displayed score to the
        ; actual score.  :-P  (Note:  We also use this code to force a
        ; redisplay of the score on purpose by setting the displayed
        ; score to 0xFFFFFFFF, which is always higher than the player's
        ; score (or is it?  How good are YOU?)).
        MOVR    R2,     R0      ; Set lo half of displayed to actual
        MOVR    R3,     R1      ; Set hi half of displayed to actual
        B       @@disp          ; Display the fixed score.

@@return:
        JR      R5

@@doincr:
        ; First, find the difference between the displayed and actual
        ; scores.  This is a 32-bit subtract.
        SUBR    R0,     R2      ; Subtract the lo halves.
        ADCR    R3              ; Subtract the borrow from the hi half
        DECR    R3              ;    as a "not borrow"
        SUBR    R1,     R3      ; Subtract the hi halves

        ; Divide the difference by 8, rounding upwards.
        ; This is effectively "num = (num + 7) / 8".  It should
        ; produce a non-zero number if num starts out non-zero.
        ADDI    #7,     R2      ; Add rounding factor
        ADCR    R3              ; ... and carry into upper half

        ; Unsigned right shift by 3
        CLRC
        RRC     R3,     1       ; Shift the first bit.  Can't SARC (sign bit)
        RRC     R2,     1       ;
        SARC    R3,     2       ; SARC is safe now: sign bit is for sure 0
        RRC     R2,     2       ;

;       TSTR    R2
;       BNEQ    @@addit
;       TSTR    R3
;       BNEQ    @@addit
;       HLT                     ; THIS SHOULD NEVER HAPPEN
;       INCR    R2
@@addit:
        ADDR    R2,     R0      ; Add the new increment's lo half
        ADCR    R1              ; Propogate the carry
        ADDR    R3,     R1      ; Add the new increment's hi half
@@disp:
        MVO     R0,     DSCORL  ; Store the updated display value (lo)
        MVO     R1,     DSCORH  ; Store the updated display value (hi)

        MVI     SCRDIG, R2
        MVI     SCRCOL, R3
        MVI     SCRPOS, R4
        B       DEC32B          ; Display with leading zeros.
                                ; (Chained return)

        ENDP

;;==========================================================================;;
;;  UPDATESTATS                                                             ;;
;;  Updates game information such as # of lines, level, and drop speed.     ;;
;;==========================================================================;;
UPDATESTATS    PROC
        PSHR    R5
        MVI     SCRCOL, R3      ; Get score color

        MVI     LINES,  R0      ; Get current number of lines

        MVI     LEVEL,  R1      ; Get current level
        TSTR    R1
        BEQ     @@initial       ; First time through, force an update.

        ADDR    R1,     R1      ; R1 = 2 * (level)
        MOVR    R1,     R2
        SLL     R2,     2       ; R2 = 8 * (level)
        ADDR    R2,     R1      ; R1 = 10 * (level)

        CMPR    R1,     R0      ; See if we've cleared enough lines to go up
                                ; a level.
        BLT     @@samelevel     ; If not new level, just update line counter.

        MVII    #3,     R1      ; Sound effect priority == 3
        CALL    PLAY.sfxp       ;
        DECLE   FXDING3         ; Ding to signify "Next Level".
       ;B       @@nextlevel

@@nextlevel:
        MVI     LEVEL,  R2      ; Get current level number
        INCR    R2              ; Update our level number
        MVO     R2,     LEVEL   ; Store the new level number

        B       @@skipinitial

@@initial:
        MVI     SLEVEL, R2      ; Get starting level
        MVO     R2,     LEVEL   ; Store it to our level number

@@skipinitial:

        MVII    #20,    R4      ; See if level > 20.  If so, clamp to that
        CMPR    R4,     R2      ; for purposes of setting speed.
        BLE     @@speedok
        MOVR    R4,     R2
@@speedok:
        ADDI    #SPDTBL+1, R2   ; Point R2 into Speed Table
        MVI@    R2,     R2
        MVO     R2,     SPEED   ; Set piece speed from speed table

        MVII    #LVLLOC + 20, R4
        MVII    #2,     R2
        MVI     LEVEL,  R0      ; Increment the level number
        CALL    DEC16A          ; Display the updated level number

        MVI     LINES,  R0
@@samelevel:
        MVII    #LINLOC + 20, R4
        MVII    #2,     R2
        PULR    R5
        B       DEC16A
        ENDP

;;==========================================================================;;
;;  SPDTBL                                                                  ;;
;;  Speed table.  Contains rates at which pieces drop for all the levels.   ;;
;;  Rates are all in "Ticks * 2".                                           ;;
;;==========================================================================;;
SPDTBL  PROC
        DECLE   160 + 0         ; Level  1
        DECLE   120 + 0         ; Level  2
        DECLE   100 + 0         ; Level  3
        DECLE    80 + 0         ; Level  4
        DECLE    60 + 0         ; Level  5
        DECLE    50 + 0         ; Level  6
        DECLE    40 + 0         ; Level  7
        DECLE    34 + 0         ; Level  8
        DECLE    28 + 0         ; Level  9
        DECLE    24 + 0         ; Level 10
        DECLE    20 + 0         ; Level 11
        DECLE    18 + 0         ; Level 12
        DECLE    16 + 0         ; Level 13
        DECLE    14 + 0         ; Level 14
        DECLE    12 + 0         ; Level 15
        DECLE    10 + 0         ; Level 16
        DECLE     8 + 0         ; Level 17
        DECLE     6 + 0         ; Level 18
        DECLE     4 + 0         ; Level 19
        DECLE     2 + 0         ; Level 20 and beyond
        ENDP

;;==========================================================================;;
;;  MARQUEE                                                                 ;;
;;  Cycles colors on a string of text.                                      ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R2 -- Task instance data:                                               ;;
;;        Low byte of instance data gives screen offset.                    ;;
;;        High byte of instance data gives length.                          ;;
;;==========================================================================;;
MARQUEE PROC

        MOVR    R2,     R3
        ANDI    #$FF,   R3      ; R3 == screen offset.
        XORR    R3,     R2
        SWAP    R2              ; R2 == length.
        ADDI    #$200,  R3      ; Convert screen offset into actual address.

        MVI@    R3,     R0      ; Get first display word from string.
        MVII    #$7,    R1      ; Color mask for three LSBs.
        ANDR    R1,     R0      ; Get color from leading character in string.
        COMR    R1              ; Invert our mask, so we can replace colors.
@@loop:
        DECR    R0              ; Cycle color by decrementing
        BNEQ    @@ok            ; Make sure it didn't go to zero!
        MVII    #$7,    R0      ; If it did, set it back to 7.
@@ok:
        MVI@    R3,     R4      ; Get a character from the display.
        ANDR    R1,     R4      ; Mask away its color bits.
        ADDR    R0,     R4      ; Replace them with our new color bits.
        MVO@    R4,     R3      ; Write the modified character to the display.
        INCR    R3              ; Move to next character.
        DECR    R2              ; Loop until done.
        BNEQ    @@loop

        JR      R5              ; Return.
        ENDP

;;==========================================================================;;
;;  ANIDROP                                                                 ;;
;;  Color cycles CS1 and CS3 so that collapse lines "flash", or sets a      ;;
;;  solid color while piece is active.                                      ;;
;;==========================================================================;;
ANIDROP PROC
        MVI     CURPC.C, R2     ; Is there a current piece active?
        TSTR    R2
        BNEQ    @@active
        MVI     CS1,    R2      ; No? Just flash colors then.
        INCR    R2
@@active:
        MVO     R2,     CS1     ; Set up colorstack #1 and #3
        MVO     R2,     CS3
        JR      R5              ; Return.
        ENDP

;;==========================================================================;;
;;  MUTETOGGLE                                                              ;;
;;  Toggles MUTE state                                                      ;;
;;==========================================================================;;
MUTETOGGLE PROC
        DIS
        MVI     MUTE,   R0      ; Get current mute status
        TSTR    R0              ; If zero, we're muted.
        BEQ     @@unmute
        ; Mute the music by zeroing its volume registers.
        CLRR    R0
        MVI     SNDSTRM+3,R1    ; Get current stream mask
        SLL     R1,     2       ; Move volume-register bits to top

        SLLC    R1              ; If channel C's vol control is enabled ....
        BNC     @@notc
        MVO     R0,     PSG0+13 ; ... clear it
@@notc:
        SLLC    R1              ; If channel B's vol control is enabled ....
        BNC     @@notb
        MVO     R0,     PSG0+12 ; ... clear it

@@notb:
        SLLC    R1              ; If channel A's vol control is enabled ....
        BNC     @@nota
        MVO     R0,     PSG0+11 ; ... clear it

@@nota:
        INCR    PC              ; skip INCR R0.
@@unmute:
        INCR    R0              ; Un-mute the audio

        MVO     R0,     MUTE    ; Store the new mute status
        EIS
        JR      R5
        ENDP


;;==========================================================================;;
;;  ROTPIECE_CW                                                             ;;
;;  Rotates a piece clockwise by updating the two LSBs of its piece         ;;
;;  index #.                                                                ;;
;;                                                                          ;;
;;  ROTPIECE_CCW                                                            ;;
;;  Rotates a piece counter clockwise by updating the two LSBs of its       ;;
;;  piece index #.                                                          ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R3 -- Piece number/rotation                                             ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS                                                                 ;;
;;  R3 -- New piece/rotation                                                ;;
;;  R0 -- Trashed                                                           ;;
;;==========================================================================;;
ROTPIECE_CW:    PROC
        SETC                    ; Flag that this is a clockwise rotate
        INCR    PC              ; (skip the CLRC)
ROTPIECE_CCW:
        CLRC                    ; Flag that this is a counter-clockwise rotate
        MOVR    R3,     R0      ; Copy piece number/rotation to R0
        ADCR    PC              ; If counter-clockwise, complement R0,
        COMR    R0              ; ... else don't.
        SETC
        RLC     R0,     1
        ANDI    #3,     R0
        XORR    R0,     R3      ; Magic!
        JR      R5
        ENDP

;;==========================================================================;;
;; GRAVITY                                                                  ;;
;; Makes the pieces fall!                                                   ;;
;;                                                                          ;;
;; MVPIECE                                                                  ;;
;; Moves/Rotates a piece around on the screen.                              ;;
;;                                                                          ;;
;; INPUTS:                                                                  ;;
;; R2 -- Event number:                                                      ;;
;;       13: Disc Left       Move piece left                                ;;
;;       14: Disc Right      Move piece right                               ;;
;;       15: Disc Down       Move piece down                                ;;
;;       16: Action Top      Rotate CCW                                     ;;
;;       17: Action Left     Rotate CW                                      ;;
;;       18: Action Right    Rotate CW                                      ;;
;;                                                                          ;;
;; THIS CODE SUCKS.                                                         ;;
;; (Although, it sucks less than the original code that I had here.)        ;;
;;==========================================================================;;
GRAVITY PROC
        MVII    #15,    R2      ; Event #15 is 'down'
        CLRR    R0              ; Flag that this is NOT a hand-ctrl input
        INCR    PC              ; (skip 'MOVR')
MVPIECE:
        MOVR    R2,     R0      ; Flag that this IS a hand-ctrl input
        MVO     R0,     WASHAND ; Store 'WASHAND' flag.

        PSHR    R5              ; Save return address
        PSHR    R2              ; Save keypad #

        MVII    #CURPC.C, R4
        CALL    GETPIECEREC     ; Load current piece's record into R0..R3

        PULR    R4              ; Get keypad # into R4

        TSTR    R0              ; Is there an active piece?
        BEQ     @@return        ; Do nothing if no piece is active.

        MVII    #@@check, R5    ; Set up "return address"

        MVO     R4,     WASDOWN ; Default "WAS DOWN" to "NOT"

        SUBI    #13,    R4      ; Scale range down to 0..6, check for R4==13
        BMI     @@return        ; Bogus input R4 < 13 (should never happen)
        BNEQ    @@not_left      ; If taken, R4 != 13

        DECR    R1              ; Move left
        JR      R5              ; Jump to '@@check'

@@not_left:
        SUBI    #2,     R4      ; Check for R4==14 or R4==15
        BMI     @@is_right      ; Bogus input R4 == 13 (should never happen)
        BNEQ    @@not_down      ; If taken, R4 != 14

        MVO     R4,     WASDOWN ; Set 'WASDOWN' to 'Yes, it was DOWN'.
        INCR    R2              ; Move down one
        JR      R5              ; Jump to '@@check'

@@is_right:
        INCR    R1              ; Move right
        JR      R5              ; Jump to '@@check'

@@not_down:
        DECR    R4              ; Check for R4==16 or R4 > 16
        ; Note: Both of these branches chain return through ROTPIECE_xxx
        BEQ     ROTPIECE_CCW    ; If taken, R4==16 (top action button)
        B       ROTPIECE_CW     ; If taken, R4>=17 (bottom action buttons)

@@check:
        ;; All movement paths lead to here.  At this point, R0..R3
        ;; contain the proposed piece state (rotation, X, Y)

        CALL    TESTPIECE       ; Is the piece placeable here?
        TSTR    R0
        BNEQ    @@cantmove      ; If we couldn't move the piece, leave.


        ; Prepare to move the piece.
        CALL    GETPIECECOLOR   ; Get the piece's color.
        MVO     R0,     CURPC.C ; Set piece's color in its record.
        MVO     R0,     CS1     ; Force color-stack to piece's color
        MVO     R0,     CS3     ; Force color-stack to piece's color

        MVII    #TMPPC.X, R4
        CALL    PUTPIECEREC.NC  ; Save updated piece in temp storage
        CALL    GETPIECEREC     ; Restore original piece info
        CLRR    R0              ; Force color to 0 for erase.
        CALL    DRAWPIECE       ; Erase previous piece

        MVII    #TMPPC.X, R4
        CALL    GETPIECEREC.NC  ; Restore updated piece info
        INCR    R4
        MVII    #7,     R0      ; Always draw in color 7 here!
        CALL    PUTPIECEREC.NC  ; Store it to the current piece info

        CALL    DRAWPIECE       ; Draw the updated piece on the screen

        ; Bip and increment the per-move total
        MVI     WASHAND, R0     ; Was this a hand-controller input?
        TSTR    R0
        BEQ     @@nothand       ; No... don't bip, and don't add to MSCORE

        CALL    PLAY.sfx        ; Yes... BIP!
        DECLE   FXBIP

        MVI     WASDOWN, R0     ; If this was a hand movement that pushed
        TSTR    R0              ; down, increment the MSCORE.
        BNEQ    @@notdown       ; No... don't increment MSCORE

        MVI     MSCORE, R0
        ADDI    #MOVEINC,R0     ; Yes... increment MSCORE!
        MVO     R0,     MSCORE
@@notdown:
@@nothand:

        PULR    R5              ; Chain Return via COMMITPXQ
        B       COMMITPXQ       ; Commit the pixels to be drawn to screen.

@@cantmove:
        MVI     WASDOWN, R0     ; Was this a downward move?
        TSTR    R0
        BNEQ    @@return        ; No?  Just return then.
        PULR    R5
        B       PLACEPIECE      ; Yes?  Place the piece (w/ chained return)

@@return:
        PULR    PC              ; Return
        ENDP

;;==========================================================================;;
;;  NEXTSHOW                                                                ;;
;;  Shows the currently displayed piece in the next-piece box.              ;;
;;                                                                          ;;
;;  NEXTCLEAR                                                               ;;
;;  Clears the currently displayed piece in the next-piece box.             ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  Location 'NXTPC' holds current next piece.                              ;;
;;  R5 -- return address.                                                   ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- saved                                                             ;;
;;  R1 -- NXTPCX                                                            ;;
;;  R2 -- NXTPCY                                                            ;;
;;  R3 -- Next piece's piece number/rotation                                ;;
;;  R4, R5 -- trashed                                                       ;;
;;==========================================================================;;
NEXTSHOW PROC
        MVO     PC,     DIDNXT  ; Set the "We showed next piece" flag.
                                ; (Note: PC guaranteed to be non-zero)
        SETC                    ; Set the "This is NEXTSHOW" flag.
        INCR    PC              ; Skip the CLRC.
NEXTCLEAR:
        CLRC                    ; Set the "This is NEXTCLEAR" flag.
        PSHR    R5              ; Save our return address.
        PSHR    R0              ; Save R0.
        MVI     NXTPC,  R3      ; Get the next piece's piece number/rotation
        BNC     @@clear         ; If carry cleared, set color to 0.
        CALL    GETPIECECOLOR   ; Otherwise, get the piece's color.
        INCR    PC              ; Skip the CLRR.
@@clear:
        CLRR    R0              ; Clear the piece's color.
        MVII    #NXTPCX, R1     ; Position ourself in the next-piece box.
        MVII    #NXTPCY, R2
        CALL    DRAWPIECE

        PULR    R0              ; Restore R0.
        PULR    R5              ; Chain return via COMMITPXQ
        B       COMMITPXQ       ; Commit the pixels for display immediately.
        ENDP


;;==========================================================================;;
;;  NEXTTOGGLE                                                              ;;
;;  Toggles whether next-piece is shown.                                    ;;
;;==========================================================================;;
NEXTTOGGLE      PROC
        MVI     SHOWNXT, R1     ; Are we currently showing next piece?
        TSTR    R1
        BEQ     @@turnon        ; No... then start showing it.
@@turnoff:
        CLRR    R1              ; Yes... turn it off!
        MVO     R1,     SHOWNXT
        B       NEXTCLEAR       ; Chain return through NEXTCLEAR
@@turnon:
        INCR    R1
        MVO     R1,     SHOWNXT
        B       NEXTSHOW        ; Chain return through NEXTSET
        ENDP


;;==========================================================================;;
;;  PLACEPIECE                                                              ;;
;;  Make piece permanent where it is at.                                    ;;
;;==========================================================================;;
PLACEPIECE      PROC
        DIS
        PSHR    R5
        MVII    #CURPC.C, R4    ; Write to "current piece" record.
        CALL    GETPIECEREC     ; Make this the current piece.
        EIS

        TSTR    R0              ; Make sure we actually had an active piece
        BNEQ    @@active
        PULR    PC              ; Not active?  Just return!
@@active:

        CALL    DRAWPIECE       ; Draw it.
        CALL    COMMITPXQ       ; Commit the pixels.
        CALL    WAIT            ; Wait one tick, so pixels are committed.
        DECLE   1

        CLRR    R5              ; Mark our current piece as inactive.
        MVO     R5,     CURPC.C ; It will get reactivated by DOSCORE.
                                ; We do this after the 'wait' so that we're
                                ; sure our queued pixels are drawn.

        CMPI    #EDGEB, R4      ; Is this piece off-screen?
        BLE     @@notgameover   ; No... keep going
        CALL    NEXTCLEAR
        PULR    R5
        CLRR    R0
        MVO     R0,     HEIGHT
        B       SCHEDEXIT       ; Yes... game over!

@@notgameover:
        CMP     HEIGHT, R4      ; Did this piece form the new top-of-well?
        BGE     @@nottop
        MVO     R4,     HEIGHT

@@nottop:
        MVI     DIDNXT, R0      ; Was 'next piece' displayed during this
        TSTR    R0              ; piece?  If not, double MSCORE
        BNEQ    @@didnext
        MVI     MSCORE, R0
        ADDR    R0,     R0      ; double the MSCORE
        MVO     R0,     MSCORE
@@didnext:

        PSHR    R4
        CALL    PLAY.sfx
        DECLE   FXBYUMP

        PULR    R1              ; Make row # the task-instance data
        MVII    #DOSCORE, R0    ; Queue the scoring task.
        PULR    R5
        JD      QTASK           ; Queue and chain return
        ENDP


;;==========================================================================;;
;;  TITLESCREEN                                                             ;;
;;  Draws the title screen and waits for user input.                        ;;
;;==========================================================================;;
TITLESCREEN     PROC
        PSHR    R5

        CLRR    R0
        MVO     R0,     CS0             ; Set color stack to BLACK
        MVO     R0,     CS1
        MVO     R0,     CS2
        MVO     R0,     CS3
        MVO     R0,     CB              ; Set display border to BLACK

        MVII    #$F0,   R1              ; Set entire display to BLACK
        MVII    #$200,  R4
        CALL    FILLMEM

        CALL    DRAWSTRING3
        DECLE   6, $200+0*20+3
        BYTE    "Joseph Zbiciak",0

        CALL    DRAWSTRING4
        DECLE   $200+1*20+6
        BYTE    "presents",0

        CALL    DRAWSTRING5
        DECLE   TITLE + 1               ; Point to the title
        DECLE   $807                    ; In GRAM font.
        DECLE   $200+5*20+10-3          ; Point R4 to location on screen

        CALL    DRAWSTRING3
        DECLE   1, $200+10*20+2
        DECLE   "Copyright ", COPYR, " 2000", 0
        MVO     R1,     SCRCOL

        ; Animate the '2000' in Copyright 2000.
        SUBI    #4,     R4              ; Back up to the '2000'
        MVO     R4,     SCRPOS
        MVII    #6,     R4              ; Suppress 6 digits
        MVO     R4,     SCRDIG
        MVII    #2000,  R0
        CLRR    R1
        MVII    #PSCORL,R4
        MVO@    R0,     R4              ; PSCORL = 2000
        MVO@    R1,     R4              ; PSCORH = 0 --> PSCOR = 2000
        SUBI    #25,    R0
        MVO@    R0,     R4              ; DSCORL = 1975
        MVO@    R1,     R4              ; DSCORH = 0 --> DSCOR = 1975

        CALL    STARTTASK
        DECLE   2                       ; Task # 2
        DECLE   UPDATESCORE             ; Update displayed date
        DECLE   4,      4               ; 15Hz

        ; Start a Marquee task on the 4-TRIS title string.
        MVII    #107 + 256*6, R2        ; Length 6, Offset 107
        CALL    STARTTASK
        DECLE   0                       ; Task #0
        DECLE   MARQUEE                 ; MARQUEE task
        DECLE   10,     10              ; Period: every 5 ticks

        ; Make all three tasks active.
        MVII    #3,     R3
        MVO     R3,     TSKACT

        ; Start off title screen music.
        CALL    PLAY.mus
        DECLE   M_TITLE

        ; Set our keypad dispatch
        CLRR    R0
        MVO     R0,     HANDFN

        PULR    PC
        ENDP

;;==========================================================================;;
;;  GAMEHAND                                                                ;;
;;  Table of keypad dispatches for INGAME                                   ;;
;;==========================================================================;;
GAMEHAND        PROC
@@kp0:  DECLE   STUB            ;  0: KP0
@@kp1:  DECLE   STUB            ;  1: KP1
@@kp2:  DECLE   STUB            ;  2: KP2
@@kp3:  DECLE   STUB            ;  3: KP3
@@kp4:  DECLE   NEXTTOGGLE      ;  4: KP4
@@kp5:  DECLE   STUB            ;  5: KP5
@@kp6:  DECLE   NEXTTOGGLE      ;  6: KP6
@@kp7:  DECLE   MUTETOGGLE      ;  7: KP7
@@kp8:  DECLE   STUB            ;  8: KP8
@@kp9:  DECLE   MUTETOGGLE      ;  9: KP9
@@kpC:  DECLE   NEXTTOGGLE      ; 10: KPC
@@kpE:  DECLE   STUB            ; 11: KPE
@@dsU:  DECLE   STUB            ; 12: Disc Up
@@dsL:  DECLE   MVPIECE         ; 13: Disc Left
@@dsR:  DECLE   MVPIECE         ; 14: Disc Right
@@dsD:  DECLE   MVPIECE         ; 15: Disc Down
@@acT:  DECLE   MVPIECE         ; 16: Action Top
@@acL:  DECLE   MVPIECE         ; 17: Action Left
@@acR:  DECLE   MVPIECE         ; 18: Action Right
        ENDP

;;==========================================================================;;
;;  INGAME                                                                  ;;
;;  Sets up the display for the game by clearing the screen to "colored     ;;
;;  squares" mode, etc.  Also sets up some initial tasks for starting the   ;;
;;  game.                                                                   ;;
;;==========================================================================;;
INGAME: PROC
        PSHR    R5
        ; Clear screen to colored squares mode
        MVII    #$10,   R0
        SWAP    R0              ; $1000 corresponds to four black sq
        MVII    #$200,  R4      ; Fill memory starting at $200
        MVII    #$F0,   R1      ; ... through $2EF (all BACKTAB)
        CALL    FILLMEM

        ; Set up color stack advance bits to left and right of the "well".
        ; Also, draw well edges in yellow.
        MVII    #$206, R4
        MVII    #$22F8, R0      ; Black w/ "advance" bit, no colsq
        MVII    #11,    R1      ; 11 rows
        MVII    #$1186, R2      ; Yellow in blocks 0, 2
        MVII    #$3430, R3      ; Yellow in blocks 1, 3
@@csloop:
        MVO@    R0,     R4
        MVO@    R2,     R4
        ADDI    #4,     R4
        MVO@    R3,     R4
        MVO@    R0,     R4
        ADDI    #12,    R4
        DECR    R1
        BNEQ    @@csloop

        ; Draw bottom of the "well"
        MVII    #EDGEL, R1
        MVII    #EDGEB, R2
        MVII    #6,     R0
@@botloop:
        CALL    PUTPIXELS
        INCR    R1
        CMPI    #EDGER, R1
        BLE     @@botloop


        ; Draw the 'next piece' box
        MVII    #NXTPCX-3, R1
        MVII    #NXTPCY-3, R2
        MVII    #6,     R0
@@npxloop:
        CALL    PUTPIXELS
        ADDI    #7,     R2
        CALL    PUTPIXELS
        SUBI    #7,     R2
        INCR    R1
        CMPI    #NXTPCX+3, R1
        BLE     @@npxloop

        INCR    R2
        DECR    R1
@@npyloop:
        CALL    PUTPIXELS
        SUBI    #6,     R1
        CALL    PUTPIXELS
        ADDI    #6,     R1
        INCR    R2
        CMPI    #NXTPCY+4, R2
        BLE     @@npyloop

        ; draw 'Lines', 'Level' and 'Next' onscreen in cyan
        MVII    #$1809, R1
        MVII    #LVLLOC, R4
        MVII    #8,     R2
        CALL    TLA             ; Draw 'Level'
        ADDI    #LINLOC - LVLLOC - 3, R4
        CALL    TLA             ; Draw 'Lines'
        SUBI    #3 + LINLOC - NXTLOC, R4
        CALL    TLA             ; Draw 'Next'

        ; Spool up the random number generator a bit
        MVII    #$FF,   R2
        CALL    NEXTRANDX

        ; Pick a starting piece and a starting 'next piece'
        CLRR    R0
        MVO     R0,     NXTPC
        CALL    PICKPIECE       ; Pick a random piece (starting piece)
        MVII    #$FF,   R2
        CALL    NEXTRANDX       ; Spin the random number generator some more.
        CALL    PICKPIECE       ; Pick a random piece (next piece)

        ; Set PREVBL routine to animate CS1 and CS3
        MVII    #ANIDROP, R0
        MVO     R0,     PREVBL

        ; Task 0:  Step blocks down the screen / score drops.
        ; Task 1:  Key/disc/pad repeat manager.
        ; Task 2:  Score display update.
        ; Task 3:  Sleep task.

        ; Set up piece drop task
        CALL    STARTTASK
        DECLE   0
        DECLE   GRAVITY
        DECLE   120,    120     ; 1Hz

        ; Set up the score update.
        CLRR    R2              ; Do not suppress any digits.
        MVII    #$1802, R3      ; Display in orange, using GRAM font
        MVII    #20*11 + 5, R4  ; Put at bottom of screen in middle
        MVO     R2,     SCRDIG
        MVO     R3,     SCRCOL
        MVO     R4,     SCRPOS
        CALL    STARTTASK
        DECLE   2               ; Task # 2
        DECLE   UPDATESCORE     ; Update displayed score
        DECLE   4,      4       ; 30Hz


        ; Allow up to four active tasks.  (Task 3 is used for SLEEP).
        MVII    #4,     R3
        MVO     R3,     TSKACT

        ; Set up color stack.  Entries 0 and 2 are black, 1 and 3 are white.
        ; Entries 1 and 3 will be "cycled" during a drop animation sequence.
        MVII    #CS0,   R4
        CLRR    R0
        MVO@    R0,     R4
        MVO@    R0,     R4
        MVO@    R0,     R4
        MVO@    R0,     R4
        MVII    #PSCORL,R4      ; (interruptible, for STIC)

        ; Clear our total lines count and level number.  (UPDATESTATS
        ; will move the level number to 1 for us.)
        MVO     R0,     LINES   ; Lines cleared so far
        MVO     R0,     LEVEL   ; Clear level number to force an update


        ; Clear our starting score
        MVO@    R0,     R4      ; PSCORL
        MVO@    R0,     R4      ; PSCORH
        DECR    R0              ; Set to -1 to force a redisplay.
        MVO@    R0,     R4      ; DSCORL
        MVO@    R0,     R4      ; DSCORH

        ; Clear the well height.
        MVII    #EDGEB, R0
        MVO     R0,     HEIGHT

        ; Set our keypad dispatch
        MVII    #GAMEHAND, R0
        MVO     R0,     HANDFN

        ; Play some background music
        CALL    PLAY.mus
        DECLE   M_GAME

        ; Chain through "UPDATESTATS"
        PULR    R5
        B       UPDATESTATS
        ENDP

;;==========================================================================;;
;;  TLA                                                                     ;;
;;  'Three Letter Acronym'                                                  ;;
;;  Used to draw the labels over 'Level', 'Lines' and 'Next' on the screen. ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- Screen format word, including starting GRAM/GROM card #.          ;;
;;  R2 -- Index value for stepping between GRAM/GROM card #'s.              ;;
;;  R4 -- Display pointer.                                                  ;;
;;  R5 -- Return address.                                                   ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R1 -- Screen format word, with GRAM/GROM card # advanced by 3.          ;;
;;  R4 -- Display pointer, advanced by 3                                    ;;
;;  R2, R5 untouched.                                                       ;;
;;==========================================================================;;
TLA     PROC
        MVO@    R1,     R4
        ADDR    R2,     R1
        MVO@    R1,     R4
        ADDR    R2,     R1
        MVO@    R1,     R4
        ADDR    R2,     R1
        JR      R5
        ENDP


;;==========================================================================;;
;;  TESTHAND                                                                ;;
;;  Table of keypad dispatches for SOUNDTEST                                ;;
;;==========================================================================;;
TESTHAND        PROC
@@kp0:  DECLE   MUTETOGGLE      ; KP0
@@kp1:  DECLE   SOUNDTEST.sfx   ; KP1
@@kp2:  DECLE   SOUNDTEST.sfx   ; KP2
@@kp3:  DECLE   SOUNDTEST.sfx   ; KP3
@@kp4:  DECLE   SOUNDTEST.sfx   ; KP4
@@kp5:  DECLE   SOUNDTEST.sfx   ; KP5
@@kp6:  DECLE   SOUNDTEST.sfx   ; KP6
@@kp7:  DECLE   SOUNDTEST.mus   ; KP7
@@kp8:  DECLE   SOUNDTEST.mus   ; KP8
@@kp9:  DECLE   SOUNDTEST.mus   ; KP9
@@kpC:  DECLE   SOUNDTEST.soff  ; KPC
@@kpE:  DECLE   SOUNDTEST.moff  ; KPE
@@dsU:  DECLE   SCHEDEXIT       ; Disc Up
@@dsL:  DECLE   SCHEDEXIT       ; Disc Left
@@dsR:  DECLE   SCHEDEXIT       ; Disc Right
@@dsD:  DECLE   SCHEDEXIT       ; Disc Down
@@acT:  DECLE   SOUNDTEST.spd   ; Action Top
@@acL:  DECLE   SOUNDTEST.spd   ; Action Left
@@acR:  DECLE   SOUNDTEST.spd   ; Action Right
        ENDP



;;==========================================================================;;
;;  SOUNDTEST                                                               ;;
;;  Extremely simple Sound Test routine                                     ;;
;;==========================================================================;;
SOUNDTEST       PROC
        PSHR    R5

        ; Set our keypad dispatch
        MVII    #TESTHAND, R0
        MVO     R0,     HANDFN

        ; Force music and sound effects off
        CALL    @@soff                  ; Force sound effects off.
        CALL    @@moff                  ; Force music off.

        ; Display instructions
        MVII    #$F0,   R1              ; Set entire display to BLACK
        MVII    #$200,  R4
        CALL    FILLZERO

        MVII    #$1807, R1              ; Pastel Purple, GRAM font
        MVII    #$200 + 5, R4

        CALL    DRAWSTRING2
        BYTE    "SOUND TEST",0

        CALL    DRAWSTRING3
        DECLE   6, $200 + 40            ; Yellow, top of screen
        ;        01234567890123456789
        BYTE    "1 - 6:  Sound Effect"
        BYTE    "7,8,9:  Bkgnd Music "
        BYTE    "0:      Toggle Mute "
        BYTE    "Clear:  Stop SFX    "
        BYTE    "Enter:  Stop Music  "
        BYTE    "Action: Music speed "
        BYTE    "Disc:   Exit", 0

        CALL    DRAWSTRING3
        DECLE   1, $2F0 - 40            ; Blue, bottom of screen.
      
        ;        01234567890123456789
        BYTE    "im14u2c@primenet.com"
        BYTE    "       v1.12",0

        ; Start a Marquee task on the Sound Test string.
        MVII    #5 + 256*10, R2         ; Length 10, Offset 5
        CALL    STARTTASK
        DECLE   0                       ; Task #0
        DECLE   MARQUEE                 ; MARQUEE task
        DECLE   14,     14              ; Period: every 7 ticks

        ; Make maquee task active.
        MVII    #1,     R3
        MVO     R3,     TSKACT

        PULR    PC                      ; Return

@@soff:
        MVII    #$FF,   R1
        MVII    #FX_OFF, R4
        B       PLAY.sfxn               ; Force sound effects off

@@moff:
        MVII    #FX_OFF, R4
        B       PLAY.musn               ; Force music off

@@sfx:  ; Play a sound effect
        DECR    R2
        ADDR    R2,     R2
        MOVR    R2,     R4
        B       PLAY.sfxn

@@mus:  ; Play new music
        DECR    R2
        ADDR    R2,     R2
        MOVR    R2,     R4
        B       PLAY.musn

@@spd:  ; Toggle music speed
        MVII    #EDGEB, R0
        XOR     HEIGHT, R0
        MVO     R0,     HEIGHT
        JR      R5

        ENDP

;;==========================================================================;;
;;  GAMEOVER                                                                ;;
;;  Guess what?  It's game over time!!                                      ;;
;;==========================================================================;;
GAMEOVER        PROC
        PSHR    R5

        ; Set our keypad dispatch
        CLRR    R0
        MVO     R0,     HANDFN

        ; Stop the current game music & sfx, and play a loud BOOM!
        MVO     R0,     SNDSTRM+7   ; Make SFX enable mask go to 0
        DECR    R0
        MVO     R0,     MUTE        ; Force music to be un-muted
        MVO     R0,     DSCORL      ; set 'displayed score' to 0xFFFFFFFF
        MVO     R0,     DSCORH      ; (moved from below to save some code)
        CALL    PLAY.mus            ; Play loud BOOM on music channel
        DECLE   FXBOOM2

        ; Force the displayed score to be fully updated.
        CALL    UPDATESCORE

        ; Wait for "BOOM2" to finish
        MVII    #2,     R0
@@spin:
        CMP     SNDSTRM, R0
        BLE     @@spin

        ; Clear the well!
        CALL    CLEARWELL
        CALL    WAIT
        DECLE   20                      ; Wait 1/3rd second afterwards

        ; Put up the GAME OVER string.
        CALL    DRAWSTRING3
        DECLE   $7
        DECLE   $200 + 20*5 + 5
        BYTE    "Game Over!",0

        ; Start a Marquee task on the Game Over string.
        MVII    #105 + 256*10, R2       ; Length 10, Offset 105
        CALL    STARTTASK
        DECLE   0                       ; Task #0
        DECLE   MARQUEE                 ; MARQUEE task
        DECLE   10,     10              ; Period: every 5 ticks

        MVII    #1,     R0
        MVO     R0,     TSKACT

        ; Play the Game Over music
        CALL    PLAY.mus
        DECLE   M_OVER

        PULR    PC

        ENDP

;;==========================================================================;;
;;  SETLEVEL                                                                ;;
;;  Receives a keypad number and sets up our starting score and level       ;;
;;  number appropriately.                                                   ;;
;;                                                                          ;;
;;  INPUT:                                                                  ;;
;;  R2 -- Key/disc/action button number (0..18)                             ;;
;;                                                                          ;;
;;  OUTPUT:                                                                 ;;
;;  SLEVEL set up appropriately.                                            ;;
;;==========================================================================;;
SETLEVEL        PROC
        TSTR    R2              ; See if it's zero
        BEQ     @@got_level     ; Yes... Set it anyway.  Triggers 'sound test'
        CMPI    #10,    R2      ; See if it's > 10
        BLE     @@got_level     ; No, use this as the level number

@@default_lev:
        MVII    #4,     R2      ; Default is to start at level #4

@@got_level:
        MVO     R2,     SLEVEL  ; Store starting level number

        B       SCHEDEXIT       ; Return via SCHEDEXIT
        ENDP

;;==========================================================================;;
;;  HANDTASK                                                                ;;
;;  Handles a keypad press.                                                 ;;
;;                                                                          ;;
;;  INPUT:                                                                  ;;
;;  R2 -- Keypad key number (0..11), disc dir (12..15), or action           ;;
;;        button (16..18)                                                   ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUT:                                                                 ;;
;;  Jumps to appropriate function.  R2 is still set to the keypad number.   ;;
;;==========================================================================;;
HANDTASK         PROC
        MVI     HANDFN, R4
        TSTR    R4
        BEQ     SETLEVEL
        ADDR    R2,     R4
        MVI@    R4,     PC
        ENDP

;;==========================================================================;;
;;  KEYREPEAT                                                               ;;
;;  Key repeat task, scheduled on task #1.  De-schedules self if nothing    ;;
;;  is currently pressed.  Works by clearing LHAND/LHKPD.                   ;;
;;==========================================================================;;
KEYREPEAT       PROC
        MVII    #1,     R3      ; Prepare to disable task if needed
        MVI     LHAND,  R0      ; See if something was pressed
        MVO     R0,     REPEAT  ; Update "repeating" flag.
        TSTR    R0              ;
        BEQ     STOPTASK        ; No? Disable task and chain the return
        CLRR    R0              ; Yes? Clear last-pressed info.
        MVO     R0,     LHAND
        MVO     R0,     LHKPD
        JR      R5              ; Return.
        ENDP

;;==========================================================================;;
;;  SCANHAND                                                                ;;
;;  Scan the hand controllers and schedule tasks based on the input         ;;
;;  triggers we see.  Hand controller debouncing is done here too, which    ;;
;;  makes scanning controllers somewhat time-consuming.  (I need to         ;;
;;  eventually experiment with debounce parameters on real hardware if I    ;;
;;  plan to make this a cartridge.)                                         ;;
;;                                                                          ;;
;;  Hand controller docs:                                                   ;;
;;   Bit 0:  Down,   Row 0 keys --------- 1    2    3                       ;;
;;   Bit 1:  Right,  Row 1 keys --------- 4    5    6                       ;;
;;   Bit 2:  Up,     Row 2 keys --------- 7    8    9                       ;;
;;   Bit 3:  Left,   Row 3 keys --------- C    0    E                       ;;
;;   Bit 4:  Corner                       |    |    |                       ;;
;;   Bit 5:  T/L     Col 2 keys --------- | -- | ---+                       ;;
;;   Bit 6:  L/R     Col 1 keys --------- | ---+                            ;;
;;   Bit 7:  T/R     Col 0 keys ----------+                                 ;;
;;==========================================================================;;
        ;        E    C    9    8    7    6    5    4    3    2    1    0
PADTBL: BYTE    $28, $88, $24, $44, $84, $22, $42, $82, $21, $41, $81, $48
        ;       rgt  lft  top
ACTTBL: BYTE    $C0, $60, $A0
        ;       bot  rgt  lft  top
DSCTBL: BYTE    $01, $02, $08, $04

SCANHAND        PROC
DBOUT   EQU     5               ; Debounce outer loop
DBIN    EQU     120             ; Debounce inner loop
DB80    EQU     (80*DBIN)/100   ; 80% of inner loop iters

@@startover:
        CLRR    R2
        MVO     R2,     PAUSED  ; Clear 'we paused' flag

        MVII    #DBIN,  R2
@@dbnothing:
        MVI     CTRL0,  R0
        AND     CTRL1,  R0
        XORI    #$FF,   R0

        ; See if it is the same as our last hand-controller input.
        ; If so, just ignore it.
        CMP     LHAND,  R0
        BEQ     @@nothing

        MOVR    R0,     R1      ; Lets see if it's anything at all
        BNEQ    @@input         ; If it comes up non-empty, process it.

        DECR    R2              ; If it comes up empty, debounce before
        BNEQ    @@dbnothing     ; saying it's just nothing.
        B       @@noinput
@@input:

        ; If last input was keypad, ignore this if they share any bits.
        MVI     LHKPD,  R0
        ANDR    R1,     R0
        BNEQ    @@nothing

        ; Debounce the input.  If we read the same value >80% of the
        ; iterations of the debounce inner loop, we say it's good.
        ; Otherwise we iterate the outer loop.
@@debounce:
        MVII    #DBOUT, R3      ; Outer loop max count
@@oloop:
        CLRR    R2              ; Match count
        MVII    #DBIN,  R0      ; Inner loop count

        MVI     CTRL0,  R1
        AND     CTRL1,  R1
@@iloop:
        MVI     CTRL0,  R4
        AND     CTRL1,  R4
        CMPR    R4,     R1
        BNEQ    @@notsame       ; Yes... don't count it
        INCR    R2              ; No... do count it.
@@notsame:
        DECR    R0              ; Count down inner loop
        BNEQ    @@iloop
        XORI    #$FF,   R1

        CMPI    #DB80,  R2      ; Was it same >80% of inner loop?
        BGE     @@ok            ; Yes, keep it.

        DECR    R3              ; No, iterate outer loop
        BNEQ    @@oloop
        ; If we fall through here, just parse whatever we read last.
        ; Somebody really needs to clean their controllers!
@@ok:
        MVI     PAUSED, R3      ; If we paused during debounce, start this
        TSTR    R3              ; whole thing over.
        BNEQ    @@startover

        ; Parse controller info in R1.
        ; If bit 4 is set, assume this is NOT a keypad press.
        MOVR    R1,     R0
        ANDI    #$10,   R0
        BNEQ    @@nopad

        ; See if it EXACTLY matches a keypad key
        MVII    #PADTBL,R4
        MVII    #11,    R0
@@kloop:
        CMP@    R4,     R1
        BEQ     @@gotpad
        DECR    R0
        BGE     @@kloop
        B       @@nopad

@@gotpad:
        MVO     R1,     LHAND   ; Set last-hand value

        ; If we _think_ we got a keypad input, make DAMN SURE by debouncing
        ; AGAIN with 3x the debounce inner loop.
        MVII    #3*DBIN, R3    ; Inner loop count
@@kpdloop:
        MVI     CTRL0,  R4
        AND     CTRL1,  R4
        XORI    #$FF,   R4
        CMPR    R4,     R1
        BNEQ    @@debounce      ; Miscompare?  Hell with this!
@@kpnotsame:
        DECR    R3              ; Count down inner loop
        BNEQ    @@kpdloop

        MVO     R1,     LHKPD   ; Record that last input was keypad.

        MOVR    R0,     R1
        B       @@dispatch      ; Keypad keys don't repeat.

@@notrepeating:
        PSHR    R5
        CALL    STARTTASK       ; Make the keypress repeat
        DECLE   1               ; Task #1
        DECLE   KEYREPEAT       ; KEYREPEAT task
        DECLE   30,     8       ; First repeat 250ms, rest @ 15Hz
        PULR    R5

@@repeating:
        PULR    R1
@@dispatch:
        MVII    #HANDTASK, R0   ; Queue the HANDTASK to process key

        JD      QTASK

@@nopad:
        CLRR    R0
        MVO     R0,     LHKPD

        MOVR    R1,     R0      ; Copy input to R0 to decode action buttons.
        ANDI    #$E0,   R0
        BEQ     @@noaction      ; Don't decode action btns if there are none

        ; Action buttons shouldn't repeat like the disc, so avoid doing
        ; action buttons if these bits match bits in REPEAT.
@@hmm
        MVI     REPEAT, R2
        ANDI    #$E0,   R2
        CMPR    R2,     R0
        BEQ     @@noaction

        MVI     LHAND,  R2
        ANDI    #$E0,   R2
        CMPR    R2,     R0
        BEQ     @@noaction

        PSHR    R1
        ; Ok, decode the action buttons.
        MVII    #ACTTBL,R4      ; R4 points to the "action table"
        MVII    #3,     R1
@@actloop:
        CMP@    R4,     R0
        BEQ     @@gotaction
        DECR    R1
        BNEQ    @@actloop
        B       @@notaction
@@gotaction:
        ; Queue an action-key event
        ADDI    #15,    R1
        PSHR    R5
        MVII    #HANDTASK, R0   ; Queue the HANDTASK to process key
        JSRD    R5,     QTASK
        PULR    R5
@@notaction:
        PULR    R1

@@noaction:
        ; Now process disc events.
        MVI     LHAND,  R2      ; Get our previous controller input
        MVO     R1,     LHAND   ; Save current input
        MOVR    R1,     R3

        MVII    #DSCTBL,R4      ; R4 points to the "disc table"
        MVII    #4,     R1
@@discloop:
        MOVR    R3,     R0      ; Restore word.
        AND@    R4,     R0      ; Compare against a direction bit.
        BNEQ    @@gotdisc       ; Is this bit set?
        DECR    R1
        BNEQ    @@discloop
        B       @@nothing
@@gotdisc:
        ADDI    #11,    R1
        PSHR    R1
        MOVR    R0,     R3
        AND     REPEAT, R0      ; See if we're repeating
        BNEQ    @@repeating
        ANDR    R3,     R2      ; See if this is just our prev keypress
        BEQ     @@notrepeating  ; Yup, this is new.
        PULR    R1
        B       @@nothing       ; ... otherwise ignore it.

@@noinput:
        CLRR    R1
        MVO     R1,     LHAND   ; Clear last-hand
        MVO     R1,     LHKPD   ; Clear last-was-keypad
        MVO     R1,     REPEAT  ; Clear repeating key
@@nothing:
        JR      R5              ; Return.

        ENDP

;;==========================================================================;;
;;  NEXTRAND                                                                ;;
;;  Simple task which updates the random number generator.  It advances     ;;
;;  the random number generator's state by one bit.  This is a good one     ;;
;;  to call from the background task to keep things random.                 ;;
;;                                                                          ;;
;;  NEXTRANDX                                                               ;;
;;  Same as NEXTRAND, only it iterates for R2 iterations, generating R2     ;;
;;  new random bits.  (default is only one new random bit)                  ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R2 -- Number of iterations (NEXTRANDX only)                             ;;
;;  R5 -- Return address                                                    ;;
;;  Random state in RANDLO, RANDHI                                          ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0, R1 -- Contains current rand # state.                                ;;
;;  R2 -- Zeroed                                                            ;;
;;  R4 -- Trashed                                                           ;;
;;==========================================================================;;
NEXTRAND        PROC
        MVII    #1,     R2
NEXTRANDX
        MVII    #RANDLO, R4
        MVI@    R4,     R0
        MVI@    R4,     R1

@@loop:
        SLLC    R0,     1
        RLC     R1,     1
        BNC     @@nocarry
;       XORI    #$5,    R0      ; period==(2**31 - 1) polynomial
        XORI    #$04C1, R1      ; period==(2**32 - 1) polynomial (CRC-32 poly)
        XORI    #$1DB7, R0
@@nocarry:
        DECR    R2
        BNEQ    @@loop

        SUBI    #2,     R4
        MVO@    R0,     R4
        MVO@    R1,     R4
        JR      R5

        ENDP

;;==========================================================================;;
;; POW10                                                                    ;;
;; Look-up table with powers of 10 as 32-bit numbers (little endian)        ;;
;;                                                                          ;;
;; NPW10                                                                    ;;
;; Same as POW10, only -10**x instead of 10**x                              ;;
;;==========================================================================;;

POW10
POW10_9 DECLE  1000000000 AND $FFFF, 1000000000 SHR 16  ; 10**9
POW10_8 DECLE  100000000  AND $FFFF, 100000000  SHR 16  ; 10**8
POW10_7 DECLE  10000000   AND $FFFF, 10000000   SHR 16  ; 10**7
POW10_6 DECLE  1000000    AND $FFFF, 1000000    SHR 16  ; 10**6
POW10_5 DECLE  100000     AND $FFFF, 100000     SHR 16  ; 10**5
POW10_4 DECLE  10000      AND $FFFF, 10000      SHR 16  ; 10**4
POW10_3 DECLE  1000       AND $FFFF, 1000       SHR 16  ; 10**3
POW10_2 DECLE  100        AND $FFFF, 100        SHR 16  ; 10**2
POW10_1 DECLE  10         AND $FFFF, 10         SHR 16  ; 10**1
POW10_0 DECLE  1          AND $FFFF, 1          SHR 16  ; 10**0

NPW10
NPW10_9 DECLE -1000000000 AND $FFFF,(-1000000000 SHR 16) AND $FFFF  ;-10**9
NPW10_8 DECLE -100000000  AND $FFFF,(-100000000  SHR 16) AND $FFFF  ;-10**8
NPW10_7 DECLE -10000000   AND $FFFF,(-10000000   SHR 16) AND $FFFF  ;-10**7
NPW10_6 DECLE -1000000    AND $FFFF,(-1000000    SHR 16) AND $FFFF  ;-10**6
NPW10_5 DECLE -100000     AND $FFFF,(-100000     SHR 16) AND $FFFF  ;-10**5
NPW10_4 DECLE -10000      AND $FFFF,(-10000      SHR 16) AND $FFFF  ;-10**4
NPW10_3 DECLE -1000       AND $FFFF,(-1000       SHR 16) AND $FFFF  ;-10**3
NPW10_2 DECLE -100        AND $FFFF,(-100        SHR 16) AND $FFFF  ;-10**2
NPW10_1 DECLE -10         AND $FFFF,(-10         SHR 16) AND $FFFF  ;-10**1
NPW10_0 DECLE -1          AND $FFFF,(-1          SHR 16) AND $FFFF  ;-10**0

;;==========================================================================;;
;;  DEC16                                                                   ;;
;;  Displays a 16-bit decimal number on the screen with leading blanks      ;;
;;  in a field up to 5 characters wide.                                     ;;
;;                                                                          ;;
;;  DEC16A                                                                  ;;
;;  Same as DEC16, only displays leading zeroes.                            ;;
;;                                                                          ;;
;;  DEC16B                                                                  ;;
;;  Same as DEC16, only leading zeros are controlled by bit 15 of R3.       ;;
;;  (If set, suppress leading zeros.  If clear, show leading zeros.)        ;;
;;                                                                          ;;
;;  DEC16C                                                                  ;;
;;  Same as DEC16B, except R1 contains an amount to add to the first digit  ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Decimal number                                                    ;;
;;  R1 -- (DEC16C only): Amount to add to initial digit.                    ;;
;;  R2 -- Number of digits to suppress  (If R2<=5, it is 5-field_width)     ;;
;;  R3 -- Color mask                                                        ;;
;;  R4 -- Screen offset (8-bit)                                             ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Zeroed                                                            ;;
;;  R1 -- Trashed                                                           ;;
;;  R2 -- Remaining digits to suppress (0 if initially <= 5.)               ;;
;;  R3 -- Color mask, with bit 15 set if no digits displayed.               ;;
;;  R4 -- Pointer to character just right of string                         ;;
;;  R5 -- Trashed                                                           ;;
;;                                                                          ;;
;;  Routine uses $110, $111 for temporary storage.                          ;;
;;==========================================================================;;
DEC16   PROC
@@so    EQU     $110
@@fw    EQU     $111
        SETC                    ; Prepare to set bit 15 of color mask
        INCR    PC              ; Skip the CLRC
DEC16A
        CLRC                    ; Prepare to clear bit 15 of clrmask
        SLL     R3,     1
        RRC     R3,     1       ; Set/clear bit 15 of color mask
DEC16B
        CLRR    R1
DEC16C
        PSHR    R5              ; Save return address
        MVII    #POW10_4, R5    ; Point to '10000' entry in POW10
        MVO     R4,     @@so    ; Save screen offset
        MVO     R2,     @@fw    ; Save field width
        MVII    #5,     R4      ; Iterate 5 (16-bit goes to 65536)
        MOVR    R1,     R2
        INCR    PC
@@digitlp:

        CLRR    R2              ; Start with division result == 0
        MVI@    R5,     R1      ; Load power of 10
        INCR    R5              ; Point to next smaller power of 10
@@divloop:
        INCR    R2
        SUBR    R1,     R0      ; Divide by repeated subtraction
        BC      @@divloop
        ADDR    R1,     R0      ; Loop iterates 1 extra time: Fix it.
        DECR    R2              ; Fix extra iter.  Also test if 0
        BNEQ    @@disp          ; If digit != 0, display it.
        TSTR    R3              ; If digit == 0 and no lead 0, skip
        BMI     @@blank
@@disp:
        SLL     R3,     1       ; Clear "no leading 0" flag
        SLR     R3,     1       ;
        MVI     @@fw,   R1      ; Get field width
        DECR    R1              ; Are we in active field yet?
        BMI     @@ok            ; Yes: Go ahead and display
        MVO     R1,     @@fw    ; No: Save our count-down till field
        B       @@iter          ;     and don't display the digit.
@@blank:
        MOVR    R3,     R2      ; Blank character _just_ gets format
        MVI     @@fw,   R1      ; Get field width
        DECR    R1              ; Are we in active field yet?
        BMI     @@drawit        ; Yes: Go ahead and display
        MVO     R1,     @@fw    ; No: Save our count-down till field
        B       @@iter          ;     and don't display the digit.
@@ok:
        ADDI    #$10,   R2      ; Pseudo-ASCII digits start at 0x10
        SLL     R2,     2       ; Put pseudo-ASCII char in position
        ADDR    R2,     R2      ; ... by shifting left 3
        XORR    R3,     R2      ; Merge with display format
@@drawit:
        MVI     @@so,   R1      ; Get screen offset
        XORI    #$200,  R1      ; Move to screen
        MVO@    R2,     R1      ; Put character on screen
        INCR    R1              ; Move the pointer
        MVO     R1,     @@so    ; Save the new offset
@@iter:
        DECR    R4              ; Count down our digit count
        BNEQ    @@digitlp       ; Keep iterating

        MVI     @@so,   R4      ; Restore offset
        MVI     @@fw,   R2      ; Restore digit suppress ocunt

        PULR    PC              ; Whew!  Done!
        ENDP

;;==========================================================================;;
;;  DEC32                                                                   ;;
;;  Displays a 32 bit number without leading zeros.  It performs this feat  ;;
;;  by calling DEC16 twice.                                                 ;;
;;                                                                          ;;
;;  DEC32A                                                                  ;;
;;  Same as DEC32, except leading zeros are displayed.                      ;;
;;                                                                          ;;
;;  DEC32B                                                                  ;;
;;  Same as DEC32, except leading zeros are controlled by bit 15 of R3      ;;
;;  (If set, suppress leading zeros.  If clear, show leading zeros.)        ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Low half of 32-bit number                                         ;;
;;  R1 -- High half of 32-bit number                                        ;;
;;  R2 -- Number of leading digits to suppress (10 - field width)           ;;
;;  R3 -- Screen format word                                                ;;
;;  R4 -- Screen offset (8-bit)                                             ;;
;;                                                                          ;;
;;  $110..$113 used for temporary storage                                   ;;
;;==========================================================================;;
DEC32   PROC
@@so    EQU     $110
@@fw    EQU     $111
@@fmt   EQU     $35D

        SETC                    ; Prepare to set bit 15 of color mask
        INCR    PC              ; Skip the CLRC
DEC32A
        CLRC                    ; Prepare to clear bit 15 of clrmask
        SLL     R3,     1
        RRC     R3,     1       ; Set/clear bit 15 of color mask
        NOP                     ; (interruptible for STIC)
DEC32B
        PSHR    R5              ; Save return address
        MVO     R2,     @@fw    ; Save field width
        MVO     R4,     @@so    ; Save screen offset
        MVO     R3,     @@fmt

        CLRR    R3
        PSHR    R3              ; Push accumulator (init'd to 0)

        ; Use division by repeated subtraction to generate a 16-bit
        ; value which represents the first 5 digits of the 10 digit number.
        MVII    #NPW10_9, R5    ; Point to -10**9

@@digitlp:
        CLRR    R3
        MVI@    R5,     R2      ; Load low half of 32-bit -10**x
        MVI@    R5,     R4      ; Load high half of 32-bit -10**x
@@divlp:
        SLR     R3,     1
@@divlpb:
        INCR    R3
        ADDR    R2,     R0      ; Add the low half
        ADCR    R1              ; Add carry from low half
        RLC     R3,     1       ; See if adding the carry carried
        ADDR    R4,     R1      ; Add high half
        BC      @@divlp         ; Loop if we had a carry from either
        SARC    R3,     1       ;   upper half ADD.  (We can't get
        BC      @@divlpb        ;   a carry from both, though.)

        ; Subtract off the extra iteration
        SUBR    R2,     R0      ; Subtract the low half.
        ADCR    R1              ; Add in the "not-borrow"
        DECR    R1              ; Turn "not-borrow" into "borrow"
        SUBR    R4,     R1      ; Subtract the high half.

        DECR    R3
        BEQ     @@nxtdigit

        ; Take our count and multiply it by the appropriate power of 10.
        MOVR    R5,     R2
        MOVR    R3,     R4
        SUBI    #NPW10_4, R2    ; Translate 10**x to 10**(x-5)
        BEQ     @@donemult
@@mult:
        ADDR    R4,     R4      ; To mult by 10, do (x<<1)+(x<<3)
        MOVR    R4,     R3
        SLL     R3,     2
        ADDR    R3,     R4
        ADDI    #$4,    R2
        BLT     @@mult
@@donemult:
        ADD@    SP,     R4      ; Add this to our 16-bit accum.
        PSHR    R4              ; that we keep on top-of-stack
@@nxtdigit:
        CMPI    #NPW10_4, R5
        BLT     @@digitlp

        MVI     @@fw,   R2      ; Restore field width
        MVI     @@so,   R4      ; Restore screen offset
        MVI     @@fmt,  R3      ; ...
        MVO     R0,     @@fmt   ; Save low half momentarily.
        PULR    R0              ; Get accumulated word for display
        PSHR    R1              ; Save upper bit
        CALL    DEC16B          ; Display first five digits

        ; Now, our 32-bit number should be less than 100000.  That
        ; means R1 should be 0 or 1.  We display the last five digits
        ; as a single 16-bit number by handling that bit separately.

        MVI     @@fmt,  R0      ; Restore lower 16 bits

        PULR    R1              ; Get upper bit
        TSTR    R1              ; Was it zero?
        BEQ     @@noextra       ; Yes:  Nothing special to do
        MVII    #6,     R1      ; No: Add 6 to the leading digit
        ADDI    #5536,  R0      ; ... and "5536" to remaining digits
@@noextra:
        PULR    R5              ; Chain the return.
        B       DEC16C          ; Display remaining digits.  WHEW!
        ENDP

;;==========================================================================;;
;;  DEC32Z                                                                  ;;
;;  Same as DEC32, except a zero is displayed in the final position if      ;;
;;  the whole number's value is zero.                                       ;;
;;                                                                          ;;
;;  DEC16Z                                                                  ;;
;;  Same as DEC16, except a zero is displayed in the final position if      ;;
;;  the whole number's value is zero.                                       ;;
;;==========================================================================;;
DEC32Z  PROC
        TSTR    R0              ; Is lower half non-zero?
        BNEQ    DEC32           ; Yes:  Go display the number as per usual.
        TSTR    R1              ; Is upper half non-zero?
        BNEQ    DEC32           ; Yes:  Go display the number as per usual.
        MVII    #10,    R1      ; Otherwise, display a 0 in a field that is
        B       @@dozero        ; ... up to 10 spaces wide.
DEC16Z:
        TSTR    R0              ; Is the number non-zero?
        BNEQ    DEC16           ; Yes:  Go display the number as per usual.
        MVII    #5,     R1      ; Otherwise, display a 0 in a field that is
                                ; ... up to 5 spaces wide.
@@dozero:
        SUBR    R2,     R1      ; Account for suppressed digits.
        BLE     @@nodisp        ; Display nothing if # of suppressed digits
                                ; is greater than or equal to our field width.

        ADDI    #$200,  R4      ; Point into display memory.
        INCR    PC              ; Skip the first MVO@ in loop below.
@@loop:
        MVO@    R3,     R4      ; Output a blank space (leading blanks)
        DECR    R1              ; Decrement our count of leading blanks.
        BNEQ    @@loop          ; Keep looping until we've filled the blank
                                ; ... portion of the displayed number.

        XORI    #$80,   R3      ; Now, display the digit 0 in last position.
        MVO@    R3,     R4
        XORI    #$80,   R3      ; Finally, restore our display format word.

@@nodisp:
        JR      R5              ; Return to the caller.
        ENDP

;;==========================================================================;;
;;  DRAWSTRING1                                                             ;;
;;  Puts an ASCIIZ string pointed to by R0 onscreen.                        ;;
;;                                                                          ;;
;;  DRAWSTRING2                                                             ;;
;;  Puts an ASCIIZ string after a JSR R5 onscreen.                          ;;
;;                                                                          ;;
;;  DRAWSTRING3                                                             ;;
;;  Reads R1, R4 from @ R5, then does DRAWSTRING2.                          ;;
;;                                                                          ;;
;;  DRAWSTRING4                                                             ;;
;;  Reads R4 from @ R5, then does DRAWSTRING2.                              ;;
;;                                                                          ;;
;;  DRAWSTRING5                                                             ;;
;;  Reads R0, R1, R4 from @ R5, then does DRAWSTRING1.                      ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- String pointer (if DRAWSTRING)                                    ;;
;;  R1 -- Screen format word                                                ;;
;;  R4 -- Output pointer                                                    ;;
;;  R5 -- Return address (also, string if DRAWSTRING2).                     ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Zero if DRAWSTRING2, one if DRAWSTRING                            ;;
;;  R1 -- Untouched EXCEPT bit 15 is cleared.                               ;;
;;  R4 -- Points just after displayed string.                               ;;
;;  R5 -- Points just past end of string.                                   ;;
;;  R2 and R3 are not modified.                                             ;;
;;==========================================================================;;
DRAWSTRING      PROC

DRAWSTRING5
        MVI@    R5,     R0
        MVI@    R5,     R1
        MVI@    R5,     R4
DRAWSTRING1
        PSHR    R5
        MOVR    R0,     R5
        SETC
        INCR    PC
DRAWSTRING2:
        CLRC
        SLL     R1,     1
        RRC     R1,     1
        MVI@    R5,     R0      ; Get first char of string
@@tloop:
        SUBI    #32,    R0      ; Shift ASCII range to charset
        SLL     R0,     2       ; Move it to position for BTAB word
        SLL     R0,     1
        XORR    R1,     R0      ; Merge with color info
        MVO@    R0,     R4      ; Write to display
        MVI@    R5,     R0      ; Get next character
        TSTR    R0              ; Is it NUL?
        BNEQ    @@tloop         ; --> No, keep copying then

        SLLC    R1,     1
        ADCR    R0
        SLR     R1,     1
        ADDR    R0,     PC
        JR      R5
        PULR    PC
DRAWSTRING3:
        MVI@    R5,     R1
DRAWSTRING4:
        MVI@    R5,     R4
        B       DRAWSTRING2
        ENDP

;;==========================================================================;;
;;  FILLZERO                                                                ;;
;;  Fills memory with zeros                                                 ;;
;;                                                                          ;;
;;  FILLMEM                                                                 ;;
;;  Fills memory with a constant                                            ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R0 -- Fill value (FILLMEM only)                                         ;;
;;  R1 -- Number of words to fill                                           ;;
;;  R4 -- Start of fill area                                                ;;
;;  R5 -- Return address                                                    ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R0 -- Zeroed (if FILLZERO)                                              ;;
;;  R1 -- Zeroed                                                            ;;
;;  R4 -- Points to word after fill area                                    ;;
;;==========================================================================;;
FILLZERO        PROC
        CLRR    R0
FILLMEM
        MVO@    R0,     R4
        DECR    R1
        BNEQ    FILLMEM
        JR      R5
        ENDP

;;==========================================================================;;
;;  SNDPRIO                                                                 ;;
;;                                                                          ;;
;;  Task which prioritizes sound streams by giving stream #0                ;;
;;  all of the channel bits that aren't being used by stream #1.  This is   ;;
;;  called by the ISR during sound-stream updates.                          ;;
;;                                                                          ;;
;;  This task also implements the "Mute" functionality.                     ;;
;;==========================================================================;;
SNDPRIO         PROC
        MVI     SNDSTRM+4,R0            ; load current hold count for strm 2
        CLRR    R1
        CMPI    #2,     R0              ; is it less than 2?
        BLT     @@inactive              ; yes: stream is inactive
        MVI     SNDSTRM+7,R1
@@inactive:
        XORI    #$3FFF, R1

        MVI     MUTE,   R0
        TSTR    R0
        BNEQ    @@notmute
        ANDI    #$7FF,  R1              ; Disallow access to volume.
@@notmute:

        MVO     R1,     SNDSTRM+3

        JR      R5
        ENDP

;;==========================================================================;;
;;  DOSNDSTREAM  -- This is the sound engine!                               ;;
;;                                                                          ;;
;;  PSG memory map                                                          ;;
;;                                                                          ;;
;;        $1F4:$1F0       Channel A period (12 bits)                        ;;
;;        $1F5:$1F1       Channel B period (12 bits)                        ;;
;;        $1F6:$1F2       Channel C period (12 bits)                        ;;
;;        $1F7:$1F3       Envelope period (16 bits)                         ;;
;;                                                                          ;;
;;        $1F8            Channel enables                                   ;;
;;        $1F9            Noise period (5 bits)                             ;;
;;        $1FA            Envelope mode                                     ;;
;;                                                                          ;;
;;        $1FB            Channel A volume                                  ;;
;;        $1FC            Channel B volume                                  ;;
;;        $1FD            Channel C volume                                  ;;
;;                                                                          ;;
;;        $1FE, $1FF      Hand controllers (not used for sound.)            ;;
;;                                                                          ;;
;;  Sound is microprogrammed with a series of records of the following      ;;
;;  format:                                                                 ;;
;;                                                                          ;;
;;    INSTRUCTION WORD (two formats):                                       ;;
;;                                                                          ;;
;;    Format 1 (BREF = 0)                                                   ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+----+----+----+----+ ;;
;;    |          DURATION TO HOLD THIS FRAME          |QUIT|ZERO|PCTL|BREF| ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+----+----+----+----+ ;;
;;                                                                          ;;
;;    Format 2 (BREF = 1)                                                   ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+----+----+----+----+ ;;
;;    | BREF OFFSET RELATIVE TO $ - 2 | DUR. TO HOLD  |QUIT|ZERO|PCTL|BREF| ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+----+----+----+----+ ;;
;;                                                                          ;;
;;    CONTROL WORD                                                          ;;
;;    +---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+    ;;
;;    | 0 |LINK|snD|snC|snB|snA|sn9|sn8|sn7|sn6|sn5|sn4|sn3|sn2|sn1|sn0|    ;;
;;    +---+----+---+---+---+---+---+---+---+---+---+---+---+---+---+---+    ;;
;;                                                                          ;;
;;    DATA BYTES, 1 word for each two snX bits set.                         ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+     ;;
;;    |    SECOND BYTE TO WRITE       |     FIRST BYTE TO WRITE       |     ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+     ;;
;;                                                                          ;;
;;    LINK ADDRESS (if LINK set), 1 word, as-is.                            ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+     ;;
;;    |          ADDRESS TO CONTINUE SOUND PROCESSING AT.             |     ;;
;;    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+     ;;
;;                                                                          ;;
;;                                                                          ;;
;;  Instruction Word details:                                               ;;
;;                                                                          ;;
;;  The DUR field in the Instruction Word specifies how long a set of       ;;
;;  values is held in the PSG before it processes the next word.  The       ;;
;;  duration is specified in ticks, where 1 tick == 1/60th second.  A       ;;
;;  duration of 0 means "immediately"--the sound processor will start       ;;
;;  processing the next sound record as soon as it completes this one.      ;;
;;  This is useful for combining operations, such as setting ZERO &         ;;
;;  PCTL to zero the previous notes, and then sending new ones (if they     ;;
;;  are on different channels).                                             ;;
;;                                                                          ;;
;;  The QUIT, ZERO, PCTL, BREF and LINK flags control the following         ;; 
;;  behavior:                                                               ;;
;;                                                                          ;;
;;    QUIT   When set, this terminates the sound given sound microprogram   ;;
;;           after the duration specified.                                  ;;
;;                                                                          ;;
;;    ZERO   When set, this tells the sound processor that no bytes are     ;;
;;           coded for this frame since all were zeros.  This causes the    ;;
;;           processor to write a pattern of zeros guided by the control    ;;
;;           word.                                                          ;;
;;                                                                          ;;
;;    PCTL   When set, this tells the sound processor to use the previous   ;;
;;           control word.  The previous control word is not defined when   ;;
;;           the stream is started, but it is defined across a link.        ;;
;;                                                                          ;;
;;    BREF   When set, this frame is coded only as a back-reference.  All   ;;
;;           control decles and data bytes come from the back reference.    ;;
;;           Note:  When using a back-reference, the QUIT, ZERO, PCTL, and  ;;
;;           DUR settings are used from the Instruction Word for the        ;;
;;           _current_ frame!  Also, the duration is limited to four bits   ;;
;;           when BREF=1, since the upper byte gives the back-reference     ;;
;;           relative offset.                                               ;;
;;                                                                          ;;
;;    LINK   When set, this tells the sound processor that this frame       ;;
;;           is followed by the address for the next frame.  Otherwise,     ;;
;;           the sound processor assumes that the next frame is after this  ;;
;;           one.  LINK is effectively a "jump after this" instruction.     ;;
;;           NOTE:  LINK is actually in the Control Word, and is remembered ;;
;;           when PCTL=1 in the next record.                                ;;
;;                                                                          ;;
;;  Control Word details:                                                   ;;
;;                                                                          ;;
;;  The bits sn0 ... snD report whether bytes are coded for each of the     ;;
;;  fourteen sound registers on the PSG.  The PCTL bit in the Instruction   ;;
;;  word specifies whether the previous control word is reused for the      ;;
;;  current frame or is read from the stream.                               ;;
;;                                                                          ;;
;;  Sound Engine details:                                                   ;;
;;                                                                          ;;
;;  Up to two sound programs may be running at once.  It is up to the       ;;
;;  programmer to ensure that they do not try to control the same channels  ;;
;;  at once.  The intended use is to have music playing on one stream       ;;
;;  and sound effects on a second stream.                                   ;;
;;                                                                          ;;
;;  The following state is kept for each active sound stream in 16-bit      ;;
;;  memory:                                                                 ;;
;;                                                                          ;;
;;      HOLDCNT    -- Number of ticks left for current record times 2       ;;
;;                    If the LSB of HOLDCNT is set, stream processing stops ;;
;;                    after this record.                                    ;;
;;                                                                          ;;
;;      PREVCTL    -- Previous control word.                                ;;
;;                                                                          ;;
;;      NEXTREC    -- Pointer to the next stream record.                    ;;
;;                                                                          ;;
;;      STRMMSK    -- Stream channel mask.  This is used to prevent a       ;;
;;                    stream from updating certain channel parameters and   ;;
;;                    is most useful when, say, a sound-effect needs to     ;;
;;                    borrow a channel briefly.                             ;;
;;                                                                          ;;
;;  NOTE: Bit 15 of control word (eg. bit 7 of second word) MUST BE 0.      ;;
;;        Otherwise, copy loop will loop forever.  Also, bit 14 of stream   ;;
;;        channel mask MUST BE 0, otherwise the copy loop will write part   ;;
;;        of the link address to one of the hand controller addresses.  :-) ;;
;;                                                                          ;;
;;==========================================================================;;
DOSNDSTREAM:    PROC
        MVI@    R5,     R4
        PSHR    R5

        MVI@    R4,     R1              ; Load tick/hold count (REC[0])
        SUBI    #2,     R1              ; Count down the hold count
        BMI     @@inactive              ; If count goes negative, channel is
                                        ;    inactive.
        BEQ     @@nextrec               ; If count goes to zero, load next
                                        ;    sound record into the channel

        DECR    R4                      ; Otherwise, store the updated count.
        MVO@    R1,     R4              ; count ==> REC[0]

@@inactive:
        PULR    PC                      ; Return.

@@backup:
        DECR    R4
@@nextrec:
        MVI@    R4,     R5              ; Load the stream record ptr (REC[1])
        MVI@    R5,     R2              ; Load hold tick-count from stream
        MOVR    R2,     R3
        SARC    R2,     1               ; See if this is a back-reference
        BNC     @@notbref
        ANDI    #$7F,   R2              ; Clear BREF offset out of upper bits.
        SWAP    R3
        ANDI    #$FF,   R3              ; Extract BREF offset from upper 8 bits
        ADDI    #2,     R3              ; Make BREF relative to just before rec
        DECR    R4
        MVO@    R5,     R4              ; Store updated record ptr (REC[1])
        SUBR    R3,     R5              ; Subtract relative offset
@@notbref:
        SARC    R2,     1               ; See if we have new control word.
        BC      @@prevctl               ; If PCTL = 1, use prev control word.
        MVI@    R5,     R1              ; Load new control-word from stream
        MVO@    R1,     R4              ; Store updated ctrl-word to str info
                                        ;   (REC[2])
        INCR    PC                      ; Skip the following MVI@.
@@prevctl:
        MVI@    R4,     R1              ; Load previous ctrl-word from str info
                                        ;   (REC[2])

        MVI@    R4,     R0              ; Load the stream channel mask (REC[3])
        SUBI    #4,     R4              ; Point back at hold tick-count
        SARC    R2,     1               ; See if we are writing a zero-record
        BNEQ    @@notzero
        ; If this was a zero hold count, set up so that we will process
        ; next record.  This is almost like tail-recursion.
        MVII    #@@backup, R3
        PSHR    R3
@@notzero:
        MVO@    R2,     R4              ; Store new tick count (REC[0])

        ; We generate a write mask by ANDing the stream channel mask
        ; with the control word.  The result is a bit-mask describing
        ; which PSG registers we'll actually write to.
        ANDR    R1,     R0              ; Generate channel write mask
        MVII    #$1F0,  R2              ; Point to the PSG for writing.

        BC      @@zerorec               ; If we're writing a zero record,
                                        ; don't read any bytes from stream.

        PSHR    R4
        MVII    #1,     R4
        COMR    R0
        SARC    R1,     1       ; 6     ; See if we have a coded word.
@@loop:
        BNC     @@noread        ; 9 7   ; If not, skip the read.
@@read:
        SWAP    R3,     1
        XORI    #1,     R4
        ADDR    R4,     PC
        MVI@    R5,     R3      ; 8     ; ... otherwise read word from stream
        SARC    R0,     1       ; 6     ; See if we are to write this word.
        ADCR    PC              ; 6
        MVO@    R3,     R2      ; 8     ; ... otherwise write to the PSG
        INCR    R2              ; 6     ; Always update the PSG pointer.

        SARC    R1,     1       ; 6     ; See if we have a coded word.
        BNEQ    @@loop          ; 9 7   ; Yes: Keep looping!
        BC      @@read          ;       ; affects last bit ONLY.

@@finish:
        PULR    R4

        CMPI    #$1FF,  R2              ; Did we have a link pointer in this
                                        ; record?
        BLT     @@nolink                ; If not, then don't grab one here.
        MOVR    R3,     R5              ; Move link pointer to record pointer
        B       @@waslink               ; Skip the back-ref test.
@@nolink:
        CMP@    R4,     R5              ; Detect if this was a back-ref
        BLE     @@return                ; If it was, our pointer didn't advance
        DECR    R4                      ; Else, prepare to store new rec ptr
@@waslink:
        MVO@    R5,     R4              ; Store record ptr to stream info
@@return:
        PULR    PC                      ; Return

@@noread:
        INCR    R2              ; 6
        SARC    R0,     1       ; 6
        SARC    R1,     1       ; 6
        BNEQ    @@loop          ; 9 7
        BC      @@read          ;      ; affects last bit ONLY.
        B       @@finish

                                ; 56  coded and written
                                ; 48  coded, not written
                                ; 36  not coded, not written
@@zerorec:
        CLRR    R3
        SARC    R0,     1       ; 6     ; See if we are to write this word.
@@zloop:
        BNC     @@nozero        ; 9 7   ; Skip write if not in write mask
        MVO@    R3,     R2      ; 8     ; ... otherwise write to the PSG
@@nozero:
        INCR    R2              ; 6     ; Always update the PSG0pointer.
        SARC    R0,     1       ; 6     ; See if we are to write this word.
        BNEQ    @@zloop         ; 9 7   ; Yes: Keep looping!
        BNC     @@zlink
        MVO@    R3,     R2

        ; We need to handle link records specially here.
@@zlink:
        SLLC    R1,     2               ; Check bit 14.
        BNOV    @@nolink                ; If set, we have link, else, we don't
        MVI@    R5,     R3              ; Load the link address
        MVO@    R3,     R4              ; Store record ptr to stream info

        PULR    PC

        ENDP


;;==========================================================================;;
;;  PLAY.sfx                                                                ;;
;;  Plays a sound effect on stream #1.                                      ;;
;;                                                                          ;;
;;  PLAY.sfxp                                                               ;;
;;  Plays a sound effect with raised priority on stream #1.                 ;;
;;                                                                          ;;
;;  PLAY.mus                                                                ;;
;;  Plays a music stream on stream #0.                                      ;;
;;                                                                          ;;
;;  PLAY.sfxn                                                               ;;
;;  Same as PLAY.sfxp, only sound effect number is passed in R4             ;;
;;  instead of in the invocation record.                                    ;;
;;                                                                          ;;
;;  PLAY.musn                                                               ;;
;;  Same as PLAY.mus, only sound effect number is passed in R4              ;;
;;  instead of in the invocation record.                                    ;;
;;                                                                          ;;
;;  INPUTS:                                                                 ;;
;;  R1 -- Priority (if PLAY.sfxp or PLAY.sfxn)                              ;;
;;  R5 -- Invocation record (if PLAY.sfx, PLAY.sfxp, PLAY.mus)              ;;
;;        DECLE   Index into SNDTBL.                                        ;;
;;                                                                          ;;
;;  OUTPUTS:                                                                ;;
;;  R1, R4 is clobbered                                                     ;;
;;  Sound is triggered on requested channel.                                ;;
;;  Returns at R5, or just after if invocation record is used.              ;;
;;==========================================================================;;
PLAY            PROC

@@musn: CLRC                            ; Flag that this is music
        INCR    PC                      ; (skip the SETC)
@@sfxn: SETC                            ; Flag that this is a sound effect
        B       @@go2                   ; Start the music/sound effect

@@sfx:  CLRR    R1                      ; If PLAY.sfx, set prio == 0.
@@sfxp: SETC                            ; Flag that this is a sound effect
        INCR    PC                      ; (skip the CLRC)
@@mus:  CLRC                            ; Flag that this is music.


@@go:   MVI@    R5,     R4              ; Read invocation record.
@@go2:  PSHR    R5                      ; Save return address
        PSHR    R0                      ; Save R0 (minimize clobbering)

        MVII    #SNDSTRM+4, R5          ; point to sound effect stream
        BC      @@notmusic
        SUBI    #4,     R5              ; If carry was set, point to sfx stream
        CLRR    R1
        DECR    R1
@@notmusic:

        ; Note: The following is non-interruptible, and that's necessary
        ; to avoid a race w/ the sound code that runs in the ISR.
        DIS
        ; First see if something is already playing on this stream
        MVII    #2,     R0
        CMP@    R5,     R0              ; Compare hold delay vs. 2.
        BGT     @@notbusy               ; If delay > 2, stream is busy

        ; If the channel is busy, check our sound priority.  This test is
        ; performed only for sound effects.  (INCR R1/BEQ is always taken
        ; for music).
        INCR    R1                      ; If this is music, don't do prio.
        BEQ     @@music                 ;

        CMP     SFXPRIO, R1             ; Now see if this sound effect is
                                        ; high enough priority to play.
        BLT     @@noplay                ; No... don't play it.

@@play:
        MVO     R1,     SFXPRIO         ; Update the sound effect priority

@@music:
        ADDI    #2,     R5              ; Point to stream mask
        MVI@    R5,     R0              ; Get the current stream mask
        SUBI    #3,     R5              ; Move the pointer back
        SWAP    R0,     1               ; We're interested in vol bits
                                        ; and channel enables.

        SLR     R0,     2               ; Shift away bits we don't want

        SARC    R0,     2               ; ChanEn->C  VolA->OV
        BNC     @@no_chan_en            ; ChanEn set?
        MVII    #$38,   R1              ;Yes...
        MVO     R1,     PSG0+10         ;Set channel enables to "Tone Only"
@@no_chan_en:
        CLRR    R1                      ; Prepare to write zeros.
        BNOV    @@no_vol_a              ; VolA set?
        MVO     R1,     PSG0+11         ;Yes... Clear Volume for channel A
        MVO     R1,     PSG0+0          ;Yes... Clear PeriodLo for channel A
        MVO     R1,     PSG0+4          ;Yes... Clear PeriodHi for channel A
@@no_vol_a:
        SARC    R0,     2               ; VolB->C  VolC->OV
        BNC     @@no_vol_b              ; VolB set?
        MVO     R1,     PSG0+12         ;Yes... Clear Volume for channel B
        MVO     R1,     PSG0+1          ;Yes... Clear PeriodLo for channel B
        MVO     R1,     PSG0+5          ;Yes... Clear PeriodHi for channel B
@@no_vol_b:
        BNOV    @@no_vol_c              ; VolC set?
        MVO     R1,     PSG0+13         ;Yes... Clear Volume for channel C
        MVO     R1,     PSG0+2          ;Yes... Clear PeriodLo for channel C
        MVO     R1,     PSG0+6          ;Yes... Clear PeriodHi for channel C
@@no_vol_c:
        B       @@dorecord

@@notbusy:
        INCR    R1
        BEQ     @@dorecord
        MVO     R1,     SFXPRIO         ; Update the sound effect priority
@@dorecord:

        ; At this point R4 is an index into the sound table
        ; and R5 points to the second word of the stream record.

        ADDI    #SNDTBL,R4              ; Point R5 into sound table

        DECR    R5
        MVII    #2,     R1              ; Start off with hold count = 2
        MVO@    R1,     R5              ; Store hold count == 2
        MVI@    R4,     R1
        MVO@    R1,     R5              ; Store record pointer
        INCR    R5
        MVI@    R4,     R1
        MVO@    R1,     R5              ; Store channel mask

@@noplay:
        EIS
        PULR    R0
        PULR    PC
        ENDP

;;==========================================================================;;
;;  SOUND EFFECTS                                                           ;;
;;  BOMB    -- Plays a descending pitch following by an explosion.          ;;
;;  DING3   -- Dings 3 times (signals "next level")                         ;;
;;  BOOM    -- Just the explosion                                           ;;
;;  SILENCE -- The calm after the storm                                     ;;
;;==========================================================================;;
BOMB
        DECLE   16                      ; 1/60th second
        DECLE   $2144                   ; Channel C period/vol, Channel Enable
        DECLE   $0010, $0C38            ; Period: $010, ChEnable: $38, Vol: $0C

        DECLE   16                      ; 1/60th second
        DECLE   $04                     ; Rapid descening pitch on channel C
        DECLE        $18, $12, $20, $12, $30, $12, $40
        DECLE   $12, $50, $12, $60, $12, $70, $12, $80
BOOM
        DECLE   800                     ; Long delay (5/6ths second)
        DECLE   $2788                   ; Env & noise period, Ch.Enable, C Vol
        DECLE   $28FF                   ; Env Period == $28FF
        DECLE   $1F1C                   ; Noise on channel C only., period 1F
        DECLE   $3F00                   ; Envelope type = 0000
SILENCE
        DECLE   0
        DECLE   $0100
        DECLE   $38                     ; No noise enabled

        DECLE   12
        DECLE   $3AFF                   ; All except envelope trigger

BOOM2
        DECLE   4                       ; No delay
        DECLE   $3AFF                   ; Clear nearly everything

        DECLE   $3F0                    ; Long delay
        DECLE   $3F88                   ; Env & noise period, Ch.Enable, Vols
        DECLE   $1FFF   
        DECLE   $1F07                   ; Noise on all channels, period 1F
        DECLE   $3F00                   ; Envelope type = 0000
        DECLE   $3F3F                   ; Volume = 15 + Envelope

        DECLE   $0F0                    ; Long delay
        DECLE   $4000
        DECLE   SILENCE

BIP
        DECLE   $20                     ; 1/30th second
        DECLE   $2144                   ; Chan C
        DECLE   $00FF, $0938            ; Period: $0FF, ChEnable: $38, Vol: $09

        DECLE   $0A                     ; 0/60th second, PCTL, QUIT
        DECLE   $0000, $0038            ; Zero all but channel enables.

BYUMP
        DECLE   16                      ; 1/60th second
        DECLE   $2144                   ; Channel C period/vol, Channel Enable
        DECLE   $0410, $0C38            ; Period: $510, ChEnable: $38, Vol: $0D
        DECLE   16                      ; 1/60th second
        DECLE   $04                     ; Rapid descening pitch on channel C
        DECLE   $50, $12, $90
        DECLE   $10                     ; Last frame, chain to SILENCE
        DECLE   $4044
        DECLE   $04E0
        DECLE   SILENCE

DING3
        DECLE   15*16                   ; 1/4th second
        DECLE   $35EE                   ; Channel C
        DECLE   $6160, $00FF
        DECLE   $1800                   ; B = $060 C = $061, Env Per = $18FF
        DECLE   $0038                   ; Channel enables, Envelope type 0
        DECLE   $3F3F                   ; Volume = 15 + envelope

        DECLE   15*16 + 4               ; 1/4th second + zero
        DECLE   $0400                   ; Hit envelope register w/ Zero

        DECLE   15*16 + 4               ; 1/4th second, + zero
        DECLE   $4400                   ; Hit envelope register w/ Zero & Link
        DECLE   SILENCE                 ; Silence afterwards.

BOMB4
        DECLE   $14                     ; One tick
        DECLE   $3FFF                   ; Clear everything.
                                        ; (makes the 'gong' more consistent).

        DECLE   $10                     ;
        DECLE   $1DBB                   ;
        DECLE   $1419, $07FF,  $3807
        DECLE   $0038, $3F3F

        DECLE   16                      ; 1/60th second
        DECLE   $2044                   ; Channel C period/vol, Channel Enable
        DECLE   $0010, $000C            ; Period: $010, Vol: $0C

        DECLE   16                      ; 1/60th second
        DECLE   $04                     ; Rapid descening pitch on channel C
        DECLE               $18,$12,$20,$12,$28,$12,$30,$12,$38,$12,$40
        DECLE   $12,$10,$12,$18,$12,$20,$12,$28,$12,$30,$12,$38,$12,$40
        DECLE   $12,$10,$12,$18,$12,$20,$12,$28,$12,$30,$12,$38,$12,$40
        DECLE   0
        DECLE   $4000, BOMB

;;==========================================================================;;
;; Background music data                                                    ;;
;;==========================================================================;;
        INCLUDE "nut1mrch.asm"
        INCLUDE "chindnce.asm"
        INCLUDE "behappy.asm"

;;==========================================================================;;
;;  FONT                                                                    ;;
;;==========================================================================;;
        INCLUDE "font.asm"


        DECLE   "http://www.primenet.com/~im14u2c/intv/", 0
        DECLE   "Copyright 2000 Joseph Zbiciak",0
        DECLE   "JZ"
        DECLE   0
