#
# Makefile for Linux
# It's mostly for conveniance and also so i don't have to recompile SDL2 each time.
#

CC = gcc
CXX = g++

OUTPUTNAME = cps2emu.elf

DEFINES = -DSDL
INCLUDES = -I. -Isrc -Isrc/cps2 -I/usr/include/SDL

OPT_FLAGS  = -O2 -m32

CFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) -std=gnu99 -DRS97
CXXFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) -std=gnu++98
LDFLAGS = -lSDL -lm -pthread -ldl -lstdc++ -lz

# Redream (main engine)
OBJS =  \
src/emumain.o src/emudraw.o src/usbjoy.o src/font.o src/zip/unzip.o src/zip/zfile.o \
src/sound/qsound.o src/sound/sndintrf.o src/cps2/cache.o src/cps2/cps2.o src/cps2/cps2crpt.o \
src/cps2/driver.o src/cps2/eeprom.o src/cps2/inptport.o src/cps2/memintrf.o src/cps2/sprite.o src/cps2/state.o src/cps2/timer.o \
src/cps2/vidhrdw.o src/cps2/loadrom.o src/cps2/coin.o src/cps2/hiscore.o \
src/cpu/m68000/c68k.o src/cpu/m68000/m68000.o \
src/cpu/z80/cz80.o src/cpu/z80/z80.o
 
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< 
	
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $< 
	
all: executable

executable : $(OBJS)
	$(CC) -o $(OUTPUTNAME) $(OBJS) $(CFLAGS) $(LDFLAGS)

clean:
	rm $(OBJS) $(OUTPUTNAME)
