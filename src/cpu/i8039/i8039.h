/**************************************************************************
 *                      Intel 8039 Portable Emulator                      *
 *                                                                        *
 *                   Copyright (C) 1997 by Mirko Buffoni                  *
 *  Based on the original work (C) 1997 by Dan Boris, an 8048 emulator    *
 *     You are not allowed to distribute this software commercially       *
 *        Please, notify me, if you make any changes to this file         *
 **************************************************************************/

#ifndef _I8039_H
#define _I8039_H

/**************************************************************************
 * If your compiler doesn't know about inlined functions, uncomment this  *
 **************************************************************************/

/* #define INLINE static */

#ifndef EMU_TYPES_8039
#define EMU_TYPES_8039

#include "osd_cpu.h"

typedef union
{
#ifdef LSB_FIRST
   struct { UINT8 l,h; } B;
   UINT16 W;
#else
   struct { UINT8 h,l; } B;
   UINT16 W;
#endif
} i8039_pair;

#endif  /* EMUTYPES_8039 */

/**************************************************************************
 * End of machine dependent definitions                                   *
 **************************************************************************/

#ifndef INLINE
#define INLINE static inline
#endif

typedef struct
{
	i8039_pair	PC; 		/* -NS- */
	UINT8	A, SP, PSW;
	UINT8	RAM[128];
	UINT8	bus, f1;		/* Bus data, and flag1 */

	int 	pending_irq,irq_executing, masterClock, regPtr;
	UINT8	t_flag, timer, timerON, countON, xirq_en, tirq_en;
	UINT16	A11, A11ff;
#if NEW_INTERRUPT_SYSTEM
	int 	irq_state;
	int 	(*irq_callback)(int irqline);
#endif
} I8039_Regs;

extern   int I8039_ICount;      /* T-state count                          */

/* HJB 01/05/99 changed to positive values to use pending_irq as a flag */
#define I8039_IGNORE_INT    0   /* Ignore interrupt                     */
#define I8039_EXT_INT		1	/* Execute a normal extern interrupt	*/
#define I8039_TIMER_INT 	2	/* Execute a Timer interrupt			*/
#define I8039_COUNT_INT 	4	/* Execute a Counter interrupt			*/

unsigned I8039_GetPC  (void);			/* Get program counter			*/
void I8039_GetRegs(I8039_Regs *Regs);	/* Get registers				*/
void I8039_SetRegs(I8039_Regs *Regs);	/* Set registers				*/
void I8039_Reset  (void);				/* Reset processor & registers	*/
int I8039_Execute(int cycles);			/* Execute cycles T-States - returns number of cycles actually run */

#if NEW_INTERRUPT_SYSTEM
void I8039_set_nmi_line(int state);
void I8039_set_irq_line(int irqline, int state);
void I8039_set_irq_callback(int (*callback)(int irqline));
#else
void I8039_Cause_Interrupt(int type);	/* NS 970904 */
void I8039_Clear_Pending_Interrupts(void);	/* NS 970904 */
#endif

/*   This handling of special I/O ports should be better for actual MAME
 *   architecture.  (i.e., define access to ports { I8039_p1, I8039_p1, dkong_out_w })
 */

#if OLDPORTHANDLING
		UINT8	 I8039_port_r(UINT8 port);
		void	 I8039_port_w(UINT8 port, UINT8 data);
		UINT8	 I8039_test_r(UINT8 port);
		void	 I8039_test_w(UINT8 port, UINT8 data);
		UINT8	 I8039_bus_r(void);
		void	 I8039_bus_w(UINT8 data);
#else
        #define  I8039_p0	0x100   /* Not used */
        #define  I8039_p1	0x101
        #define  I8039_p2	0x102
        #define  I8039_p4	0x104
        #define  I8039_p5	0x105
        #define  I8039_p6	0x106
        #define  I8039_p7	0x107
        #define  I8039_t0	0x110
        #define  I8039_t1	0x111
        #define  I8039_bus	0x120
#endif

#include "memory.h"

/*
 *	 Input a UINT8 from given I/O port
 */
#define I8039_In(Port) ((UINT8)cpu_readport(Port))


/*
 *	 Output a UINT8 to given I/O port
 */
#define I8039_Out(Port,Value) (cpu_writeport(Port,Value))


/*
 *	 Read a UINT8 from given memory location
 */
#define I8039_RDMEM(A) ((unsigned)cpu_readmem16(A))


/*
 *	 Write a UINT8 to given memory location
 */
#define I8039_WRMEM(A,V) (cpu_writemem16(A,V))


/*
 *   I8039_RDOP() is identical to I8039_RDMEM() except it is used for reading
 *   opcodes. In case of system with memory mapped I/O, this function can be
 *   used to greatly speed up emulation
 */
#define I8039_RDOP(A) ((unsigned)cpu_readop(A))


/*
 *   I8039_RDOP_ARG() is identical to I8039_RDOP() except it is used for reading
 *   opcode arguments. This difference can be used to support systems that
 *   use different encoding mechanisms for opcodes and opcode arguments
 */
#define I8039_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

#ifdef  MAME_DEBUG
int     Dasm8039(char * dst, unsigned char* addr);
#endif

#endif  /* _I8039_H */
