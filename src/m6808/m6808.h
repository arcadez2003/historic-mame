/*** m6808: Portable 6808 emulator ******************************************/

#ifndef _M6808_H
#define _M6808_H

#include "memory.h"


/****************************************************************************/
/* sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4                         */
/****************************************************************************/
#include "types.h"	/* -NS- */

/* 6808 Registers */
typedef struct
{
	word		pc;	/* Program counter */
	word		u, s;	/* Stack pointers */
	word		x, y;	/* Index registers */
	byte		dp;	/* Direct Page register */
	byte		a, b;	/* Accumulator */
	byte		cc;

	int pending_interrupts;	/* MB */
} m6808_Regs;

#ifndef INLINE
#define INLINE static inline
#endif


#define M6808_INT_NONE  0            /* No interrupt required */
#define M6808_INT_IRQ	1            /* Standard IRQ interrupt */
#define M6808_INT_NMI	2            /* NMI interrupt          */


/* PUBLIC FUNCTIONS */
extern void m6808_SetRegs(m6808_Regs *Regs);
extern void m6808_GetRegs(m6808_Regs *Regs);
extern unsigned m6808_GetPC(void);
extern void m6808_reset(void);
extern int  m6808_execute(int cycles);             /* MB */
extern void m6808_Cause_Interrupt(int type);       /* MB */
extern void m6808_Clear_Pending_Interrupts(void);  /* MB */

/* PUBLIC GLOBALS */
/*extern int	m6808_IPeriod; */
extern int	m6808_ICount;
/* extern int	m6808_IRequest; */

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6808_RDMEM(A) ((unsigned)cpu_readmem16(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6808_WRMEM(A,V) (cpu_writemem16(A,V))

/****************************************************************************/
/* M6808_RDOP() is identical to M6808_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6808_RDOP(A) ((unsigned)cpu_readop(A))

/****************************************************************************/
/* M6808_RDOP_ARG() is identical to M6808_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6808_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))


#endif /* _M6808_H */

