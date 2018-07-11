/******************************************************************************

	z80.h

	Handling Z80 core.

******************************************************************************/

#ifndef Z80INTF_H
#define Z80INTF_H

extern int z80_ICount;

void z80_init(void);
void z80_reset(void);
void z80_exit(void);
int  z80_execute(int cycles);
void z80_set_irq_line(int irqline, int state);

#ifdef SAVE_STATE
STATE_SAVE( z80 );
STATE_LOAD( z80 );
#endif

#endif /* Z80INTF_H */
