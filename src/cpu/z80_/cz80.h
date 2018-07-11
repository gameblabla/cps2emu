/********************************************************************************/
/*                                                                              */
/* CZ80 include file                                                            */
/* C Z80 emulator version 0.1                                                   */
/* Copyright 2004 St—Èhane Dallongeville                                        */
/*                                                                              */
/********************************************************************************/

#ifndef _CZ80_H_
#define _CZ80_H_

#ifdef __cplusplus
extern "C" {
#endif


/******************************/
/* Compiler dependant defines */
/******************************/

#ifndef u8
#define u8	unsigned char
#endif

#ifndef s8
#define s8	char
#endif

#ifndef u16
#define u16	unsigned short
#endif

#ifndef s16
#define s16	short
#endif

#ifndef u32
#define u32	unsigned int
#endif

#ifndef s32
#define s32	int
#endif

/*************************************/
/* Z80 core Structures & definitions */
/*************************************/

#define CZ80_FETCH_BITS			4   // [4-12]   default = 8

#define CZ80_FETCH_SFT			(16 - CZ80_FETCH_BITS)
#define CZ80_FETCH_BANK			(1 << CZ80_FETCH_BITS)

#define CZ80_LITTLE_ENDIAN		1
#define CZ80_USE_JUMPTABLE		1
#define CZ80_BIG_FLAGS_ARRAY	1
#define CZ80_EMULATE_R_EXACTLY	0

#define zR8(A)		(*pzR8[A])
#define zR16(A)		(pzR16[A]->W)

#define pzAF		&(CPU->AF)
#define zAF			CPU->AF.W
#define zlAF		CPU->AF.B.L
#define zhAF		CPU->AF.B.H
#define zA			zhAF
#define zF			zlAF

#define pzBC		&(CPU->BC)
#define zBC			CPU->BC.W
#define zlBC		CPU->BC.B.L
#define zhBC		CPU->BC.B.H
#define zB			zhBC
#define zC			zlBC

#define pzDE		&(CPU->DE)
#define zDE			CPU->DE.W
#define zlDE		CPU->DE.B.L
#define zhDE		CPU->DE.B.H
#define zD			zhDE
#define zE			zlDE

#define pzHL		&(CPU->HL)
#define zHL			CPU->HL.W
#define zlHL		CPU->HL.B.L
#define zhHL		CPU->HL.B.H
#define zH			zhHL
#define zL			zlHL

#define zAF2		CPU->AF2.W
#define zlAF2		CPU->AF2.B.L
#define zhAF2		CPU->AF2.B.H
#define zA2			zhAF2
#define zF2			zlAF2

#define zBC2		CPU->BC2.W
#define zDE2		CPU->DE2.W
#define zHL2		CPU->HL2.W

#define pzIX		&(CPU->IX)
#define zIX			CPU->IX.W
#define zlIX		CPU->IX.B.L
#define zhIX		CPU->IX.B.H

#define pzIY		&(CPU->IY)
#define zIY			CPU->IY.W
#define zlIY		CPU->IY.B.L
#define zhIY		CPU->IY.B.H

#define pzSP		&(CPU->SP)
#define zSP			CPU->SP.W
#define zlSP		CPU->SP.B.L
#define zhSP		CPU->SP.B.H

#define zRealPC		(PC - CPU->BasePC)
#define zPC			PC

#define zI			CPU->I
#define zIM			CPU->IM

#define zwR			CPU->R.W
#define zR1			CPU->R.B.L
#define zR2			CPU->R.B.H
#define zR			zR1

#define zIFF		CPU->IFF.W
#define zIFF1		CPU->IFF.B.L
#define zIFF2		CPU->IFF.B.H

#define CZ80_SF_SFT	 7
#define CZ80_ZF_SFT	 6
#define CZ80_YF_SFT	 5
#define CZ80_HF_SFT	 4
#define CZ80_XF_SFT	 3
#define CZ80_PF_SFT	 2
#define CZ80_VF_SFT	 2
#define CZ80_NF_SFT	 1
#define CZ80_CF_SFT	 0

#define CZ80_SF		(1 << CZ80_SF_SFT)
#define CZ80_ZF		(1 << CZ80_ZF_SFT)
#define CZ80_YF		(1 << CZ80_YF_SFT)
#define CZ80_HF		(1 << CZ80_HF_SFT)
#define CZ80_XF		(1 << CZ80_XF_SFT)
#define CZ80_PF		(1 << CZ80_PF_SFT)
#define CZ80_VF		(1 << CZ80_VF_SFT)
#define CZ80_NF		(1 << CZ80_NF_SFT)
#define CZ80_CF		(1 << CZ80_CF_SFT)

#define CZ80_IFF_SFT	CZ80_PF_SFT
#define CZ80_IFF		CZ80_PF

#define CZ80_HAS_INT	CZ80_IFF
#define CZ80_HAS_NMI	0x08

#define CZ80_RUNNING	0x10
#define CZ80_HALTED	 	0x20
#define CZ80_FAULTED	0x80
#define CZ80_DISABLE	0x40


typedef u8 CZ80_READ8(u32 adr);
typedef void CZ80_WRITE8(u32 adr, u8 data);

typedef s32 CZ80_INT_CALLBACK(s32 param);

typedef union
{
	struct
	{
#if CZ80_LITTLE_ENDIAN
		u8 L;
		u8 H;
#else
		u8 H;
		u8 L;
#endif
	} B;
	u16 W;
} union16;

typedef struct
{
	union
	{
		u8 r8[8];
		union16 r16[4];
		struct
		{
			union16 BC;		 // 32 bytes aligned
			union16 DE;
			union16 HL;
			union16 AF;
		};
	};

	union16 IX;
	union16 IY;
	union16 SP;
	u32	 PC;

	union16 BC2;
	union16 DE2;
	union16 HL2;
	union16 AF2;

	union16 R;
	union16 IFF;

	u8 I;
	u8 IM;
	u8 IRQState;
	u8 HaltState;

	u32 BasePC;

	CZ80_READ8 *Read_Byte;
	CZ80_WRITE8 *Write_Byte;

	CZ80_READ8 *IN_Port;
	CZ80_WRITE8 *OUT_Port;

	CZ80_INT_CALLBACK *Interrupt_Ack;

	u8 *Fetch[CZ80_FETCH_BANK];
} cz80_struc;


/*************************/
/* Publics Z80 variables */
/*************************/

extern cz80_struc CZ80;
extern int z80_ICount;

/*************************/
/* Publics Z80 functions */
/*************************/

void Cz80_Init(cz80_struc *cpu);
void Cz80_Reset(cz80_struc *cpu);

void Cz80_Set_Fetch(cz80_struc *cpu, u32 low_adr, u32 high_adr, u32 fetch_adr);

void Cz80_Set_ReadB(cz80_struc *cpu, CZ80_READ8 *Func);
void Cz80_Set_WriteB(cz80_struc *cpu, CZ80_WRITE8 *Func);

void Cz80_Set_INPort(cz80_struc *cpu, CZ80_READ8 *Func);
void Cz80_Set_OUTPort(cz80_struc *cpu, CZ80_WRITE8 *Func);

void Cz80_Set_IRQ_Callback(cz80_struc *cpu, CZ80_INT_CALLBACK *Func);

s32  Cz80_Exec(cz80_struc *cpu, s32 cycles);

void Cz80_Set_IRQ(cz80_struc *cpu, s32 vector);
void Cz80_Set_NMI(cz80_struc *cpu);
void Cz80_Clear_IRQ(cz80_struc *cpu);

u32  Cz80_Get_PC(cz80_struc *cpu);
void Cz80_Set_PC(cz80_struc *cpu, u32 value);

#ifdef __cplusplus
};
#endif

#endif  // _CZ80_H_

