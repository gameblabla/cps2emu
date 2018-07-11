/*****************************************************************************/
/* FAME Fast and Accurate Motorola 68000 Emulation Core                      */
/* (c) 2005 Oscar Orallo Pelaez                                              */
/* Version: 1.24                                                             */
/* Date: 08-20-2005                                                          */
/* See FAME.HTML for documentation and license information                   */
/*****************************************************************************/

#ifdef FAME

#ifndef __FAME_H__
#define __FAME_H__

#include "include/osd_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif


/************************************/
/* General library defines          */
/************************************/

#ifndef M68K_OK
    #define M68K_OK 0
#endif

#ifndef M68K_RUNNING
    #define M68K_RUNNING 1
#endif

#ifndef M68K_NO_SUP_ADDR_SPACE
    #define M68K_NO_SUP_ADDR_SPACE 2
#endif

#ifndef M68K_INV_REG
    #define M68K_INV_REG -1
#endif

/* Hardware interrupt state */

#ifndef M68K_IRQ_LEVEL_ERROR
    #define M68K_IRQ_LEVEL_ERROR -1
#endif

#ifndef M68K_IRQ_INV_PARAMS
    #define M68K_IRQ_INV_PARAMS -2
#endif

/* Defines to specify hardware interrupt type */

#ifndef M68K_AUTOVECTORED_IRQ
    #define M68K_AUTOVECTORED_IRQ -1
#endif

#ifndef M68K_SPURIOUS_IRQ
    #define M68K_SPURIOUS_IRQ -2
#endif

/* Defines to specify address space */

#ifndef M68K_SUP_ADDR_SPACE
    #define M68K_SUP_ADDR_SPACE 0
#endif

#ifndef M68K_USER_ADDR_SPACE
    #define M68K_USER_ADDR_SPACE 2
#endif

#ifndef M68K_PROG_ADDR_SPACE
    #define M68K_PROG_ADDR_SPACE 0
#endif

#ifndef M68K_DATA_ADDR_SPACE
    #define M68K_DATA_ADDR_SPACE 1
#endif


/*******************/
/* Data definition */
/*******************/

typedef union
{
    u8 B;
    s8 SB;
} famec_union8;

typedef union
{
    u8 B;
    s8 SB;
	u16 W;
	s16 SW;
} famec_union16;

typedef union
{
	u8 B;
	s8 SB;
	u16 W;
	s16 SW;
	u32 D;
	s32 SD;
} famec_union32;

/* M68K registers */
typedef enum
{
      M68K_REG_D0=0,
      M68K_REG_D1,
      M68K_REG_D2,
      M68K_REG_D3,
      M68K_REG_D4,
      M68K_REG_D5,
      M68K_REG_D6,
      M68K_REG_D7,
      M68K_REG_A0,
      M68K_REG_A1,
      M68K_REG_A2,
      M68K_REG_A3,
      M68K_REG_A4,
      M68K_REG_A5,
      M68K_REG_A6,
      M68K_REG_A7,
      M68K_REG_ASP,
      M68K_REG_PC,
      M68K_REG_SR
} m68k_register;

/* The memory blocks must be in native (Motorola) format */
typedef struct
{
	u32 low_addr;
	u32 high_addr;
	u32 offset;
} M68K_PROGRAM;

/* The memory blocks must be in native (Motorola) format */
typedef struct
{
	u32 low_addr;
	u32 high_addr;
	void    *mem_handler;
	void    *data;
} M68K_DATA;

/* M68K CPU CONTEXT */
typedef struct
{
	M68K_PROGRAM *fetch;
	M68K_DATA *read_byte;
	M68K_DATA *read_word;
	M68K_DATA *write_byte;
	M68K_DATA *write_word;
	M68K_PROGRAM *sv_fetch;
	M68K_DATA *sv_read_byte;
	M68K_DATA *sv_read_word;
	M68K_DATA *sv_write_byte;
	M68K_DATA *sv_write_word;
	M68K_PROGRAM *user_fetch;
	M68K_DATA *user_read_byte;
	M68K_DATA *user_read_word;
	M68K_DATA *user_write_byte;
	M68K_DATA *user_write_word;
	void           (*reset_handler)(void);
	void           (*iack_handler)(u32 level);
	u32 *icust_handler;
	famec_union32   dreg[8];
	famec_union32   areg[8];
	u32 asp;
	u32  pc;
	u32 cycles_counter;
	u8  interrupts[8];
	u16 sr;
	u16 execinfo;
} M68K_CONTEXT;


/************************/
/* Function definition  */
/************************/

/* General purpose functions */
void     m68k_init(void);
u32 m68k_reset(void);
u32 m68k_emulate(s32 n);
u32 m68k_get_pc(void);
u32 m68k_get_cpu_state(void);
s32 m68k_fetch(u32 address, u32 memory_space);

/* Interrupt handling functions */
s32  m68k_raise_irq(s32 level, s32 vector);
s32  m68k_lower_irq(s32 level);
void m68k_burst_irq(s32 mask, s32 vector);
void m68k_set_irq_type(void *context, s32 type);
s32  m68k_get_irq_vector(s32 level);
s32  m68k_change_irq_vector(s32 level, s32 vector);

/* CPU context handling functions */
s32  m68k_get_context_size(void);
void m68k_get_context(void *context);
void m68k_set_context(void *context);
s32  m68k_get_register(m68k_register reg);
s32  m68k_set_register(m68k_register reg, u32 value);

/* Timing functions */
u32 m68k_get_cycles_counter(void);
u32 m68k_trip_cycles_counter(void);
u32 m68k_control_cycles_counter(s32 n);
void     m68k_release_timeslice(void);
void     m68k_add_cycles(s32 cycles);
void     m68k_release_cycles(s32 cycles);


#ifdef __cplusplus
}
#endif

#endif

#endif // !FAME
