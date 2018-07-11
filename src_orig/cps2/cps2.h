/******************************************************************************

	cps2.c

	CPS2«¨«ß«å«ì£­«·«ç«ó«³«¢

******************************************************************************/

#ifndef CPS2_H
#define CPS2_H
#include "emumain.h"
#if defined(N900)
#include "cpu/cyclone/m68000.h"
#include "cpu/z80/z80.h"
#elif defined(GP2X)
#include "cpu/cyclone/m68000.h"
#include "cpu/drz80/z80.h"
#else
#ifdef M68K
#include "cpu/m68k/m68000.h"
#elif defined(FAME)
#include "cpu/fame/m68000.h"
#else
#include "cpu/m68000/m68000.h"
#endif
#include "cpu/z80/z80.h"
#endif
#include "sound/sndintrf.h"
#include "sound/qsound.h"
#include "timer.h"
#include "driver.h"
#include "eeprom.h"
#include "inptport.h"
#include "memintrf.h"
#include "sprite.h"
#include "vidhrdw.h"

void cps2_main(void);

#endif /* CPS2_H */
