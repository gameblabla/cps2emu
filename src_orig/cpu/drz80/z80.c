/* drz80 interface */

#ifdef GP2X

#include "emumain.h"
#include "cpu/drz80/DrZ80.h"

struct DrZ80 mydrz80;

unsigned int drz80_rebasePC(unsigned short address)
{
    mydrz80.Z80PC_BASE = (unsigned int)memory_region_cpu2;
	mydrz80.Z80PC = mydrz80.Z80PC_BASE + address;
    return mydrz80.Z80PC;
}

unsigned int drz80_rebaseSP(unsigned short address)
{
    mydrz80.Z80SP_BASE = (unsigned int)memory_region_cpu2;
	mydrz80.Z80SP = mydrz80.Z80SP_BASE + address;
	return mydrz80.Z80SP;
}

unsigned short drz80_read16(unsigned short address) {
    return z80_read_memory_8(address) | (z80_read_memory_8(address + 1) << 8);
}

void drz80_write16(unsigned short data,unsigned short address) {
    z80_write_memory_8(data & 0xFF,address);
    z80_write_memory_8(data >> 8,address + 1);
}

static u8 irq_state;

void z80_irq_callback(void)
{
	if (irq_state == HOLD_LINE)
	{
		irq_state = CLEAR_LINE;
		mydrz80.Z80_IRQ = 0x00;
	}
}

INLINE void z80_assert_irq(int irqline)
{
	if (irqline == IRQ_LINE_NMI)
    	mydrz80.Z80_IRQ |= 0x02;
	else
    	mydrz80.Z80_IRQ |= 0x1;
}


INLINE void z80_clear_irq(int irqline)
{
	mydrz80.Z80_IRQ &= ~0x1;
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

void z80_init(void)
{
    memset (&mydrz80, 0, sizeof(mydrz80));

    mydrz80.z80_rebasePC = drz80_rebasePC;
    mydrz80.z80_rebaseSP = drz80_rebaseSP;
    mydrz80.z80_read8    = z80_read_memory_8;
    mydrz80.z80_read16   = drz80_read16;
    mydrz80.z80_write8   = z80_write_memory_8;
    mydrz80.z80_write16  = drz80_write16;
    mydrz80.z80_irq_callback = z80_irq_callback;

    mydrz80.Z80A = 0x00 <<24;
    mydrz80.Z80F = (1<<2); /* set ZFlag */
    mydrz80.Z80BC = 0x0000 <<16;
    mydrz80.Z80DE = 0x0000 <<16;
    mydrz80.Z80HL = 0x0000 <<16;
    mydrz80.Z80A2 = 0x00 <<24;
    mydrz80.Z80F2 = 1<<2;  /* set ZFlag */
    mydrz80.Z80BC2 = 0x0000 <<16;
    mydrz80.Z80DE2 = 0x0000 <<16;
    mydrz80.Z80HL2 = 0x0000 <<16;
    mydrz80.Z80IX = 0xFFFF;// <<16;
    mydrz80.Z80IY = 0xFFFF;// <<16;
    mydrz80.Z80I = 0x00;
    mydrz80.Z80IM = 0x01;
    mydrz80.Z80_IRQ = 0x00;
    mydrz80.Z80IF = 0x00;
    mydrz80.Z80PC=mydrz80.z80_rebasePC(0);
    mydrz80.Z80SP=mydrz80.z80_rebaseSP(0xffff);/*0xf000;*/
}

void z80_reset(void)
{
    z80_init();
}

int z80_execute(int cycles)
{
    DrZ80Run(&mydrz80, cycles);
	return 0;
}

#ifdef SAVE_STATE
void state_save_z80(FILE *fp)
{
    u32 pc = mydrz80.Z80PC - mydrz80.Z80PC_BASE;
    u32 sp = mydrz80.Z80SP - mydrz80.Z80SP_BASE;
    
    state_save_byte(&mydrz80, sizeof(mydrz80));
    state_save_long(&pc, 1);
    state_save_long(&sp, 1);
}

void state_load_z80(FILE *fp)
{
	u32 pc, sp;

    state_load_byte(&mydrz80, sizeof(mydrz80));
    state_load_long(&pc, 1);
    state_load_long(&sp, 1);

    mydrz80.z80_rebasePC = drz80_rebasePC;
    mydrz80.z80_rebaseSP = drz80_rebaseSP;
    mydrz80.z80_read8    = z80_read_memory_8;
    mydrz80.z80_read16   = drz80_read16;
    mydrz80.z80_write8   = z80_write_memory_8;
    mydrz80.z80_write16  = drz80_write16;
    mydrz80.z80_irq_callback = z80_irq_callback;
    mydrz80.Z80PC = mydrz80.z80_rebasePC(pc);
    mydrz80.Z80SP = mydrz80.z80_rebaseSP(sp);
}
#endif

#endif // GP2X
