/******************************************************************************

	m68000.h

	M68000 CPU interface

******************************************************************************/

#ifdef FAME

#ifndef M68000_H
#define M68000_H

#include "fame.h"

enum
{
	/* NOTE: M68K_SP fetches the current SP, be it USP, ISP, or MSP */
	M68K_PC=1, M68K_SP, M68K_ISP, M68K_USP, M68K_MSP, M68K_SR, M68K_VBR,
	M68K_SFC, M68K_DFC, M68K_CACR, M68K_CAAR, M68K_PREF_ADDR, M68K_PREF_DATA,
	M68K_D0, M68K_D1, M68K_D2, M68K_D3, M68K_D4, M68K_D5, M68K_D6, M68K_D7,
	M68K_A0, M68K_A1, M68K_A2, M68K_A3, M68K_A4, M68K_A5, M68K_A6, M68K_A7
};

void m68000_init(void);
void m68000_reset(void);
void m68000_exit(void);
int m68000_execute(int cycles);
void m68000_set_irq_line(int irqline, int state);
void m68000_set_irq_callback(int (*callback)(int irqline));

void m68000_set_encrypted_range(u32 start, u32 end, void *decrypted_rom);

#ifdef SAVE_STATE
STATE_SAVE( m68000 );
STATE_LOAD( m68000 );
#endif

#endif /* M68000_H */

#endif // !FAME
