/* cyclone interface */

#ifdef GP2X

#include "emumain.h"
#include "cpu/cyclone/Cyclone.h"

struct Cyclone MyCyclone;
static int total_cycles;
static int time_slice;
u32 cyclone_pc;
extern int current_line;

static u32 m68k_encrypt_start;
static u32 m68k_encrypt_end;
static u8  *m68k_decrypted_rom;

void m68000_set_encrypted_range(u32 start, u32 end, void *decrypted_rom)
{
	m68k_encrypt_start = start;
	m68k_encrypt_end   = end;
	m68k_decrypted_rom = (u8 *)decrypted_rom;
}

static int irq_state;

static void m68000_irq_callback(int irqline)
{
	if (irq_state == HOLD_LINE)
	{
		irq_state = CLEAR_LINE;
		MyCyclone.irq = 0;
	}
}

static unsigned int MyCheckPc(unsigned int pc) {
	pc -= MyCyclone.membase;
	
	switch(pc >> 22) {
        case 0x0:
            if(pc >= m68k_encrypt_start && pc <= m68k_encrypt_end)
                MyCyclone.membase = (int)m68k_decrypted_rom;                
            else
                MyCyclone.membase = (int)memory_region_cpu1;
            break;
        case 0x1:
            MyCyclone.membase = (int)cps2_ram - 0x660000;
            break;
        case 0x2:
            MyCyclone.membase = (int)cps1_gfxram - 0x900000;
            break;
        case 0x3:
            MyCyclone.membase = (int)cps1_ram - 0xff0000;
            break;
    }

	return MyCyclone.membase + pc;
}

static unsigned int MyCheckPc_norm(unsigned int pc) {
	pc -= MyCyclone.membase;
	
	switch(pc >> 22) {
        case 0x0:
            MyCyclone.membase = (int)memory_region_cpu1;
            break;
        case 0x1:
            MyCyclone.membase = (int)cps2_ram - 0x660000;
            break;
        case 0x2:
            MyCyclone.membase = (int)cps1_gfxram - 0x900000;
            break;
        case 0x3:
            MyCyclone.membase = (int)cps1_ram - 0xff0000;
            break;
    }

	return MyCyclone.membase + pc;
}

static unsigned int   MyRead32 (unsigned int a) {
	return (m68000_read_memory_16(a) << 16) | m68000_read_memory_16(a + 2);
}

static void MyWrite32(unsigned int a,unsigned int d) {
    m68000_write_memory_16(a, d >> 16);
    m68000_write_memory_16(a + 2, d & 0xffff);
}

static unsigned char  MyFetch8  (unsigned int offset) {
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return m68k_decrypted_rom[offset ^ 1];
	else
		return m68000_read_memory_8(offset);
}

static unsigned short MyFetch16 (unsigned int offset) {
	if (offset >= m68k_encrypt_start && offset <= m68k_encrypt_end)
		return *(u16 *)&m68k_decrypted_rom[offset];
	else
		return m68000_read_memory_16(offset);
}

static unsigned int MyFetch32 (unsigned int offset) {
	return (MyFetch16(offset) << 16) | MyFetch16(offset + 2);
}

void m68000_init(void)
{
	CycloneInit();
	memset(&MyCyclone, 0,sizeof(MyCyclone));

    if(memory_length_user1) {
    	MyCyclone.checkpc = MyCheckPc;
    	MyCyclone.fetch8  = MyFetch8;
    	MyCyclone.fetch16 = MyFetch16;
    	MyCyclone.fetch32 = MyFetch32;
    } else {
    	MyCyclone.checkpc = MyCheckPc_norm;
    	MyCyclone.fetch8  = m68000_read_memory_8;
    	MyCyclone.fetch16 = m68000_read_memory_16;
    	MyCyclone.fetch32 = MyRead32;
    }

	//MyCyclone.IrqCallback = m68000_irq_callback;

	MyCyclone.read8 = m68000_read_memory_8;
	MyCyclone.read16 = m68000_read_memory_16;
	MyCyclone.read32 = MyRead32;

	MyCyclone.write8 = m68000_write_memory_8;
	MyCyclone.write16 = m68000_write_memory_16;
	MyCyclone.write32 = MyWrite32;
}

void m68000_reset(void)
{
	MyCyclone.srh=0x27;
	MyCyclone.irq=0;
	MyCyclone.a[7]=MyCyclone.fetch32(0);
	
	MyCyclone.membase=0;
	MyCyclone.pc=MyCyclone.checkpc(MyCyclone.fetch32(4));
}

void m68000_exit(void)
{
	/* nothing to do ? */
}

int m68000_execute(int cycles)
{
	MyCyclone.cycles += cycles;
	CycloneRun(&MyCyclone);
	return -MyCyclone.cycles;
}

void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;

	irq_state = state;

	switch (state)
	{
	case CLEAR_LINE:
		MyCyclone.irq = 0;
		return;

	case ASSERT_LINE:
		MyCyclone.irq = irqline;
		return;

	default:
		MyCyclone.irq = irqline;
		return;
	}
}

#ifdef SAVE_STATE

STATE_SAVE( m68000 )
{
    u32 pc=MyCyclone.pc-MyCyclone.membase;

	state_save_byte(&MyCyclone, sizeof(MyCyclone));
	state_save_long(&pc, 1);
}

STATE_LOAD( m68000 )
{
	u32 pc;

	state_load_byte(&MyCyclone, sizeof(MyCyclone));
	state_load_long(&pc, 1);

    if(memory_length_user1) {
    	MyCyclone.checkpc = MyCheckPc;
    	MyCyclone.fetch8  = MyFetch8;
    	MyCyclone.fetch16 = MyFetch16;
    	MyCyclone.fetch32 = MyFetch32;
    } else {
    	MyCyclone.checkpc = MyCheckPc_norm;
    	MyCyclone.fetch8  = m68000_read_memory_8;
    	MyCyclone.fetch16 = m68000_read_memory_16;
    	MyCyclone.fetch32 = MyRead32;
    }

	MyCyclone.IrqCallback = m68000_irq_callback;

	MyCyclone.read8 = m68000_read_memory_8;
	MyCyclone.read16 = m68000_read_memory_16;
	MyCyclone.read32 = MyRead32;

	MyCyclone.write8 = m68000_write_memory_8;
	MyCyclone.write16 = m68000_write_memory_16;
	MyCyclone.write32 = MyWrite32;

    MyCyclone.membase=0;
    MyCyclone.pc=MyCyclone.checkpc(pc);
}

#endif /* SAVE_STATE */

#endif // GP2X
