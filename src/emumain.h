/******************************************************************************

	emumain.c

	Emulator core

******************************************************************************/

#ifndef EMUMAIN_H
#define EMUMAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#ifdef GP2X
#include <sys/mman.h>
#endif

#define CPS1 0
#define CPS2 1
#define MVS 2

#define CPS_VERSION 2
#define EMU_SYSTEM CPS2
#define USE_CACHE		1
//#define SAVE_STATE
//#define HISCORE
#define MMUHACK
#ifndef INLINE
#define INLINE __inline
#endif

#define FPS				59.633333
#define EEPROM_SIZE		128
#define CACHE_SIZE      0x1000000

#define ALIGN_DATA		__attribute__((aligned(16)))
#define MEM_ALIGN       16

#define SOUND_SAMPLES		(1024*2)
#define SOUND_BUFFER_SIZE	(SOUND_SAMPLES*2)

#define MAX_SPRITE  8192

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include "emudraw.h"
#include "include/osd_cpu.h"
#include "include/cpuintrf.h"
#include "include/memory.h"
#include "zip/zfile.h"
#include "cps2/loadrom.h"
#include "cps2/state.h"
#include "cps2/cache.h"
#include "cps2/coin.h"
#include "cps2/hiscore.h"

typedef u64 TICKER;

#define ui_popup(...)
#define ui_show_popup(x)
#define ui_popup_reset(x)

/*
#define msg_screen_init(x)
#define msg_screen_clear(x)
#define msg_printf printf
*/
void msg_screen_init();
void msg_screen_clear();
void msg_printf(const char *text, ...);

#define save_gamecfg(x)
#define load_gamecfg(x)

#define video_clear_screen(x)
#define video_flip_screen(x)
#define video_wait_vsync(x)

#define pad_wait_clear(x)
#define pad_wait_press(x) button_wait()
void button_wait();

#ifdef GP2X
#define memset gp2x_memset
#define memcpy gp2x_memcpy
#endif

#ifdef WIN32
#define memalign(x, l) malloc(l)
#endif

extern volatile int Loop;
extern char launchDir[MAX_PATH];

extern u8 *upper_memory;
extern const u8 font_data[];

enum
{
	LOOP_EXIT = 0,
	LOOP_BROWSER,
	LOOP_RESTART,
	LOOP_RESET,
	LOOP_EXEC
};

#if (CPS_VERSION == 2)
#include "cps2/cps2.h"
#else
#include "cps1/cps1.h"
#endif

extern char game_name[16];
extern char parent_name[16];
extern char game_dir[MAX_PATH];

#if (CPS_VERSION == 2)
extern char cache_parent_name[16];
extern char cache_dir[MAX_PATH];
#endif

extern int option_showfps;
extern int option_showtitle;
extern int option_autoframeskip;
extern int option_frameskip;
extern int option_speedlimit;
extern int option_vsync;
extern int option_rescale;
extern int option_screen_position;
extern int option_linescroll;
extern int option_fullcache;
extern int option_extinput;
extern int option_xorrom;
extern int option_tweak;
extern int option_cpuspeed;
extern int option_hiscore;

extern int option_sound_enable;
extern int option_samplerate;
extern int option_sound_volume;

extern int option_m68k_clock;
extern int option_z80_clock;

extern int machine_driver_type;
extern int machine_input_type;
extern int machine_init_type;
extern int machine_screen_type;
extern int machine_sound_type;

extern u32 frames_displayed;
extern int fatal_error;


void emu_main(void);

void reset_frameskip(void);
void autoframeskip_reset(void);

void update_screen(void);

void save_snapshot(void);

#endif /* EMUMAIN_H */
