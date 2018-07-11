/******************************************************************************

	m68000.c

	M68000 CPU«¤«ó«¿«Õ«§£­«¹Î¼Þü

******************************************************************************/

#ifdef FAME

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
/*
void m68000_irq_callback(u32 irqline)
{
	if (irq_state == HOLD_LINE) {
		irq_state = CLEAR_LINE;
		m68k_set_irq(0);
	}
}
*/
M68K_PROGRAM prg_fetch[] = {
    {0x000000, 0x3fffff, 0},
    {0x660000, 0x663fff, (u32)cps2_ram - 0x660000},
    {0x900000, 0x92ffff, (u32)cps1_gfxram - 0x900000},
    {0xff0000, 0xffffff, (u32)cps1_ram - 0xff0000},
    {-1, -1, 0}
};

M68K_DATA data_rb[] = {
    {0x000000, 0x3fffff, NULL, NULL},
    {0x400000, 0x65ffff, m68000_read_memory_8, NULL},
    {0x660000, 0x663fff, NULL, (u8 *)cps2_ram - 0x660000},
    {0x700000, 0x8fffff, m68000_read_memory_8, NULL},
    {0x900000, 0x92ffff, NULL, (u8 *)cps1_gfxram - 0x900000},
    {0xff0000, 0xffffff, NULL, (u8 *)cps1_ram - 0xff0000},
    {-1, -1, NULL, NULL}
};

M68K_DATA data_rw[] = {
    {0x000000, 0x3fffff, NULL, NULL},
    {0x400000, 0x65ffff, m68000_read_memory_16, NULL},
    {0x660000, 0x663fff, NULL, (u8 *)cps2_ram - 0x660000},
    {0x700000, 0x8fffff, m68000_read_memory_16, NULL},
    {0x900000, 0x92ffff, NULL, (u8 *)cps1_gfxram - 0x900000},
    {0xff0000, 0xffffff, NULL, (u8 *)cps1_ram - 0xff0000},
    {-1, -1, NULL, NULL}
};

M68K_DATA data_wb[] = {
    {0x000000, 0x3fffff, NULL, NULL},
    {0x400000, 0x65ffff, m68000_write_memory_8, NULL},
    {0x660000, 0x663fff, NULL, (u8 *)cps2_ram - 0x660000},
    {0x700000, 0x8fffff, m68000_write_memory_8, NULL},
    {0x900000, 0x92ffff, NULL, (u8 *)cps1_gfxram - 0x900000},
    {0xff0000, 0xffffff, NULL, (u8 *)cps1_ram - 0xff0000},
    {-1, -1, NULL, NULL}
};

M68K_DATA data_ww[] = {
    {0x000000, 0x3fffff, NULL, NULL},
    {0x400000, 0x65ffff, m68000_write_memory_16, NULL},
    {0x660000, 0x663fff, NULL, (u8 *)cps2_ram - 0x660000},
    {0x700000, 0x8fffff, m68000_write_memory_16, NULL},
    {0x900000, 0x92ffff, NULL, (u8 *)cps1_gfxram - 0x900000},
    {0xff0000, 0xffffff, NULL, (u8 *)cps1_ram - 0xff0000},
    {-1, -1, NULL, NULL}
};

void m68000_init(void)
{
    M68K_CONTEXT m68k;

    m68k_init();
    
    memset(&m68k, 0, sizeof(M68K_CONTEXT));
    
    if(memory_length_user1)
        prg_fetch[0].offset = (u32)memory_region_user1;
    else
        prg_fetch[0].offset = (u32)memory_region_cpu1;
    data_rb[0].data = data_rw[0].data = data_wb[0].data = data_ww[0].data = memory_region_cpu1;

    m68k.fetch = prg_fetch;
    m68k.read_byte = data_rb;
    m68k.read_word = data_rw;
    m68k.write_byte = data_wb;
    m68k.write_word = data_ww;

    m68k.sv_fetch = prg_fetch;
    m68k.sv_read_byte = data_rb;
    m68k.sv_read_word = data_rw;
    m68k.sv_write_byte = data_wb;
    m68k.sv_write_word = data_ww;

    m68k.user_fetch = prg_fetch;
    m68k.user_read_byte = data_rb;
    m68k.user_read_word = data_rw;
    m68k.user_write_byte = data_wb;
    m68k.user_write_word = data_ww;
    
    //m68k.iack_handler = m68000_irq_callback;

    m68k_set_context(&m68k);
}


void m68000_reset(void)
{
	irq_state = CLEAR_LINE;
	m68k_reset();
}


void m68000_exit(void)
{
	/* nothing to do ? */
}


int m68000_execute(int cycles)
{
	return m68k_emulate(cycles);
}


void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	irq_state = state;

	switch (state)
	{
	case CLEAR_LINE:
		m68k_lower_irq(0);
		return;

	case ASSERT_LINE:
		m68k_raise_irq(irqline, M68K_AUTOVECTORED_IRQ);
		return;

	default:
		m68k_raise_irq(irqline, M68K_AUTOVECTORED_IRQ);
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

#endif // !FAME
