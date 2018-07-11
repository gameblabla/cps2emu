/********************************************************************************/
/*                                                                              */
/* CZ80 macro file                                                              */
/* C Z80 emulator version 0.9                                                   */
/* Copyright 2004-2005 St—Èhane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

#define _SSOP(A,B) A##B
#define OP(A) _SSOP(OP,A)
#define OPCB(A) _SSOP(OPCB,A)
#define OPED(A) _SSOP(OPED,A)
#define OPXY(A) _SSOP(OPXY,A)
#define OPXYCB(A) _SSOP(OPXYCB,A)

#define GET_BYTE		(*(u8*)PC)
#define GET_BYTE_S		(*(s8*)PC)

#if CZ80_LITTLE_ENDIAN
#define GET_WORD		((*(u8*)(PC + 0)) | ((*(u8*)(PC + 1)) << 8))
#else
#define GET_WORD		((*(u8*)(PC + 1)) | ((*(u8*)(PC + 0)) << 8))
#endif

#define FETCH_BYTE		(*(u8*)PC++)
#define FETCH_BYTE_S	(*(s8*)PC++)

#define FETCH_WORD(A)	A = GET_WORD; PC += 2;

#define USE_CYCLES(A)		z80_ICount -= (A);
#define ADD_CYCLES(A)		z80_ICount += (A);

#define RET(A)				\
	z80_ICount -= A;		\
	goto Cz80_Exec;

#define SET_PC(A)				\
	CPU->BasePC = (u32) CPU->Fetch[(A) >> CZ80_FETCH_SFT];  \
	PC = (A) + CPU->BasePC;

#define READ_BYTE(A, D)				 \
	D = CPU->Read_Byte(A);

#if CZ80_LITTLE_ENDIAN
#define READ_WORD(A, D)				 \
	D = CPU->Read_Byte(A) | (CPU->Read_Byte((A) + 1) << 8);
#else
#define READ_WORD(A, D)				 \
	D = (CPU->Read_Byte(A) << 8) | CPU->Read_Byte((A) + 1);
#endif

#define READSX_BYTE(A, D)	D = (s32)(s8)CPU->Read_Byte(A);

#define WRITE_BYTE(A, D)	CPU->Write_Byte(A, D);

#if CZ80_LITTLE_ENDIAN
#define WRITE_WORD(A, D)				\
	CPU->Write_Byte(A, D);			  \
	CPU->Write_Byte((A) + 1, (D) >> 8);
#else
#define WRITE_WORD(A, D)				\
	CPU->Write_Byte(A, D);			  \
	CPU->Write_Byte((A) + 1, (D) >> 8);
#endif

#define PUSH_16(A)			\
	{						\
		u32 sp;				\
							\
		zSP -= 2;			\
		sp = zSP;			\
		WRITE_WORD(sp, A);	\
	}

#define POP_16(A)			\
	{						\
		u32 sp;				\
							\
		sp = zSP;			\
		READ_WORD(sp, A)	\
		zSP = sp + 2;		\
	}

#define IN(A, D)			D = CPU->IN_Port(A);
#define OUT(A, D)			CPU->OUT_Port(A, D);

#define CHECK_INT													\
	if (zIFF1)														\
	{																\
		u32 IntVect;												\
																	\
		CPU->HaltState = 0;											\
		zIFF1 = zIFF2 = 0;											\
		IntVect = CPU->Interrupt_Ack(0);							\
																	\
		if (zIM == 2)												\
		{															\
			PUSH_16(zRealPC)										\
			IntVect = (IntVect & 0xff) | (zI << 8);					\
			READ_WORD(IntVect, PC)									\
			SET_PC(PC)												\
			z80_ExtraCycles += 17;									\
		}															\
		else if (zIM == 1)											\
		{															\
			PUSH_16(zRealPC)										\
			SET_PC(0x38)											\
			z80_ExtraCycles += 13;									\
		}															\
		else														\
		{															\
			PUSH_16(zRealPC)										\
			PC = IntVect & 0x38;									\
			SET_PC(PC)												\
			z80_ExtraCycles += 13;									\
		}															\
	}
