/********************************************************************************/
/*                                                                              */
/* CZ80 (Z80 CPU emulator) version 0.9                                          */
/* Compiled with Dev-C++                                                        */
/* Copyright 2004-2005 St—Èhane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

#ifndef GP2X

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cz80.h"

#define CF					0x01
#define NF					0x02
#define PF					0x04
#define VF					PF
#define XF					0x08
#define HF					0x10
#define YF					0x20
#define ZF					0x40
#define SF					0x80

// include macro file
//////////////////////

#include "cz80macro.h"

// shared global variable
//////////////////////////

cz80_struc CZ80;
int z80_ICount;
static int z80_ExtraCycles;

static u8 *pzR8[8];
static union16 *pzR16[4];

static u8 SZ[256];            // zero and sign flags
static u8 SZP[256];           // zero, sign and parity flags
static u8 SZ_BIT[256];        // zero, sign and parity/overflow (=zero) flags for BIT opcode
static u8 SZHV_inc[256];      // zero, sign, half carry and overflow flags INC R8
static u8 SZHV_dec[256];      // zero, sign, half carry and overflow flags DEC R8
#if CZ80_BIG_FLAGS_ARRAY
static u8 SZHVC_add[2*256*256];
static u8 SZHVC_sub[2*256*256];
#endif

// core main functions
///////////////////////

void Cz80_Init(cz80_struc *CPU)
{
	u32 i, j, p;
#if CZ80_BIG_FLAGS_ARRAY
	int oldval, newval, val;
	u8 *padd, *padc, *psub, *psbc;
#endif

	memset(CPU, 0, sizeof(cz80_struc));

	// flags tables initialisation
	for (i = 0; i < 256; i++)
	{
		SZ[i] = i & (SF | YF | XF);
		if (!i) SZ[i] |= ZF;

		SZ_BIT[i] = i & (SF | YF | XF);
		if (!i) SZ_BIT[i] |= ZF | PF;

		for (j = 0, p = 0; j < 8; j++) if (i & (1 << j)) p++;
		SZP[i] = SZ[i];
		if (!(p & 1)) SZP[i] |= PF;

		SZHV_inc[i] = SZ[i];
		if(i == 0x80) SZHV_inc[i] |= VF;
		if((i & 0x0f) == 0x00) SZHV_inc[i] |= HF;

		SZHV_dec[i] = SZ[i] | NF;
		if (i == 0x7f) SZHV_dec[i] |= VF;
		if ((i & 0x0f) == 0x0f) SZHV_dec[i] |= HF;
	}

#if CZ80_BIG_FLAGS_ARRAY
	padd = &SZHVC_add[  0*256];
	padc = &SZHVC_add[256*256];
	psub = &SZHVC_sub[  0*256];
	psbc = &SZHVC_sub[256*256];

	for (oldval = 0; oldval < 256; oldval++)
	{
		for (newval = 0; newval < 256; newval++)
		{
			/* add or adc w/o carry set */
			val = newval - oldval;
			*padd = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
			*padd |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
			if ((newval & 0x0f) < (oldval & 0x0f)) *padd |= HF;
			if (newval < oldval ) *padd |= CF;
			if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padd |= VF;
			padd++;

			/* adc with carry set */
			val = newval - oldval - 1;
			*padc = (newval) ? ((newval & 0x80) ? SF : 0) : ZF;
			*padc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
			if ((newval & 0x0f) <= (oldval & 0x0f)) *padc |= HF;
			if (newval <= oldval) *padc |= CF;
			if ((val ^ oldval ^ 0x80) & (val ^ newval) & 0x80) *padc |= VF;
			padc++;

			/* cp, sub or sbc w/o carry set */
			val = oldval - newval;
			*psub = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
			*psub |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
			if ((newval & 0x0f) > (oldval & 0x0f)) *psub |= HF;
			if (newval > oldval) *psub |= CF;
			if ((val^oldval) & (oldval^newval) & 0x80) *psub |= VF;
			psub++;

			/* sbc with carry set */
			val = oldval - newval - 1;
			*psbc = NF | ((newval) ? ((newval & 0x80) ? SF : 0) : ZF);
			*psbc |= (newval & (YF | XF));	/* undocumented flag bits 5+3 */
			if ((newval & 0x0f) >= (oldval & 0x0f)) *psbc |= HF;
			if (newval >= oldval) *psbc |= CF;
			if ((val ^ oldval) & (oldval^newval) & 0x80) *psbc |= VF;
			psbc++;
		}
	}
