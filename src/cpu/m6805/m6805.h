/*** m6805: Portable 6805 emulator ******************************************/

#ifndef _M6805_H
#define _M6805_H

#include "memory.h"
#include "osd_cpu.h"

enum { M6805_A, M6805_PC, M6805_S, M6805_X, M6805_CC, M6805_IRQ_STATE };

#define M6805_INT_NONE  0           /* No interrupt required */
#define M6805_INT_IRQ	1 			/* Standard IRQ interrupt */

/* PUBLIC GLOBALS */
extern int  m6805_ICount;

/* PUBLIC FUNCTIONS */
extern void m6805_reset(void *param);
extern void m6805_exit(void);
extern int  m6805_execute(int cycles);             /* MB */
extern unsigned m6805_get_context(void *dst);
extern void m6805_set_context(void *src);
extern unsigned m6805_get_pc(void);
extern void m6805_set_pc(unsigned val);
extern unsigned m6805_get_sp(void);
extern void m6805_set_sp(unsigned val);
extern unsigned m6805_get_reg(int regnum);
extern void m6805_set_reg(int regnum, unsigned val);
extern void m6805_set_nmi_line(int state);
extern void m6805_set_irq_line(int irqline, int state);
extern void m6805_set_irq_callback(int (*callback)(int irqline));
extern void m6805_state_save(void *file);
extern void m6805_state_load(void *file);
extern const char *m6805_info(void *context, int regnum);

/****************************************************************************
 * 68705 section
 ****************************************************************************/
#define M68705_A					M6805_A
#define M68705_PC					M6805_PC
#define M68705_S					M6805_S
#define M68705_X					M6805_X
#define M68705_CC					M6805_CC
#define M68705_IRQ_STATE			M6805_IRQ_STATE

#define M68705_INT_NONE             M6805_INT_NONE
#define M68705_INT_IRQ				M6805_INT_IRQ

#define m68705_ICount				m6805_ICount
extern void m68705_reset(void *param);
extern void m68705_exit(void);
extern int	m68705_execute(int cycles); 			/* MB */
extern unsigned m68705_get_context(void *dst);
extern void m68705_set_context(void *src);
extern unsigned m68705_get_pc(void);
extern void m68705_set_pc(unsigned val);
extern unsigned m68705_get_sp(void);
extern void m68705_set_sp(unsigned val);
extern unsigned m68705_get_reg(int regnum);
extern void m68705_set_reg(int regnum, unsigned val);
extern void m68705_set_nmi_line(int state);
extern void m68705_set_irq_line(int irqline, int state);
extern void m68705_set_irq_callback(int (*callback)(int irqline));
extern void m68705_state_save(void *file);
extern void m68705_state_load(void *file);
extern const char *m68705_info(void *context, int regnum);

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6805_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6805_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* M6805_RDOP() is identical to M6805_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6805_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* M6805_RDOP_ARG() is identical to M6805_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6805_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6805_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6805_FAST_S is set, bytes are pulled 		*/
/* directly from the RAM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6805_Flags;
#define M6805_FAST_NONE	0x00	/* no memory optimizations */
#define M6805_FAST_S	0x02	/* stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6805(unsigned char *base, char *buf, int pc);
#endif

#endif /* _M6805_H */