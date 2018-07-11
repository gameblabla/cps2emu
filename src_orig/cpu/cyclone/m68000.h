/******************************************************************************

	m68000.h

	M68000 CPU interface

******************************************************************************/

#ifdef GP2X

#ifndef M68000_H
#define M68000_H

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

#endif // GP2X
