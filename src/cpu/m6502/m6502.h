/*****************************************************************************
 *
 *	 m6502.h
 *	 Portable 6502/65c02/6510 emulator interface
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _M6502_H
#define _M6502_H

#include "osd_cpu.h"

#define SUPP65C02	1		/* set to 1 to support the 65C02 opcodes */
#define SUPP6510	1		/* set to 1 to support the 6510 opcodes */

/* set to 1 to test cur_mrhard/cur_wmhard to avoid calls */
#define FAST_MEMORY 1

enum { M6502_A, M6502_X, M6502_Y, M6502_S, M6502_PC, M6502_P,
	M6502_EA, M6502_ZP, M6502_NMI_STATE, M6502_IRQ_STATE };

#define M6502_INT_NONE  0
#define M6502_INT_IRQ	1
#define M6502_INT_NMI	2

#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe

extern int m6502_ICount;				/* cycle count */

extern void m6502_reset (void *param);			/* Reset registers to the initial values */
extern void m6502_exit	(void); 				/* Shut down CPU core */
extern int	m6502_execute(int cycles);			/* Execute cycles - returns number of cycles actually run */
extern unsigned m6502_get_context (void *dst); /* Get registers, return context size */
extern void m6502_set_context (void *src); 	/* Set registers */
extern unsigned m6502_get_pc (void); 			/* Get program counter */
extern void m6502_set_pc (unsigned val); 		/* Set program counter */
extern unsigned m6502_get_sp (void); 			/* Get stack pointer */
extern void m6502_set_sp (unsigned val); 		/* Set stack pointer */
extern unsigned m6502_get_reg (int regnum);
extern void m6502_set_reg (int regnum, unsigned val);
extern void m6502_set_nmi_line(int state);
extern void m6502_set_irq_line(int irqline, int state);
extern void m6502_set_irq_callback(int (*callback)(int irqline));
extern void m6502_state_save(void *file);
extern void m6502_state_load(void *file);
extern const char *m6502_info(void *context, int regnum);

/****************************************************************************
 * The 65C02
 ****************************************************************************/
#define M65C02_A						M6502_A
#define M65C02_X						M6502_X
#define M65C02_Y						M6502_Y
#define M65C02_S						M6502_S
#define M65C02_PC						M6502_PC
#define M65C02_P						M6502_P
#define M65C02_EA						M6502_EA
#define M65C02_ZP						M6502_ZP
#define M65C02_NMI_STATE				M6502_NMI_STATE
#define M65C02_IRQ_STATE				M6502_IRQ_STATE

#define M65C02_INT_NONE 				M6502_INT_NONE
#define M65C02_INT_IRQ					M6502_INT_IRQ
#define M65C02_INT_NMI					M6502_INT_NMI

#define M65C02_NMI_VEC					M6502_NMI_VEC
#define M65C02_RST_VEC					M6502_RST_VEC
#define M65C02_IRQ_VEC					M6502_IRQ_VEC

#define m65c02_ICount					m6502_ICount

extern void m65c02_reset (void *param);
extern void m65c02_exit  (void);
extern int	m65c02_execute(int cycles);
extern unsigned m65c02_get_context (void *dst);
extern void m65c02_set_context (void *src);
extern unsigned m65c02_get_pc (void);
extern void m65c02_set_pc (unsigned val);
extern unsigned m65c02_get_sp (void);
extern void m65c02_set_sp (unsigned val);
extern unsigned m65c02_get_reg (int regnum);
extern void m65c02_set_reg (int regnum, unsigned val);
extern void m65c02_set_nmi_line(int state);
extern void m65c02_set_irq_line(int irqline, int state);
extern void m65c02_set_irq_callback(int (*callback)(int irqline));
extern void m65c02_state_save(void *file);
extern void m65c02_state_load(void *file);
extern const char *m65c02_info(void *context, int regnum);

/****************************************************************************
 * The 6510
 ****************************************************************************/
#define M6510_A 						M6502_A
#define M6510_X 						M6502_X
#define M6510_Y 						M6502_Y
#define M6510_S 						M6502_S
#define M6510_PC						M6502_PC
#define M6510_P 						M6502_P
#define M6510_EA						M6502_EA
#define M6510_ZP						M6502_ZP
#define M6510_NMI_STATE 				M6502_NMI_STATE
#define M6510_IRQ_STATE 				M6502_IRQ_STATE

#define M6510_INT_NONE					M6502_INT_NONE
#define M6510_INT_IRQ					M6502_INT_IRQ
#define M6510_INT_NMI					M6502_INT_NMI

#define M6510_NMI_VEC					M6502_NMI_VEC
#define M6510_RST_VEC					M6502_RST_VEC
#define M6510_IRQ_VEC					M6502_IRQ_VEC

#define m6510_ICount					m6502_ICount

extern void m6510_reset (void *param);
extern void m6510_exit	(void);
extern int	m6510_execute(int cycles);
extern unsigned m6510_get_context (void *dst);
extern void m6510_set_context (void *src);
extern unsigned m6510_get_pc (void);
extern void m6510_set_pc (unsigned val);
extern unsigned m6510_get_sp (void);
extern void m6510_set_sp (unsigned val);
extern unsigned m6510_get_reg (int regnum);
extern void m6510_set_reg (int regnum, unsigned val);
extern void m6510_set_nmi_line(int state);
extern void m6510_set_irq_line(int irqline, int state);
extern void m6510_set_irq_callback(int (*callback)(int irqline));
extern void m6510_state_save(void *file);
extern void m6510_state_load(void *file);
extern const char *m6510_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6502(char *buffer, int pc);
#endif

#endif /* _M6502_H */


