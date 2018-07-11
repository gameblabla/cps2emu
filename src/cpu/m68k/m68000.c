/******************************************************************************

	m68000.c

	M68000 CPU«¤«ó«¿«Õ«§£­«¹Î¼Þü

******************************************************************************/

#ifdef M68K

#include "emumain.h"


static u32 m68k_encrypt_start;
static u32 m68k_encrypt_end;
static u8  *m68k_decrypted_rom;

void m68000_set_encrypted_range(u32 start, u32 end, void *decrypted_rom)
{
	m68k_encrypt_start = start;
	m68k_encrypt_end   = end;
	m68k_decrypted_rom = (u8 *)decrypted_rom;
}

u8 m68000_read_pcrelative_8(u32 offset)
{
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return m68k_decrypted_rom[offset ^ 1];
	else
		return m68000_read_memory_8(offset);
}

u16 m68000_read_pcrelative_16(u32 offset)
{
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return *(u16 *)&m68k_decrypted_rom[offset];
	else
		return m68000_read_memory_16(offset);
}

static int irq_state;

int m68000_irq_callback(int irqline)
{
	if (irq_state == HOLD_LINE) {
		irq_state = CLEAR_LINE;
		m68k_set_irq(0);
	}
	return M68K_INT_ACK_AUTOVECTOR;
}


void m68000_init(void)
{
    m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68000_set_encrypted_range(0, memory_length_cpu1-1, memory_region_cpu1);
}


void m68000_reset(void)
{
	irq_state = CLEAR_LINE;
	m68k_pulse_reset();
}


void m68000_exit(void)
{
	/* nothing to do ? */
}


int m68000_execute(int cycles)
{
	return m68k_execute(cycles);
}


void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	irq_state = state;

	switch (state)
	{
	case CLEAR_LINE:
		m68k_set_irq(0);
		return;

	case ASSERT_LINE:
		m68k_set_irq(irqline);
		return;

	default:
		m68k_set_irq(irqline);
		return;
	}
}


/*------------------------------------------------------
	«»£­«Ö/«í£­«É «¹«Æ£­«È
------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( m68000 )
{
    /*
	int i;
	u32 pc = C68k_Get_PC(&C68K);

	for (i = 0; i < 8; i++)
		state_save_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_save_long(&C68K.A[i], 1);

	state_save_long(&C68K.flag_C, 1);
	state_save_long(&C68K.flag_V, 1);
	state_save_long(&C68K.flag_Z, 1);
	state_save_long(&C68K.flag_N, 1);
	state_save_long(&C68K.flag_X, 1);
	state_save_long(&C68K.flag_I, 1);
	state_save_long(&C68K.flag_S, 1);
	state_save_long(&C68K.USP, 1);
	state_save_long(&pc, 1);
	state_save_long(&C68K.HaltState, 1);
	state_save_long(&C68K.IRQLine, 1);
	state_save_long(&irq_state, 1);
	*/
}

STATE_LOAD( m68000 )
{
    /*
	int i;
	u32 pc;

	for (i = 0; i < 8; i++)
		state_load_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_load_long(&C68K.A[i], 1);

	state_load_long(&C68K.flag_C, 1);
	state_load_long(&C68K.flag_V, 1);
	state_load_long(&C68K.flag_Z, 1);
	state_load_long(&C68K.flag_N, 1);
	state_load_long(&C68K.flag_X, 1);
	state_load_long(&C68K.flag_I, 1);
	state_load_long(&C68K.flag_S, 1);
	state_load_long(&C68K.USP, 1);
	state_load_long(&pc, 1);
	state_load_long(&C68K.HaltState, 1);
	state_load_long(&C68K.IRQLine, 1);
	state_load_long(&irq_state, 1);

	C68k_Set_PC(&C68K, pc);
	*/
}

#endif /* SAVE_STATE */

#endif // !GP2X
