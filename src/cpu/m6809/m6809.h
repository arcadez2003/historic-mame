/*** m6809: Portable 6809 emulator ******************************************/

#ifndef _M6809_H
#define _M6809_H

#include "memory.h"
#include "osd_cpu.h"

enum { M6809_A, M6809_B, M6809_PC, M6809_S, M6809_U, M6809_X, M6809_Y, M6809_CC,
	M6809_NMI_STATE, M6809_IRQ_STATE, M6809_FIRQ_STATE };

#define M6809_INT_NONE  0   /* No interrupt required */
#define M6809_INT_IRQ	1	/* Standard IRQ interrupt */
#define M6809_INT_FIRQ	2	/* Fast IRQ */
#define M6809_INT_NMI	4	/* NMI */	/* NS 970909 */
#define M6809_IRQ_LINE	0	/* IRQ line number */
#define M6809_FIRQ_LINE 1   /* FIRQ line number */

/* PUBLIC GLOBALS */
extern int  m6809_ICount;


/* PUBLIC FUNCTIONS */
extern void m6809_reset(void *param);
extern void m6809_exit(void);
extern int m6809_execute(int cycles);  /* NS 970908 */
extern unsigned m6809_get_context(void *dst);
extern void m6809_set_context(void *src);
extern unsigned m6809_get_pc(void);
extern void m6809_set_pc(unsigned val);
extern unsigned m6809_get_sp(void);
extern void m6809_set_sp(unsigned val);
extern unsigned m6809_get_reg(int regnum);
extern void m6809_set_reg(int regnum, unsigned val);
extern void m6809_set_nmi_line(int state);
extern void m6809_set_irq_line(int irqline, int state);
extern void m6809_set_irq_callback(int (*callback)(int irqline));
extern void m6809_state_save(void *file);
extern void m6809_state_load(void *file);
extern const char *m6809_info(void *context,int regnum);

/****************************************************************************/
/* For now the 6309 is using the functions of the 6809						*/
/****************************************************************************/
#define M6309_INT_NONE                  M6809_INT_NONE
#define M6309_INT_IRQ					M6809_INT_IRQ
#define M6309_INT_FIRQ					M6809_INT_FIRQ
#define M6309_INT_NMI					M6809_INT_NMI
#define M6309_IRQ_LINE					M6809_IRQ_LINE
#define M6309_FIRQ_LINE 				M6809_FIRQ_LINE

#define m6309_ICount                    m6809_ICount
extern void m6309_reset(void *param);
extern void m6309_exit(void);
extern int m6309_execute(int cycles);  /* NS 970908 */
extern unsigned m6309_get_context(void *dst);
extern void m6309_set_context(void *src);
extern unsigned m6309_get_pc(void);
extern void m6309_set_pc(unsigned val);
extern unsigned m6309_get_sp(void);
extern void m6309_set_sp(unsigned val);
extern unsigned m6309_get_reg(int regnum);
extern void m6309_set_reg(int regnum, unsigned val);
extern void m6309_set_nmi_line(int state);
extern void m6309_set_irq_line(int irqline, int state);
extern void m6309_set_irq_callback(int (*callback)(int irqline));
extern void m6309_state_save(void *file);
extern void m6309_state_load(void *file);
extern const char *m6309_info(void *context,int regnum);

/****************************************************************************/
/* Read a byte from given memory location									*/
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6809_RDMEM(Addr) ((unsigned)cpu_readmem16(Addr))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6809_WRMEM(Addr,Value) (cpu_writemem16(Addr,Value))

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6809_RDOP(Addr) ((unsigned)cpu_readop(Addr))

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6809_RDOP_ARG(Addr) ((unsigned)cpu_readop_arg(Addr))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6809_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6809_FAST_OP is set, opcodes are fetched     */
/* directly from the ROM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6809_Flags;
#define M6809_FAST_NONE	0x00	/* no memory optimizations */
#define M6809_FAST_S	0x02	/* stack */
#define M6809_FAST_U	0x04	/* user stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6809 (char *buffer, int pc);
#endif

#endif /* _M6809_H */
