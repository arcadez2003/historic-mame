/*****************************************************************************
 *
 *	 m6502.c
 *	 Portable 6502/65c02/6510 emulator V1.1
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

#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "osd_dbg.h"
#include "m6502.h"
#include "m6502ops.h"

extern FILE * errorlog;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

extern int previouspc;

#define M6502_PLAIN 0		/* set M6502_Type to this for a plain 6502 emulation */

#if SUPP65C02
#define M6502_65C02 1       /* set M6502_Type to this for a 65C02 emulation */
#endif

#if SUPP6510
#define M6502_6510  2       /* set M6502_Type to this for a 6510 emulation */
#endif

int m6502_type = 0;
int m6502_ICount = 0;

/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
    UINT8   cpu_type;       /* currently selected cpu sub type */
    void    (**insn)(void); /* pointer to the function pointer table */
    PAIR    pc;             /* program counter */
    PAIR    sp;             /* stack pointer (always 100 - 1FF) */
    PAIR    zp;             /* zero page address */
    PAIR    ea;             /* effective address */
    UINT8   a;              /* Accumulator */
    UINT8   x;              /* X index register */
    UINT8   y;              /* Y index register */
    UINT8   p;              /* Processor status */
    UINT8   pending_interrupt; /* nonzero if a NMI or IRQ is pending */
    UINT8   after_cli;      /* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	UINT8	irq_state;
    int     (*irq_callback)(int irqline);   /* IRQ callback */
}   m6502_Regs;

static m6502_Regs m6502;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "tbl6502.c"
#include "tbl65c02.c"
#include "tbl6510.c"

/*****************************************************************************
 *
 *		6502 CPU interface functions
 *
 *****************************************************************************/

void m6502_reset(void *param)
{
	/* wipe out the m6502 structure */
    memset(&m6502, 0, sizeof(m6502_Regs));

	m6502.cpu_type = m6502_type;

	switch (m6502.cpu_type)
	{
#if SUPP65C02
		case M6502_65C02:
			m6502.insn = insn65c02;
			break;
#endif
#if SUPP6510
		case M6502_6510:
			m6502.insn = insn6510;
			break;
#endif
		default:
			m6502.insn = insn6502;
			break;
	}

	/* set T, I and Z flags */
	P = F_T | F_I | F_Z;

    /* stack starts at 0x01ff */
	m6502.sp.d = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(M6502_RST_VEC);
	PCH = RDMEM(M6502_RST_VEC+1);
	change_pc16(PCD);

    m6502.pending_interrupt = 0;
    m6502.nmi_state = 0;
    m6502.irq_state = 0;
}

void m6502_exit(void)
{
	/* nothing to do yet */
}

unsigned m6502_get_context (void *dst)
{
	if( dst )
		*(m6502_Regs*)dst = m6502;
	return sizeof(m6502_Regs);
}

void m6502_set_context (void *src)
{
	if( src )
		m6502 = *(m6502_Regs*)src;
	change_pc(PCD);
}

unsigned m6502_get_pc (void)
{
	return PCD;
}

void m6502_set_pc (unsigned val)
{
	PCW = val;
	change_pc(PCD);
}

unsigned m6502_get_sp (void)
{
	return S;
}

void m6502_set_sp (unsigned val)
{
	S = val;
}

unsigned m6502_get_reg (int regnum)
{
	switch( regnum )
	{
		case M6502_A: return m6502.a;
		case M6502_X: return m6502.x;
		case M6502_Y: return m6502.y;
		case M6502_S: return m6502.sp.b.l;
		case M6502_PC: return m6502.pc.w.l;
		case M6502_P: return m6502.p;
		case M6502_EA: return m6502.ea.w.l;
		case M6502_ZP: return m6502.zp.w.l;
		case M6502_NMI_STATE: return m6502.nmi_state;
		case M6502_IRQ_STATE: return m6502.irq_state;
	}
	return 0;
}

void m6502_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case M6502_A: m6502.a = val; break;
		case M6502_X: m6502.x = val; break;
		case M6502_Y: m6502.y = val; break;
		case M6502_S: m6502.sp.b.l = val; break;
		case M6502_PC: m6502.pc.w.l = val; break;
		case M6502_P: m6502.p = val; break;
		case M6502_EA: m6502.ea.w.l = val; break;
		case M6502_ZP: m6502.zp.w.l = val; break;
		case M6502_NMI_STATE: m6502.nmi_state = val; break;
		case M6502_IRQ_STATE: m6502.irq_state = val; break;
    }
}

