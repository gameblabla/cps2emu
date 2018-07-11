/******************************************************************************

	m68000.c

	C68K M68000 CPU interface function

******************************************************************************/

#if !defined(GP2X) && !defined(M68K) && !defined(FAME)

#include "emumain.h"
#include "c68k.h"


static u32 m68k_encrypt_start;
static u32 m68k_encrypt_end;
static u8  *m68k_decrypted_rom;

void m68000_set_encrypted_range(u32 start, u32 end, void *decrypted_rom)
{
	m68k_encrypt_start = start;
	m68k_encrypt_end   = end;
	m68k_decrypted_rom = (u8 *)decrypted_rom;
}

static u8 m68000_read_pcrelative_8(u32 offset)
{
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return m68k_decrypted_rom[offset ^ 1];
	else
		return m68000_read_memory_8(offset);
}

static u16 m68000_read_pcrelative_16(u32 offset)
{
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return *(u16 *)&m68k_decrypted_rom[offset];
	else
		return m68000_read_memory_16(offset);
}

static int irq_state;

static int m68000_irq_callback(int irqline)
{
	if (irq_state == HOLD_LINE)
	{
		irq_state = CLEAR_LINE;
		C68k_Set_IRQ(&C68K, 0);
	}
	return C68K_INTERRUPT_AUTOVECTOR_EX + irqline;
}


void m68000_init(void)
{
	C68k_Init(&C68K);
	C68k_Set_Fetch(&C68K, 0x000000, 0x3fffff, (u32)memory_region_user1);
	C68k_Set_Fetch(&C68K, 0x660000, 0x663fff, (u32)cps2_ram);
	C68k_Set_Fetch(&C68K, 0x900000, 0x92ffff, (u32)cps1_gfxram);
	C68k_Set_Fetch(&C68K, 0xff0000, 0xffffff, (u32)cps1_ram);
	C68k_Set_ReadB(&C68K, m68000_read_memory_8);
	C68k_Set_ReadW(&C68K, m68000_read_memory_16);
	C68k_Set_WriteB(&C68K, m68000_write_memory_8);
	C68k_Set_WriteW(&C68K, m68000_write_memory_16);
	C68k_Set_ReadB_PC_Relative(&C68K, m68000_read_pcrelative_8);
	C68k_Set_ReadW_PC_Relative(&C68K, m68000_read_pcrelative_16);
	C68k_Set_IRQ_Callback(&C68K, m68000_irq_callback);

	if(memory_length_user1 == 0) {
    	C68k_Set_Fetch(&C68K, 0x000000, 0x3fffff, (u32)memory_region_cpu1);
    	C68k_Set_ReadB_PC_Relative(&C68K, m68000_read_memory_8);
    	C68k_Set_ReadW_PC_Relative(&C68K, m68000_read_memory_16);
    }
}


void m68000_reset(void)
{
	irq_state = CLEAR_LINE;
	C68k_Reset(&C68K);
}


void m68000_exit(void)
{
	/* nothing to do ? */
}


int m68000_execute(int cycles)
{
	return C68k_Exec(&C68K, cycles);
}


void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	irq_state = state;

	switch (state)
	{
	case CLEAR_LINE:
		C68k_Set_IRQ(&C68K, 0);
		return;

	case ASSERT_LINE:
		C68k_Set_IRQ(&C68K, irqline);
		return;

	default:
		C68k_Set_IRQ(&C68K, irqline);
		return;
	}
}


/*------------------------------------------------------
	Save/Load state
------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( m68000 )
{
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
}

STATE_LOAD( m68000 )
{
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
}

#endif /* SAVE_STATE */

#endif // !GP2X && !M68K
