/*** TMS34010: Portable TMS34010 emulator ***********************************

	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
	 originally based on code by Aaron Giles

	Public include file

*****************************************************************************/

#ifndef _TMS34010_H
#define _TMS34010_H

#include "osd_cpu.h"

enum { TMS34010_PC, TMS34010_SP, TMS34010_ST,
	TMS34010_A0, TMS34010_A1, TMS34010_A2, TMS34010_A3,
	TMS34010_A4, TMS34010_A5, TMS34010_A6, TMS34010_A7,
	TMS34010_A8, TMS34010_A9, TMS34010_A10, TMS34010_A11,
	TMS34010_A12, TMS34010_A13, TMS34010_A14,
	TMS34010_B0, TMS34010_B1, TMS34010_B2, TMS34010_B3,
	TMS34010_B4, TMS34010_B5, TMS34010_B6, TMS34010_B7,
	TMS34010_B8, TMS34010_B9, TMS34010_B10, TMS34010_B11,
	TMS34010_B12, TMS34010_B13, TMS34010_B14 };

/* average instruction time in cycles */
#define TMS34010_AVGCYCLES 2

/* Interrupt Types that can be generated by outside sources */
#define TMS34010_INT_NONE	0x0000
#define TMS34010_INT1		0x0002	/* External Interrupt 1 */
#define TMS34010_INT2		0x0004	/* External Interrupt 2 */

/* PUBLIC FUNCTIONS */
extern void tms34010_reset(void *param);
extern void tms34010_exit(void);
extern int	tms34010_execute(int cycles);
extern unsigned tms34010_get_context(void *dst);
extern void tms34010_set_context(void *src);
extern unsigned tms34010_get_pc(void);
extern void tms34010_set_pc(unsigned val);
extern unsigned tms34010_get_sp(void);
extern void tms34010_set_sp(unsigned val);
extern unsigned tms34010_get_reg(int regnum);
extern void tms34010_set_reg(int regnum, unsigned val);
extern void tms34010_set_nmi_line(int linestate);
extern void tms34010_set_irq_line(int irqline, int linestate);
extern void tms34010_set_irq_callback(int (*callback)(int irqline));
extern void tms34010_internal_interrupt(int type);
extern const char *tms34010_info(void *context, int regnum);

extern void TMS34010_State_Save(int cpunum, void *f);
extern void TMS34010_State_Load(int cpunum, void *f);

void TMS34010_HSTADRL_w (int offset, int data);
void TMS34010_HSTADRH_w (int offset, int data);
void TMS34010_HSTDATA_w (int offset, int data);
int  TMS34010_HSTDATA_r (int offset);
void TMS34010_HSTCTLH_w (int offset, int data);

/* Sets functions to read/write shift register */
void TMS34010_set_shiftreg_functions(int cpu,
									 void (*to_shiftreg  )(UINT32, UINT16*),
									 void (*from_shiftreg)(UINT32, UINT16*));

/* Sets functions to read/write shift register */
void TMS34010_set_stack_base(int cpu, UINT8* stackbase, UINT32 stackoffs);

/* Writes to the 34010 io */
void TMS34010_io_register_w(int offset, int data);

/* Reads from the 34010 io */
int TMS34010_io_register_r(int offset);

/* Checks whether the display is inhibited */
int TMS34010_io_display_blanked(int cpu);

int TMS34010_get_DPYSTRT(int cpu);

/* PUBLIC GLOBALS */
extern int tms34010_ICount;


/* Use this macro in the memory definitions to specify bit-based addresses */
#define TOBYTE(bitaddr) ((UINT32) (bitaddr)>>3)

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm34010 (unsigned char *pBase, char *buff, int _pc);
#endif

#endif /* _TMS34010_H */