INLINE void take_nmi(void)
{
	EAD = M6502_NMI_VEC;
	m6502_ICount -= 7;
	PUSH(PCH);
	PUSH(PCL);
	PUSH(P & ~F_B);
	P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
	PCL = RDMEM(EAD);
	PCH = RDMEM(EAD+1);
	LOG((errorlog,"M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
    m6502.pending_interrupt &= ~M6502_INT_NMI;
    change_pc16(PCD);
}

INLINE void take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M6502_IRQ_VEC;
		m6502_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG((errorlog,"M6502#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m6502.irq_callback) (*m6502.irq_callback)(0);
	    change_pc16(PCD);
	}

	m6502.pending_interrupt &= ~M6502_INT_IRQ;
}

int m6502_execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc16(PCD);

	do
	{
#ifdef  MAME_DEBUG
		if (mame_debug) MAME_Debug();
#endif
		previouspc = PCW;

		(*m6502.insn[RDOP()])();

		if (m6502.pending_interrupt & M6502_INT_NMI)
            take_nmi();
        /* check if the I flag was just reset (interrupts enabled) */
		if (m6502.after_cli)
		{
			LOG((errorlog,"M6502#%d after_cli was >0", cpu_getactivecpu()));
            m6502.after_cli = 0;
			if (m6502.irq_state != CLEAR_LINE)
			{
				LOG((errorlog,": irq line is asserted: set pending IRQ\n"));
                m6502.pending_interrupt |= M6502_INT_IRQ;
			}
			else
			{
				LOG((errorlog,": irq line is clear\n"));
            }
		}
		else if (m6502.pending_interrupt)
			take_irq();

    } while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

void m6502_set_nmi_line(int state)
{
	if (m6502.nmi_state == state) return;
	m6502.nmi_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
        m6502.pending_interrupt |= M6502_INT_NMI;
	}
}

void m6502_set_irq_line(int irqline, int state)
{
	m6502.irq_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
		m6502.pending_interrupt |= M6502_INT_IRQ;
	}
}

void m6502_set_irq_callback(int (*callback)(int))
{
	m6502.irq_callback = callback;
}

void m6502_state_save(void *file)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT8(file,"m6502",cpu,"TYPE",&m6502.cpu_type,1);
	/* insn is set at restore since it's a pointer */
	state_save_UINT16(file,"m6502",cpu,"PC",&m6502.pc.w.l,2);
	state_save_UINT16(file,"m6502",cpu,"SP",&m6502.sp.w.l,2);
	state_save_UINT8(file,"m6502",cpu,"A",&m6502.a,1);
	state_save_UINT8(file,"m6502",cpu,"X",&m6502.x,1);
	state_save_UINT8(file,"m6502",cpu,"Y",&m6502.y,1);
	state_save_UINT8(file,"m6502",cpu,"P",&m6502.p,1);
	state_save_UINT8(file,"m6502",cpu,"PENDING",&m6502.pending_interrupt,1);
	state_save_UINT8(file,"m6502",cpu,"AFTER_CLI",&m6502.after_cli,1);
	state_save_UINT8(file,"m6502",cpu,"NMI_STATE",&m6502.nmi_state,1);
	state_save_UINT8(file,"m6502",cpu,"IRQ_STATE",&m6502.irq_state,1);
}

void m6502_state_load(void *file)
{
	int cpu = cpu_getactivecpu();
    state_load_UINT8(file,"m6502",cpu,"TYPE",&m6502.cpu_type,1);
	switch (m6502.cpu_type)
	{
#if SUPP65C02
		case M6502_65C02:
			m6502.insn = insn65c02;
			break;
#endif
#if SUPP6510
		case M6502_6510:
			m6502.insn = insn6510;
			break;
#endif
		default:
			m6502.insn = insn6502;
			break;
	}
	/* insn is set at restore since it's a pointer */
	state_load_UINT16(file,"m6502",cpu,"PC",&m6502.pc.w.l,2);
	state_load_UINT16(file,"m6502",cpu,"SP",&m6502.sp.w.l,2);
	state_load_UINT8(file,"m6502",cpu,"A",&m6502.a,1);
	state_load_UINT8(file,"m6502",cpu,"X",&m6502.x,1);
	state_load_UINT8(file,"m6502",cpu,"Y",&m6502.y,1);
	state_load_UINT8(file,"m6502",cpu,"P",&m6502.p,1);
	state_load_UINT8(file,"m6502",cpu,"PENDING",&m6502.pending_interrupt,1);
	state_load_UINT8(file,"m6502",cpu,"AFTER_CLI",&m6502.after_cli,1);
	state_load_UINT8(file,"m6502",cpu,"NMI_STATE",&m6502.nmi_state,1);
	state_load_UINT8(file,"m6502",cpu,"IRQ_STATE",&m6502.irq_state,1);
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m6502_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6502_Regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &m6502;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "M6502";
		case CPU_INFO_FAMILY: return "Motorola 6502";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1998 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->pc.w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%03X", r->sp.w.l); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM: r->pc.w.l += Dasm6502(buffer[which], r->pc.w.l); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "$%02x", ROM[r->pc.w.l]); r->pc.w.l++; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->p & 0x80 ? 'N':'.',
				r->p & 0x40 ? 'V':'.',
				r->p & 0x20 ? 'R':'.',
				r->p & 0x10 ? 'B':'.',
				r->p & 0x08 ? 'D':'.',
				r->p & 0x04 ? 'I':'.',
				r->p & 0x02 ? 'Z':'.',
				r->p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_REG+M6502_A: sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+M6502_X: sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+M6502_Y: sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+M6502_S: sprintf(buffer[which], "S:%02X", r->sp.b.l); break;
		case CPU_INFO_REG+M6502_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+M6502_P: sprintf(buffer[which], "P:%02X", r->p); break;
		case CPU_INFO_REG+M6502_EA: sprintf(buffer[which], "EA:%04X", r->ea.w.l); break;
		case CPU_INFO_REG+M6502_ZP: sprintf(buffer[which], "ZP:%03X", r->zp.w.l); break;
		case CPU_INFO_REG+M6502_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+M6502_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
	}
	return buffer[which];
}

