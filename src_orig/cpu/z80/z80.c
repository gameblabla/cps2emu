/******************************************************************************

	z80.c

	Handling CZ80 core.

******************************************************************************/

#ifndef GP2X

#include "emumain.h"
#include "cz80.h"


static u8 irq_state;

int z80_irq_callback(int line)
{
	if (irq_state == HOLD_LINE)
	{
		irq_state = CLEAR_LINE;
		Cz80_Clear_IRQ(&CZ80);
	}
	return 0xff;
}

void z80_init(void)
{
	Cz80_Init(&CZ80);
	Cz80_Set_Fetch(&CZ80, 0x0000, 0xffff, (u32)memory_region_cpu2);
	Cz80_Set_ReadB(&CZ80, &z80_read_memory_8);
	Cz80_Set_WriteB(&CZ80, &z80_write_memory_8);
	Cz80_Set_IRQ_Callback(&CZ80, &z80_irq_callback);
	Cz80_Reset(&CZ80);
}


void z80_reset(void)
{
	Cz80_Reset(&CZ80);
}


void z80_exit(void)
{
	/* nothing to do ? */
}


int z80_execute(int cycles)
{
    return Cz80_Exec(&CZ80, cycles);
}


static INLINE void z80_assert_irq(int irqline)
{
	if (irqline == IRQ_LINE_NMI)
		Cz80_Set_NMI(&CZ80);
	else
		Cz80_Set_IRQ(&CZ80, irqline);
}


static INLINE void z80_clear_irq(int irqline)
{
	Cz80_Clear_IRQ(&CZ80);
}


void z80_set_irq_line(int irqline, int state)
{
	irq_state = state;

	switch (state)
	{
	case CLEAR_LINE:
		z80_clear_irq(irqline);
		return;

	case ASSERT_LINE:
		z80_assert_irq(irqline);
		return;

	default:
		z80_assert_irq(irqline);
		return;
	}
}


#ifdef SAVE_STATE
void state_save_z80(FILE *fp)
{
	u32 pc = Cz80_Get_PC(&CZ80);

	state_save_word(&CZ80.BC.W, 1);
	state_save_word(&CZ80.DE.W, 1);
	state_save_word(&CZ80.HL.W, 1);
	state_save_word(&CZ80.AF.W, 1);
	state_save_word(&CZ80.IX.W, 1);
	state_save_word(&CZ80.IY.W, 1);
	state_save_word(&CZ80.SP.W, 1);
	state_save_long(&pc, 1);
	state_save_word(&CZ80.BC2.W, 1);
	state_save_word(&CZ80.DE2.W, 1);
	state_save_word(&CZ80.HL2.W, 1);
	state_save_word(&CZ80.AF2.W, 1);
	state_save_word(&CZ80.R.W, 1);
	state_save_word(&CZ80.IFF.W, 1);
	state_save_byte(&CZ80.I, 1);
	state_save_byte(&CZ80.IM, 1);
	state_save_byte(&CZ80.IRQState, 1);
	state_save_byte(&CZ80.HaltState, 1);
	state_save_byte(&irq_state, 1);
}

void state_load_z80(FILE *fp)
{
	u32 pc;

	state_load_word(&CZ80.BC.W, 1);
	state_load_word(&CZ80.DE.W, 1);
	state_load_word(&CZ80.HL.W, 1);
	state_load_word(&CZ80.AF.W, 1);
	state_load_word(&CZ80.IX.W, 1);
	state_load_word(&CZ80.IY.W, 1);
	state_load_word(&CZ80.SP.W, 1);
	state_load_long(&pc, 1);
	state_load_word(&CZ80.BC2.W, 1);
	state_load_word(&CZ80.DE2.W, 1);
	state_load_word(&CZ80.HL2.W, 1);
	state_load_word(&CZ80.AF2.W, 1);
	state_load_word(&CZ80.R.W, 1);
	state_load_word(&CZ80.IFF.W, 1);
	state_load_byte(&CZ80.I, 1);
	state_load_byte(&CZ80.IM, 1);
	state_load_byte(&CZ80.IRQState, 1);
	state_load_byte(&CZ80.HaltState, 1);
	state_load_byte(&irq_state, 1);

	Cz80_Set_PC(&CZ80, pc);
}
#endif

#endif // !GP2X
