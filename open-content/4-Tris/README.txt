;;===========================================================================;;
;; Joe Zbiciak's 4-Tris -- A "Falling Tetrominoes" Game for Intellivision    ;;
;; Copyright 2000, Joe Zbiciak, im14u2c@primenet.com                         ;;
;; http://www.primenet.com/~im14u2c/intv/                                    ;;
;;===========================================================================;;

----------------
 Emulator Notes
----------------

 -- The Numeric Keypad is reversed.  Intellivision keypads put [1], [2],
    and [3] in the top row of the keypad, much like a telephone.  The
    emulator retains this spatial relationship, despite the fact that
    PC's put [7], [8] and [9] in the top row.  So, you will need to
    remember this little tidbit when starting the game or using the 
	sound-test screen.

 -- On INTVPC, the [0] key on the keypad is mapped to the Intellivision's
    [0] and the [.] key is mapped to the Intellivision's [C].  On jzIntv,
	it's the other way around.

 -- The "Action Buttons" are mapped to Ctrl, Shift, and Alt.

 -- The "Disc" is mapped to the arrow keys (the Inverted-T, NOT the ones
    on the number-pad)!

-------------------
 Game Instructions
-------------------

At the title screen, press a number from [1]-[9] or [C] on the keypad
to select a starting level.  (Buttons [1] through [9] start you at
levels 1 through 9, and [C] starts you at Level 10.)  If you press
[0] at the title screen, you will be taken to a "Sound Test" screen.
(See "Sound Test" below for more details.)  Any other input on the
controller starts you at Level 4.  (Level 4 seemed like a good mid-way
default level.)

In the game, the controllers are set up like so:

  DISC:              Moves piece left, right or down
  Action Buttons:    Rotates piece.  Top button rotates counter-clockwise,
                     bottom buttons rotate clockwise.
  [4], [6], or [C]   Toggles "Next Piece" display
  [7] or [9]         Mutes/un-mutes background music.

Pieces fall until they hit an obstruction which keeps them from falling.
When a piece can't fall any further, it is "placed", and a score is
assessed for that piece's placement.

First, the player is awarded a small number of points for each downward
move that the player made with the piece.  The player is awarded 5 
pts/move if the next piece was displayed at while this piece was falling, 
10 pts/move otherwise.  This rewards fast play.  Next, any completed 
lines are cleared away, and a cleared-line bonus is awarded.  The table 
below illustrates the line clear bonuses.  Notice that it's worth your 
while to clear more lines at a time.

Line Clear Bonuses:

 +-----------------+----------------------------------   -------------------+
 |  Number of      |  Level Number......              ...                   |
 |  Lines Cleared  |    1   |   2   |   3   |   4   | ... |        n        |
 |-----------------+--------+-------+-------+-------+-   -+-----------------+
 |        1        |   1000 |  1500 |  2000 |  2500 | ... |  500 * (n + 1)  |
 |        2        |   3000 |  4500 |  6000 |  7500 | ... | 1500 * (n + 1)  |
 |        3        |   6000 |  9000 | 12000 | 15000 | ... | 3000 * (n + 1)  |
 |        4        |  12000 | 18000 | 24000 | 30000 | ... | 6000 * (n + 1)  |
 +-----------------+--------+-------+-------+-------+-   -+-----------------+

As lines are cleared, the player is moved up in level.  Each level has
a maximum number of lines associated with it, which is "10 * level".
When that maximum is reached, the player is moved to the next level.
For example, when a player reaches 40 lines, the player moves from Level
4 to Level 5 (if the player didn't start at a higher level number).

------------
 Sound Test
------------

The Sound Test screen allows the player to just play around with the
sound effects that are embedded in the 4-Tris ROM image.  Press buttons
on the keypad to trigger sound effects and music.  Use the action
buttons to toggle the music playback speed.  Press Disc to exit.

-------------
 Source Code
-------------

See the file "SOURCE.txt" for information on 4-Tris' source code.
