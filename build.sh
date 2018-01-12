#!/bin/sh

gcc -shared -fPIC freeintv_libretro.c intv.c memory.c cp1610.c stic.c psg.c controller.c cart.c osd.c -lm -lc -lgcc -o freeintv_libretro.so
