#if !defined(GP2X) && !defined(M68K) && !defined(FAME)

#include "c68k.h"

// global variable
///////////////////

static void *JumpTable[0x10000];

int c68k_remaining_cycles;


// include macro file
//////////////////////

#include "c68kmacro.h"


// main exec function
//////////////////////

s32 C68k_Exec(c68k_struc *cpu, s32 cycles)
{
	if (cpu)
	{
		c68k_struc *CPU;
		u32 PC;
		u32 Opcode;
		u32 adr;
		u32 res;
		u32 src;
		u32 dst;

		CPU = cpu;
		PC = CPU->PC;

		c68k_remaining_cycles = cycles;

C68k_Check_Interrupt:
		CHECK_INT
		if (!CPU->HaltState)
		{

C68k_Exec_Next:
			if (c68k_remaining_cycles > 0)
			{
				Opcode = READ_IMM_16();
				PC += 2;
				goto *JumpTable[Opcode];

				#include "c68k_op.c"
			}
		}

		CPU->PC = PC;

		return cycles - c68k_remaining_cycles;
	}
	else
	{
		#include "c68k_ini.c"
	}

	return 0;
}

#endif // !GP2X && !M68K