#endif

	pzR8[0] = &zB;
	pzR8[1] = &zC;
	pzR8[2] = &zD;
	pzR8[3] = &zE;
	pzR8[4] = &zH;
	pzR8[5] = &zL;
	pzR8[6] = &zF;	// Ù•◊‚™Œ‘¥˘Íﬂæ£¨A™»Ï˝™ÏÙ™®
	pzR8[7] = &zA;	// Ù•◊‚™Œ‘¥˘Íﬂæ£¨F™»Ï˝™ÏÙ™®

	pzR16[0] = pzBC;
	pzR16[1] = pzDE;
	pzR16[2] = pzHL;
	pzR16[3] = pzAF;

	zIX = zIY = 0xffff;
	zF = ZF;
}

void Cz80_Reset(cz80_struc *CPU)
{
	zI  = 0;
	zR  = 0;
	zR2 = 0;

	z80_ICount = 0;
	z80_ExtraCycles = 0;

	Cz80_Set_PC(CPU, 0);
}

s32 Cz80_Exec(cz80_struc *cpu, s32 cycles)
{
#if CZ80_USE_JUMPTABLE
#include "cz80jmp.c"
#endif

	cz80_struc *CPU;
	u32 PC;
	u32 Opcode;
	u32 adr;
	u32 res;
	u32 val;
	int afterEI = 0;

	CPU = cpu;
	PC = CPU->PC;
	z80_ICount = cycles - z80_ExtraCycles;
	z80_ExtraCycles = 0;

	if (!CPU->HaltState)
	{
Cz80_Exec:
		if (z80_ICount > 0)
		{
			union16 *data = pzHL;
			Opcode = FETCH_BYTE;
#if CZ80_EMULATE_R_EXACTLY
			zR++;
#endif
			#include "cz80_op.c"
		}

		if (afterEI)
		{
			z80_ICount += z80_ExtraCycles;
			z80_ExtraCycles = 0;
			afterEI = 0;
Cz80_Check_Interrupt:
			CHECK_INT
			goto Cz80_Exec;
		}
	}
	else z80_ICount = 0;

Cz80_Exec_End:
	CPU->PC = PC;
	cycles -= z80_ICount;
#if (CZ80_EMULATE_R_EXACTLY == 0)
	zR = (zR + (cycles >> 2)) & 0x7f;
#endif

	return cycles;
}

void Cz80_Set_IRQ(cz80_struc *CPU, s32 vector)
{
	u32 PC = CPU->PC;

	CPU->IRQState = 1;
	CHECK_INT
	CPU->PC = PC;
}

void Cz80_Set_NMI(cz80_struc *CPU)
{
	zIFF1 = 0;
	z80_ExtraCycles += 11;
	CPU->IRQState = 0;
	CPU->HaltState = 0;

	PUSH_16(CPU->PC - CPU->BasePC)
	Cz80_Set_PC(CPU, 0x66);
}

void Cz80_Clear_IRQ(cz80_struc *CPU)
{
	CPU->IRQState = 0;
}

// setting core functions
//////////////////////////

void Cz80_Set_Fetch(cz80_struc *cpu, u32 low_adr, u32 high_adr, u32 fetch_adr)
{
	u32 i, j;

	i = low_adr >> CZ80_FETCH_SFT;
	j = high_adr >> CZ80_FETCH_SFT;
	fetch_adr -= i << CZ80_FETCH_SFT;
	while (i <= j) cpu->Fetch[i++] = (u8 *)fetch_adr;
}

void Cz80_Set_ReadB(cz80_struc *cpu, CZ80_READ8 *Func)
{
	cpu->Read_Byte = Func;
}

void Cz80_Set_WriteB(cz80_struc *cpu, CZ80_WRITE8 *Func)
{
	cpu->Write_Byte = Func;
}

void Cz80_Set_INPort(cz80_struc *cpu, CZ80_READ8 *Func)
{
	cpu->IN_Port = Func;
}

void Cz80_Set_OUTPort(cz80_struc *cpu, CZ80_WRITE8 *Func)
{
	cpu->OUT_Port = Func;
}

void Cz80_Set_IRQ_Callback(cz80_struc *cpu, CZ80_INT_CALLBACK *Func)
{
	cpu->Interrupt_Ack = Func;
}

// externals main functions
////////////////////////////

u32 Cz80_Get_PC(cz80_struc *CPU)
{
	u32 PC = CPU->PC;
	return zRealPC;
}

void Cz80_Set_PC(cz80_struc *cpu, u32 val)
{
	cpu->BasePC = (u32)cpu->Fetch[val >> CZ80_FETCH_SFT];
	cpu->PC = val + cpu->BasePC;
}

#endif // !GP2X
