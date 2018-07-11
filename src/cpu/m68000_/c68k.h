/*********************************************************************************
 * C68K.H :
 *
 * C68K include file
 *
 ********************************************************************************/

#ifndef _C68K_H_
#define _C68K_H_

#ifdef __cplusplus
extern "C" {
#endif

// setting
///////////

//#define C68K_BIG_ENDIAN

#define C68K_FETCH_BITS 4		// [4-12]   default = 8


// Compiler dependant defines
///////////////////////////////

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


// 68K core types definitions
//////////////////////////////

#define C68K_ADR_BITS	24

#define C68K_FETCH_SFT	(C68K_ADR_BITS - C68K_FETCH_BITS)
#define C68K_FETCH_BANK	(1 << C68K_FETCH_BITS)
#define C68K_FETCH_MASK	(C68K_FETCH_BANK - 1)

#define C68K_SR_C_SFT	8
#define C68K_SR_V_SFT	7
#define C68K_SR_Z_SFT	0
#define C68K_SR_N_SFT	7
#define C68K_SR_X_SFT	8

#define C68K_SR_S_SFT	13

#define C68K_SR_C		(1 << C68K_SR_C_SFT)
#define C68K_SR_V		(1 << C68K_SR_V_SFT)
#define C68K_SR_Z		0
#define C68K_SR_N		(1 << C68K_SR_N_SFT)
#define C68K_SR_X		(1 << C68K_SR_X_SFT)

#define C68K_SR_S		(1 << C68K_SR_S_SFT)

#define C68K_CCR_MASK	0x1F
#define C68K_SR_MASK	(0x2700 | C68K_CCR_MASK)

// exception defines taken from musashi core
#define C68K_RESET_EX					1
#define C68K_BUS_ERROR_EX				2
#define C68K_ADDRESS_ERROR_EX			3
#define C68K_ILLEGAL_INSTRUCTION_EX		4
#define C68K_ZERO_DIVIDE_EX				5
#define C68K_CHK_EX						6
#define C68K_TRAPV_EX					7
#define C68K_PRIVILEGE_VIOLATION_EX		8
#define C68K_TRACE_EX					9
#define C68K_1010_EX					10
#define C68K_1111_EX					11
#define C68K_FORMAT_ERROR_EX			14
#define C68K_UNINITIALIZED_INTERRUPT_EX 15
#define C68K_SPURIOUS_INTERRUPT_EX		24
#define C68K_INTERRUPT_AUTOVECTOR_EX	24
#define C68K_TRAP_BASE_EX				32

#define C68K_INT_ACK_AUTOVECTOR			-1

typedef u8 C68K_READ8(u32 adr);
typedef u16 C68K_READ16(u32 adr);
typedef void C68K_WRITE8(u32 adr, u8 data);
typedef void C68K_WRITE16(u32 adr, u16 data);

typedef s32  C68K_INT_CALLBACK(s32 level);
typedef void C68K_RESET_CALLBACK(void);

typedef struct
{
	u32 D[8];
	u32 A[8];

	u32 flag_C;
	u32 flag_V;
	u32 flag_Z;
	u32 flag_N;

	u32 flag_X;
	u32 flag_I;
	u32 flag_S;

	u32 USP;

	u32 PC;
	u32 BasePC;
	u32 HaltState;
	s32 IRQLine;

	C68K_READ8 *Read_Byte;
	C68K_READ16 *Read_Word;

	C68K_WRITE8 *Write_Byte;
	C68K_WRITE16 *Write_Word;

	C68K_READ8 *Read_Byte_PC_Relative;
	C68K_READ16 *Read_Word_PC_Relative;

	C68K_INT_CALLBACK *Interrupt_CallBack;
	C68K_RESET_CALLBACK *Reset_CallBack;

	u32 Fetch[C68K_FETCH_BANK];

} c68k_struc;


// 68K core var declaration
////////////////////////////

extern c68k_struc C68K;


// 68K core function declaration
/////////////////////////////////

void C68k_Init(c68k_struc *cpu);

void C68k_Reset(c68k_struc *cpu);

s32  C68k_Exec(c68k_struc *cpu, s32 cycle);

void C68k_Set_IRQ(c68k_struc *cpu, s32 level);

void C68k_Set_Fetch(c68k_struc *cpu, u32 low_adr, u32 high_adr, u32 fetch_adr);

void C68k_Set_ReadB(c68k_struc *cpu, C68K_READ8 *Func);
void C68k_Set_ReadW(c68k_struc *cpu, C68K_READ16 *Func);

void C68k_Set_ReadB_PC_Relative(c68k_struc *cpu, C68K_READ8 *Func);
void C68k_Set_ReadW_PC_Relative(c68k_struc *cpu, C68K_READ16 *Func);

void C68k_Set_WriteB(c68k_struc *cpu, C68K_WRITE8 *Func);
void C68k_Set_WriteW(c68k_struc *cpu, C68K_WRITE16 *Func);

void C68k_Set_IRQ_Callback(c68k_struc *cpu, C68K_INT_CALLBACK *int_cb);

u32  C68k_Get_PC(c68k_struc *cpu);
void C68k_Set_PC(c68k_struc *cpu, u32 val);

#ifdef __cplusplus
}
#endif

#endif  /* _C68K_H_ */
