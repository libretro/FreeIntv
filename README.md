# FreeIntv
FreeIntv is a libretro emulation core for the Mattel Intellivision designed to be compatible with joypads from the SNES era forward even if they originally required a number pad.

## BIOS
FreeIntv requires two Intellivision BIOS files to be placed in the libretro 'system' folder for proper emulation:

| Function | Filename* | MD5 Hash |
| --- | --- | --- | 
| Executive ROM | `exec.bin`  | `62e761035cb657903761800f4437b8af` |
| Graphics ROM | `grom.bin` | `0cd5946c6473e42e8e4c2137785e427f`

* BIOS filenames are case-sensitive

## Controller overlays
Mattel Intellivision games were often meant to be played with game-specific cards overlaid on the numeric keypad. These overlays convey information which can be very useful in gameplay. Images of a limited selection of Intellivision titles are available at: http://www.intellivisionlives.com/bluesky/games/instructions.shtml

## Controls
* The d-pad will give you 8-way movement. If available, the left analog stick will function like the 16-way disc.
* The FreeIntv "mini keypad" allows you to view and press keys on a small keypad that appears in the lower-corner of the display while L or R is being held.
* The "X" button is also mapped to the last selected keypad button, giving quick access. In Astrosmash, for example, you can leave "3" selected to enable instant access to hyperspace.
* The select button lets you switch the left and right controllers. Some games expect the left controller to be player one, others expect the right controller. This isn't a problem if you have two controllers (and don't mind juggling them) but users with only one controller or using a portable setup would be effectively locked out of some games. Pressing select from either controller with swap the left controller for the right and vice-versa.

## Entertainment Computer System and Intellivoice
FreeIntv does not currently support Entertainment Computer System (ECS) and Intellivoice functionality. Contributions to the source are welcome!