/****************************************************************************
 * 65C02 section
 ****************************************************************************/

void m65c02_reset (void *param)
{
	m6502_type = M6502_65C02;
	m6502_reset(param);
}
void m65c02_exit  (void) { m6502_exit(); }
int  m65c02_execute(int cycles) { return m6502_execute(cycles); }
unsigned m65c02_get_context (void *dst) { return m6502_get_context(dst); }
void m65c02_set_context (void *src) { m6502_set_context(src); }
unsigned m65c02_get_pc (void) { return m6502_get_pc(); }
void m65c02_set_pc (unsigned val) { m6502_set_pc(val); }
unsigned m65c02_get_sp (void) { return m6502_get_sp(); }
void m65c02_set_sp (unsigned val) { m6502_set_sp(val); }
unsigned m65c02_get_reg (int regnum) { return m6502_get_reg(regnum); }
void m65c02_set_reg (int regnum, unsigned val) { m6502_set_reg(regnum,val); }
void m65c02_set_nmi_line(int state) { m6502_set_nmi_line(state); }
void m65c02_set_irq_line(int irqline, int state) { m6502_set_irq_line(irqline,state); }
void m65c02_set_irq_callback(int (*callback)(int irqline)) { m6502_set_irq_callback(callback); }
void m65c02_state_save(void *file) { m6502_state_save(file); }
void m65c02_state_load(void *file) { m6502_state_load(file); }
const char *m65c02_info(void *context, int regnum)
{
	switch( regnum )
    {
		case CPU_INFO_NAME: return "M65C02";
		case CPU_INFO_VERSION: return "1.1";
	}
	return m6502_info(context,regnum);
}

/****************************************************************************
 * 6510 section
 ****************************************************************************/

void m6510_reset (void *param)
{
	m6502_type = M6502_6510;
	m6502_reset(param);
}
void m6510_exit  (void) { m6502_exit(); }
int  m6510_execute(int cycles) { return m6502_execute(cycles); }
unsigned m6510_get_context (void *dst) { return m6502_get_context(dst); }
void m6510_set_context (void *src) { m6502_set_context(src); }
unsigned m6510_get_pc (void) { return m6502_get_pc(); }
void m6510_set_pc (unsigned val) { m6502_set_pc(val); }
unsigned m6510_get_sp (void) { return m6502_get_sp(); }
void m6510_set_sp (unsigned val) { m6502_set_sp(val); }
unsigned m6510_get_reg (int regnum) { return m6502_get_reg(regnum); }
void m6510_set_reg (int regnum, unsigned val) { m6502_set_reg(regnum,val); }
void m6510_set_nmi_line(int state) { m6502_set_nmi_line(state); }
void m6510_set_irq_line(int irqline, int state) { m6502_set_irq_line(irqline,state); }
void m6510_set_irq_callback(int (*callback)(int irqline)) { m6502_set_irq_callback(callback); }
void m6510_state_save(void *file) { m6502_state_save(file); }
void m6510_state_load(void *file) { m6502_state_load(file); }
const char *m6510_info(void *context, int regnum)
{
	switch( regnum )
    {
		case CPU_INFO_NAME: return "M6510";
		case CPU_INFO_VERSION: return "1.1";
	}
	return m6502_info(context,regnum);
}

