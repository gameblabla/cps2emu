/*********************************************************************************
 *
 * C68K (68000 CPU emulator) version 0.80
 * Compiled with Dev-C++
 * Copyright 2003-2004 Stephane Dallongeville
 *
 ********************************************************************************/

#if !defined(GP2X) && !defined(M68K) && !defined(FAME)

#include <stdio.h>
#include <string.h>
#include "c68k.h"

// shared global variable
//////////////////////////

c68k_struc C68K;

// include macro file
//////////////////////

#include "c68kmacro.h"


static void C68k_ResetCallback(void);

// core main functions
///////////////////////

void C68k_Init(c68k_struc *cpu)
{
	memset(cpu, 0, sizeof(c68k_struc));

	cpu->Reset_CallBack = C68k_ResetCallback;

	C68k_Exec(NULL, 0);
}

void C68k_Reset(c68k_struc *cpu)
{
	c68k_struc *CPU = cpu;
	u32 initial_pc;

	memset(cpu, 0, (u32)(&(cpu->IRQLine)) - (u32)(&(cpu->D[0])));

	cpu->flag_I = 7;
	cpu->flag_S = C68K_SR_S;

	cpu->A[7]  = READ_PCREL_32(0);
	initial_pc = READ_PCREL_32(4);
	C68k_Set_PC(cpu, initial_pc);
}

/////////////////////////////////

void C68k_Set_IRQ(c68k_struc *cpu, s32 level)
{
	cpu->IRQLine = level;
	cpu->HaltState = 0;
}

// setting core functions
//////////////////////////

void C68k_Set_Fetch(c68k_struc *cpu, u32 low_adr, u32 high_adr, u32 fetch_adr)
{
	u32 i, j;

	i = (low_adr >> C68K_FETCH_SFT) & C68K_FETCH_MASK;
	j = (high_adr >> C68K_FETCH_SFT) & C68K_FETCH_MASK;
	fetch_adr -= i << C68K_FETCH_SFT;
	while (i <= j) cpu->Fetch[i++] = fetch_adr;
}

void C68k_Set_ReadB(c68k_struc *cpu, C68K_READ8 *Func)
{
	cpu->Read_Byte = Func;
	cpu->Read_Byte_PC_Relative = Func;
}

void C68k_Set_ReadW(c68k_struc *cpu, C68K_READ16 *Func)
{
	cpu->Read_Word = Func;
	cpu->Read_Word_PC_Relative = Func;
}

void C68k_Set_ReadB_PC_Relative(c68k_struc *cpu, C68K_READ8 *Func)
{
	cpu->Read_Byte_PC_Relative = Func;
}

void C68k_Set_ReadW_PC_Relative(c68k_struc *cpu, C68K_READ16 *Func)
{
	cpu->Read_Word_PC_Relative = Func;
}

void C68k_Set_WriteB(c68k_struc *cpu, C68K_WRITE8 *Func)
{
	cpu->Write_Byte = Func;
}

void C68k_Set_WriteW(c68k_struc *cpu, C68K_WRITE16 *Func)
{
	cpu->Write_Word = Func;
}

void C68k_Set_IRQ_Callback(c68k_struc *cpu, C68K_INT_CALLBACK *int_cb)
{
	cpu->Interrupt_CallBack = int_cb;
}

u32 C68k_Get_PC(c68k_struc *cpu)
{
	return cpu->PC - cpu->BasePC;
}

void C68k_Set_PC(c68k_struc *cpu, u32 val)
{
	cpu->BasePC = cpu->Fetch[(val >> C68K_FETCH_SFT) & C68K_FETCH_MASK];
	cpu->PC = val + cpu->BasePC;
}


static void C68k_ResetCallback(void)
{
}

#endif // !GP2X && !M68K
