/*** m6800: Portable 6800 class  emulator *************************************

	m6800.c

	References:

		6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.

		m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
			the 6809 Simulator V09)

		6809 Microcomputer Programming & Interfacing with Experiments"
			by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

	System dependencies:	UINT16 must be 16 bit unsigned int
							UINT8 must be 8 bit unsigned int
							UINT32 must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

History
990316	HJB
	Renamed to 6800, since that's the basic CPU.
	Added different cycle count tables for M6800/2/8, M6801/3 and HD63701.

990314  HJB
	Also added the M6800 subtype.

990311  HJB
	Added _info functions. Now uses static m6808_Regs struct instead
	of single statics. Changed the 16 bit registers to use the generic
	PAIR union. Registers defined using macros. Split the core into
	four execution loops for M6802, M6803, M6808 and HD63701.
	TST, TSTA and TSTB opcodes reset carry flag.
TODO:
	Verify invalid opcodes for the different CPU types.
	Add proper credits to _info functions.
	Integrate m6808_Flags into the registers (multiple m6808 type CPUs?)

990301	HJB
	Modified the interrupt handling. No more pending interrupt checks.
	WAI opcode saves state, when an interrupt is taken (IRQ or OCI),
	the state is only saved if not already done by WAI.

*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpuintrf.h"
#include "osd_dbg.h"
#include "state.h"
#include "m6800.h"

extern FILE *errorlog;

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

/* 6800 Registers */
typedef struct
{
	PAIR	pc; 			/* Program counter */
	PAIR	s;				/* Stack pointer */
	PAIR	x;				/* Index register */
	PAIR	d;				/* Accumulators */
	UINT8	cc; 			/* Condition codes */
	UINT8	wai_state;		/* WAI opcode state */
	UINT8	nmi_state;		/* NMI line state */
	UINT8	irq_state[2];	/* IRQ and OCI line state */
    int     (*irq_callback)(int irqline);
	int 	extra_cycles;	/* cycles used for interrupts */
}   m6800_Regs;

/* 680x registers */
static m6800_Regs m6800;

#define m6801   m6800
#define m6802   m6800
#define m6803	m6800
#define m6808	m6800
#define hd63701 m6800

#define pPC m6800.pc
#define pS	m6800.s
#define pX	m6800.x
#define pD	m6800.d

#define PC	m6800.pc.w.l
#define S	m6800.s.w.l
#define X	m6800.x.w.l
#define D	m6800.d.w.l
#define A	m6800.d.b.h
#define B	m6800.d.b.l
#define CC	m6800.cc

static PAIR ea; 		/* effective address */
#define EAD ea.d
#define EA	ea.w.l

/* public globals */
int m6800_ICount=50000;
int m6800_Flags;		/* flags for speed optimization */

/* handlers for speed optimization */
static void (*rd_s_handler_b)(UINT8 *);
static void (*rd_s_handler_w)(PAIR *);
static void (*wr_s_handler_b)(UINT8 *);
static void (*wr_s_handler_w)(PAIR *);

/* DS -- THESE ARE RE-DEFINED IN m6800.h TO RAM, ROM or FUNCTIONS IN cpuintrf.c */
#define RM				M6800_RDMEM
#define WM				M6800_WRMEM
#define M_RDOP			M6800_RDOP
#define M_RDOP_ARG		M6800_RDOP_ARG

/* macros to access memory */
#define IMMBYTE(b) {b = M_RDOP_ARG(PC++);}
#define IMMWORD(w) {w.d = 0; w.b.h = M_RDOP_ARG(PC); w.b.l = M_RDOP_ARG(PC+1); PC+=2;}

#define PUSHBYTE(b) (*wr_s_handler_b)(&b)
#define PUSHWORD(w) (*wr_s_handler_w)(&w)
#define PULLBYTE(b) (*rd_s_handler_b)(&b)
#define PULLWORD(w) (*rd_s_handler_w)(&w)

/* check the IRQ lines for pending interrupts */
#define CHECK_IRQ_LINES {                                               \
	if( m6800.irq_state[M6800_IRQ_LINE] != CLEAR_LINE && !(CC & 0x10) ) \
	{																	\
        /* standard IRQ */                                              \
		LOG((errorlog, "M6800#%d take IRQ\n", cpu_getactivecpu()));     \
		if( m6800.wai_state & M6800_WAI )								\
        {                                                               \
			m6800.extra_cycles += 4;									\
		}																\
		else															\
        {                                                               \
			PUSHWORD(pPC);												\
			PUSHWORD(pX);												\
			PUSHBYTE(A);												\
			PUSHBYTE(B);												\
			PUSHBYTE(CC);												\
			m6800.extra_cycles += 12;									\
        }                                                               \
        SEI;                                                            \
		RM16( 0xfff8, &pPC );											\
		change_pc(PC);													\
		if( m6800.irq_callback )										\
			(void)(*m6800.irq_callback)(M6800_IRQ_LINE);				\
	}																	\
	else																\
	if( m6800.irq_state[M6800_OCI_LINE] != CLEAR_LINE ) 				\
	{																	\
        /* OCI */                                                       \
		LOG((errorlog, "M6800#%d take OCI\n", cpu_getactivecpu()));     \
		if( m6800.wai_state & M6800_WAI )								\
		{																\
			m6800.extra_cycles += 4;									\
		}																\
        else                                                            \
        {                                                               \
			PUSHWORD(pPC);												\
			PUSHWORD(pX);												\
			PUSHBYTE(A);												\
			PUSHBYTE(B);												\
			PUSHBYTE(CC);												\
			m6800.extra_cycles += 12;									\
        }                                                               \
        SEI;                                                            \
		RM16( 0xfff4, &pPC );											\
		change_pc(PC);													\
		if( m6800.irq_callback )										\
			(void)(*m6800.irq_callback)(M6800_OCI_LINE);				\
    }                                                                   \
}

/* CC masks                       HI NZVC
								7654 3210	*/
#define CLR_HNZVC	CC&=0xd0
#define CLR_NZV 	CC&=0xf1
#define CLR_HNZC	CC&=0xd2
#define CLR_NZVC	CC&=0xf0
#define CLR_Z		CC&=0xfb
#define CLR_NZC 	CC&=0xf2
#define CLR_ZC		CC&=0xfa

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!a)SEZ
#define SET_Z8(a)		SET_Z((UINT8)a)
#define SET_Z16(a)		SET_Z((UINT16)a)
#define SET_N8(a)		CC|=((a&0x80)>>4)
#define SET_N16(a)		CC|=((a&0x8000)>>12)
#define SET_H(a,b,r)	CC|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)		CC|=((a&0x100)>>8)
#define SET_C16(a)		CC|=((a&0x10000)>>16)
#define SET_V8(a,b,r)	CC|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)	CC|=(((a^b^r^(r>>1))&0x8000)>>14)

static UINT8 flags8i[256]=	 /* increment */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
static UINT8 flags8d[256]= /* decrement */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
#define SET_FLAGS8I(a)		{CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an UINT8 as a signed INT16 */
#define SIGNED(b) ((INT16)(b&0x80?b|0xff00:b))

/* Macros for addressing modes */
#define DIRECT IMMBYTE(EAD)
#define IMM8 EA=PC++
#define IMM16 {EA=PC;PC+=2;}
#define EXTENDED IMMWORD(ea)
#define INDEXED {EA=X+(UINT8)M_RDOP_ARG(PC++);}

/* macros to set status flags */
#define SEC CC|=0x01
#define CLC CC&=0xfe
#define SEZ CC|=0x04
#define CLZ CC&=0xfb
#define SEN CC|=0x08
#define CLN CC&=0xf7
#define SEV CC|=0x02
#define CLV CC&=0xfd
#define SEH CC|=0x20
#define CLH CC&=0xdf
#define SEI CC|=0x10
#define CLI CC&=~0x10

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;RM16(EAD,&w);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;RM16(EAD,&w);}

#define IDXBYTE(b) {INDEXED;b=RM(EAD);}
#define IDXWORD(w) {INDEXED;RM16(EAD,&w);}

/* Macros for branch instructions */
#define BRANCH(f) {IMMBYTE(t);if(f){PC+=SIGNED(t);change_pc(PC);}}
#define NXORV  ((CC&0x08)^((CC&0x02)<<2))

static unsigned char cycles_6800[] =
{
		/* 0  1  2	3  4  5  6	7  8  9  A	B  C  D  E	F */
	/*0*/  0, 2, 0, 0, 0, 0, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,
	/*1*/  2, 2, 0, 0, 0, 0, 2, 2, 0, 2, 0, 2, 0, 0, 0, 0,
	/*2*/  4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	/*3*/  4, 4, 4, 4, 4, 4, 4, 4, 0, 5, 0,10, 0, 0, 9,12,
	/*4*/  2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
	/*5*/  2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
	/*6*/  7, 0, 0, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 4, 7,
	/*7*/  6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
	/*8*/  2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 3, 8, 3, 0,
	/*9*/  3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 4, 0, 4, 5,
	/*A*/  5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,
	/*B*/  4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 5, 9, 5, 6,
	/*C*/  2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 0, 0, 3, 0,
	/*D*/  3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 0, 0, 4, 5,
	/*E*/  5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 0, 0, 6, 7,
	/*F*/  4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 0, 0, 5, 6
};

static unsigned char cycles_6803[] =
{
		/* 0  1  2	3  4  5  6	7  8  9  A	B  C  D  E	F */
	/*0*/  0, 2, 0, 0, 3, 3, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2,
	/*1*/  2, 2, 0, 0, 0, 0, 2, 2, 0, 2, 0, 2, 0, 0, 0, 0,
	/*2*/  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	/*3*/  3, 3, 4, 4, 3, 3, 3, 3, 5, 5, 3,10, 4,10, 9,12,
	/*4*/  2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
	/*5*/  2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
	/*6*/  6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
	/*7*/  6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
	/*8*/  2, 2, 2, 4, 2, 2, 2, 0, 2, 2, 2, 2, 4, 6, 3, 0,
	/*9*/  3, 3, 3, 5, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 4, 4,
	/*A*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 5, 5,
	/*B*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 5, 5,
	/*C*/  2, 2, 2, 4, 2, 2, 2, 0, 2, 2, 2, 2, 3, 0, 3, 0,
	/*D*/  3, 3, 3, 5, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
	/*E*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	/*F*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5
};

static unsigned char cycles_63701[] =
{
		/* 0  1  2	3  4  5  6	7  8  9  A	B  C  D  E	F */
	/*0*/  0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/*1*/  1, 1, 0, 0, 0, 0, 1, 1, 2, 2, 4, 1, 0, 0, 0, 0,
	/*2*/  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	/*3*/  1, 1, 3, 3, 1, 1, 4, 4, 4, 5, 1,10, 5, 7, 9,12,
	/*4*/  1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,
	/*5*/  1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1,
	/*6*/  6, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 5, 6, 4, 3, 5,
	/*7*/  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 4, 3, 5,
	/*8*/  2, 2, 2, 3, 2, 2, 2, 0, 2, 2, 2, 2, 3, 5, 3, 0,
	/*9*/  3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 4, 4,
	/*A*/  4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	/*B*/  4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 5, 5,
	/*C*/  2, 2, 2, 3, 2, 2, 2, 0, 2, 2, 2, 2, 3, 0, 3, 0,
	/*D*/  3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
	/*E*/  4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	/*F*/  4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5
};

/* pre-clear a PAIR union; clearing h2 and h3 only might be faster? */
#define CLEAR_PAIR(p)   p->d = 0

static void rd_s_slow_b( UINT8 *b )
{
	m6800.s.w.l++;
	*b = RM( m6800.s.d );
}

static void rd_s_slow_w( PAIR *p )
{
	CLEAR_PAIR(p);
	m6800.s.w.l++;
	p->b.l = RM( m6800.s.d );
	m6800.s.w.l++;
	p->b.h = RM( m6800.s.d );
}

static void rd_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	m6800.s.w.l++;
	*b = RAM[ m6800.s.d ];
}

static void rd_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	CLEAR_PAIR(p);
	m6800.s.w.l++;
	p->b.l = RAM[ m6800.s.d ];
	m6800.s.w.l++;
	p->b.h = RAM[ m6800.s.d ];
}

static void wr_s_slow_b( UINT8 *b )
{
	WM( m6800.s.d, *b );
	--m6800.s.w.l;
}

static void wr_s_slow_w( PAIR *p )
{
	WM( m6800.s.d, p->b.h );
	--m6800.s.w.l;
	WM( m6800.s.d, p->b.l );
	--m6800.s.w.l;
}

static void wr_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	RAM[ m6800.s.d ] = *b;
	--m6800.s.w.l;
}

static void wr_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	RAM[ m6800.s.d ] = p->b.h;
	--m6800.s.w.l;
	RAM[ m6800.s.d ] = p->b.l;
	--m6800.s.w.l;
}

INLINE void RM16( UINT32 Addr, PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM(Addr);
	if( ++Addr > 0xffff ) Addr = 0;
	p->b.l = RM(Addr);
}

INLINE void WM16( UINT32 Addr, PAIR *p )
{
	WM( Addr, p->b.h );
	if( ++Addr > 0xffff ) Addr = 0;
	WM( Addr, p->b.l );
}

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/
void m6800_reset(void *param)
{

	/* default to unoptimized memory access */
	rd_s_handler_b = rd_s_slow_b;
	rd_s_handler_w = rd_s_slow_w;
	wr_s_handler_b = wr_s_slow_b;
	wr_s_handler_w = wr_s_slow_w;

    /* optimize memory access according to flags */
	if( m6800_Flags & M6800_FAST_S )
    {
		rd_s_handler_b = rd_s_fast_b;
		rd_s_handler_w = rd_s_fast_w;
		wr_s_handler_b = wr_s_fast_b;
		wr_s_handler_w = wr_s_fast_w;
    }

    SEI;            /* IRQ disabled */
	RM16( 0xfffe, &m6800.pc );
    change_pc(PC);
}

/****************************************************************************
 * Shut down CPU emulatio
 ****************************************************************************/
void m6800_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned m6800_get_context(void *dst)
{
	if( dst )
		*(m6800_Regs*)dst = m6800;
	return sizeof(m6800_Regs);
}


/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void m6800_set_context(void *src)
{
	if( src )
		m6800 = *(m6800_Regs*)src;
	change_pc(PC);
    CHECK_IRQ_LINES;    /* HJB 990301 */
}


/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned m6800_get_pc(void)
{
    return PC;
}


/****************************************************************************
 * Set program counter
 ****************************************************************************/
void m6800_set_pc(unsigned val)
{
	PC = val;
	change_pc(PC);
}


/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned m6800_get_sp(void)
{
	return S;
}


/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void m6800_set_sp(unsigned val)
{
	S = val;
}


/****************************************************************************
 * Return a specific register
 ****************************************************************************/
unsigned m6800_get_reg(int regnum)
{
	switch( regnum )
	{
		case M6800_A: return m6800.d.b.h;
		case M6800_B: return m6800.d.b.l;
		case M6800_PC: return m6800.pc.w.l;
		case M6800_S: return m6800.s.w.l;
		case M6800_X: return m6800.x.w.l;
		case M6800_CC: return m6800.cc;
		case M6800_NMI_STATE: return m6800.nmi_state;
		case M6800_IRQ_STATE: return m6800.irq_state[M6800_IRQ_LINE];
		case M6800_OCI_STATE: return m6800.irq_state[M6800_OCI_LINE];
	}
	return 0;
}


/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void m6800_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M6800_A: m6800.d.b.h = val; break;
		case M6800_B: m6800.d.b.l = val; break;
		case M6800_PC: m6800.pc.w.l = val; break;
		case M6800_S: m6800.s.w.l = val; break;
		case M6800_X: m6800.x.w.l = val; break;
		case M6800_CC: m6800.cc = val; break;
		case M6800_NMI_STATE: m6800.nmi_state = val; break;
		case M6800_IRQ_STATE: m6800.irq_state[M6800_IRQ_LINE] = val; break;
		case M6800_OCI_STATE: m6800.irq_state[M6800_OCI_LINE] = val; break;
    }
}


void m6800_set_nmi_line(int state)
{
	if (m6800.nmi_state == state) return;
	LOG((errorlog, "M6800#%d set_nmi_line %d ", cpu_getactivecpu(), state));
	m6800.nmi_state = state;
	if (state == CLEAR_LINE) return;

	/* NMI */
	LOG((errorlog, "M6800#%d take NMI\n", cpu_getactivecpu()));
	if( m6800.wai_state & M6800_WAI )
	{
		m6800.extra_cycles += 4;
	}
	else
	{
		PUSHWORD(m6800.pc);
		PUSHWORD(m6800.x);
		PUSHBYTE(m6800.d.b.h);
		PUSHBYTE(m6800.d.b.l);
		PUSHBYTE(m6800.cc);
		m6800.extra_cycles += 12;
	}
	SEI;
	RM16(0xfffc,&m6800.pc);
	change_pc(PC);
}

void m6800_set_irq_line(int irqline, int state)
{
	if (m6800.irq_state[irqline] == state) return;
	LOG((errorlog, "M6800#%d set_irq_line %d,%d\n", cpu_getactivecpu(), irqline, state));
	m6800.irq_state[irqline] = state;
	if (state == CLEAR_LINE) return;

    CHECK_IRQ_LINES;
}

void m6800_set_irq_callback(int (*callback)(int irqline))
{
	m6800.irq_callback = callback;
}

static void state_save(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT8(file,module,cpu,"A", &m6800.d.b.h, 1);
	state_save_UINT8(file,module,cpu,"B", &m6800.d.b.l, 1);
	state_save_UINT16(file,module,cpu,"PC", &m6800.pc.w.l, 1);
	state_save_UINT16(file,module,cpu,"S", &m6800.s.w.l, 1);
	state_save_UINT16(file,module,cpu,"X", &m6800.x.w.l, 1);
	state_save_UINT8(file,module,cpu,"CC", &m6800.cc, 1);
	state_save_UINT8(file,module,cpu,"NMI_STATE", &m6800.nmi_state, 1);
	state_save_UINT8(file,module,cpu,"IRQ_STATE", &m6800.irq_state[M6800_IRQ_LINE], 1);
	state_save_UINT8(file,module,cpu,"OCI_STATE", &m6800.irq_state[M6800_OCI_LINE], 1);
}

static void state_load(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT8(file,module,cpu,"A", &m6800.d.b.h, 1);
	state_load_UINT8(file,module,cpu,"B", &m6800.d.b.l, 1);
	state_load_UINT16(file,module,cpu,"PC", &m6800.pc.w.l, 1);
	state_load_UINT16(file,module,cpu,"S", &m6800.s.w.l, 1);
	state_load_UINT16(file,module,cpu,"X", &m6800.x.w.l, 1);
	state_load_UINT8(file,module,cpu,"CC", &m6800.cc, 1);
	state_load_UINT8(file,module,cpu,"NMI_STATE", &m6800.nmi_state, 1);
	state_load_UINT8(file,module,cpu,"IRQ_STATE", &m6800.irq_state[M6800_IRQ_LINE], 1);
	state_load_UINT8(file,module,cpu,"OCI_STATE", &m6800.irq_state[M6800_OCI_LINE], 1);
}

void m6800_state_save(void *file) { state_save(file,"m6800"); }
void m6800_state_load(void *file) { state_load(file,"m6800"); }

#include "6800ops.c"

/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
int m6800_execute(int cycles)
{
	UINT8 ireg;
	m6800_ICount = cycles;

	if( m6800.wai_state & M6800_WAI )
	{
		m6800_ICount = 0;
		goto getout;
	}

	do
	{

#if ASGARD680X_DEBUG
{
extern void Asgard63701MiniTrace(unsigned long sx, unsigned long pcab, unsigned long flags, int icount);
	Asgard63701MiniTrace ((S<<16)+X, (PC<<16)+(A<<8)+B, (wai_state<<16)+CC, m6800_ICount);
}
#endif

#if 0
		asg_6808Trace(OP_ROM, PC);
#endif

#ifdef	MAME_DEBUG
		if (mame_debug) MAME_Debug();
#endif
		ireg=M_RDOP(PC++);

		switch( ireg )
		{
			case 0x00: illegal(); break;
			case 0x01: nop(); break;
			case 0x02: illegal(); break;
			case 0x03: illegal(); break;
			case 0x04: illegal(); break;
			case 0x05: illegal(); break;
			case 0x06: tap(); break;
			case 0x07: tpa(); break;
			case 0x08: inx(); break;
			case 0x09: dex(); break;
			case 0x0A: CLV; break;
			case 0x0B: SEV; break;
			case 0x0C: CLC; break;
			case 0x0D: SEC; break;
			case 0x0E: CLI; CHECK_IRQ_LINES; break;
			case 0x0F: SEI; break;
			case 0x10: sba(); break;
			case 0x11: cba(); break;
			case 0x12: illegal(); break;
			case 0x13: illegal(); break;
			case 0x14: illegal(); break;
			case 0x15: illegal(); break;
			case 0x16: tab(); break;
			case 0x17: tba(); break;
			case 0x18: illegal(); break;
			case 0x19: daa(); break;
			case 0x1a: illegal(); break;
			case 0x1b: aba(); break;
			case 0x1c: illegal(); break;
			case 0x1d: illegal(); break;
			case 0x1e: illegal(); break;
			case 0x1f: illegal(); break;
			case 0x20: bra(); break;
			case 0x21: brn(); break;
			case 0x22: bhi(); break;
			case 0x23: bls(); break;
			case 0x24: bcc(); break;
			case 0x25: bcs(); break;
			case 0x26: bne(); break;
			case 0x27: beq(); break;
			case 0x28: bvc(); break;
			case 0x29: bvs(); break;
			case 0x2a: bpl(); break;
			case 0x2b: bmi(); break;
			case 0x2c: bge(); break;
			case 0x2d: blt(); break;
			case 0x2e: bgt(); break;
			case 0x2f: ble(); break;
			case 0x30: tsx(); break;
			case 0x31: ins(); break;
			case 0x32: pula(); break;
			case 0x33: pulb(); break;
			case 0x34: des(); break;
			case 0x35: txs(); break;
			case 0x36: psha(); break;
			case 0x37: pshb(); break;
			case 0x38: illegal(); break;
			case 0x39: rts(); break;
			case 0x3a: illegal(); break;
			case 0x3b: rti(); break;
			case 0x3c: illegal(); break;
			case 0x3d: illegal(); break;
			case 0x3e: wai(); break;
			case 0x3f: swi(); break;
			case 0x40: nega(); break;
			case 0x41: illegal(); break;
			case 0x42: illegal(); break;
			case 0x43: coma(); break;
			case 0x44: lsra(); break;
			case 0x45: illegal(); break;
			case 0x46: rora(); break;
			case 0x47: asra(); break;
			case 0x48: asla(); break;
			case 0x49: rola(); break;
			case 0x4a: deca(); break;
			case 0x4b: illegal(); break;
			case 0x4c: inca(); break;
			case 0x4d: tsta(); break;
			case 0x4e: illegal(); break;
			case 0x4f: clra(); break;
			case 0x50: negb(); break;
			case 0x51: illegal(); break;
			case 0x52: illegal(); break;
			case 0x53: comb(); break;
			case 0x54: lsrb(); break;
			case 0x55: illegal(); break;
			case 0x56: rorb(); break;
			case 0x57: asrb(); break;
			case 0x58: aslb(); break;
			case 0x59: rolb(); break;
			case 0x5a: decb(); break;
			case 0x5b: illegal(); break;
			case 0x5c: incb(); break;
			case 0x5d: tstb(); break;
			case 0x5e: illegal(); break;
			case 0x5f: clrb(); break;
			case 0x60: neg_ix(); break;
			case 0x61: illegal(); break;
			case 0x62: illegal(); break;
			case 0x63: com_ix(); break;
			case 0x64: lsr_ix(); break;
			case 0x65: illegal(); break;
			case 0x66: ror_ix(); break;
			case 0x67: asr_ix(); break;
			case 0x68: asl_ix(); break;
			case 0x69: rol_ix(); break;
			case 0x6a: dec_ix(); break;
			case 0x6b: illegal(); break;
			case 0x6c: inc_ix(); break;
			case 0x6d: tst_ix(); break;
			case 0x6e: jmp_ix(); break;
			case 0x6f: clr_ix(); break;
			case 0x70: neg_ex(); break;
			case 0x71: illegal(); break;
			case 0x72: illegal(); break;
			case 0x73: com_ex(); break;
			case 0x74: lsr_ex(); break;
			case 0x75: illegal(); break;
			case 0x76: ror_ex(); break;
			case 0x77: asr_ex(); break;
			case 0x78: asl_ex(); break;
			case 0x79: rol_ex(); break;
			case 0x7a: dec_ex(); break;
			case 0x7b: illegal(); break;
			case 0x7c: inc_ex(); break;
			case 0x7d: tst_ex(); break;
			case 0x7e: jmp_ex(); break;
			case 0x7f: clr_ex(); break;
			case 0x80: suba_im(); break;
			case 0x81: cmpa_im(); break;
			case 0x82: sbca_im(); break;
			case 0x83: illegal(); break;
			case 0x84: anda_im(); break;
			case 0x85: bita_im(); break;
			case 0x86: lda_im(); break;
			case 0x87: sta_im(); break;
			case 0x88: eora_im(); break;
			case 0x89: adca_im(); break;
			case 0x8a: ora_im(); break;
			case 0x8b: adda_im(); break;
			case 0x8c: cmpx_im(); break;
			case 0x8d: bsr(); break;
			case 0x8e: lds_im(); break;
			case 0x8f: sts_im(); /* orthogonality */ break;
			case 0x90: suba_di(); break;
			case 0x91: cmpa_di(); break;
			case 0x92: sbca_di(); break;
			case 0x93: illegal(); break;
			case 0x94: anda_di(); break;
			case 0x95: bita_di(); break;
			case 0x96: lda_di(); break;
			case 0x97: sta_di(); break;
			case 0x98: eora_di(); break;
			case 0x99: adca_di(); break;
			case 0x9a: ora_di(); break;
			case 0x9b: adda_di(); break;
			case 0x9c: cmpx_di(); break;
			case 0x9d: jsr_di(); break;
			case 0x9e: lds_di(); break;
			case 0x9f: sts_di(); break;
			case 0xa0: suba_ix(); break;
			case 0xa1: cmpa_ix(); break;
			case 0xa2: sbca_ix(); break;
			case 0xa3: illegal(); break;
			case 0xa4: anda_ix(); break;
			case 0xa5: bita_ix(); break;
			case 0xa6: lda_ix(); break;
			case 0xa7: sta_ix(); break;
			case 0xa8: eora_ix(); break;
			case 0xa9: adca_ix(); break;
			case 0xaa: ora_ix(); break;
			case 0xab: adda_ix(); break;
			case 0xac: cmpx_ix(); break;
			case 0xad: jsr_ix(); break;
			case 0xae: lds_ix(); break;
			case 0xaf: sts_ix(); break;
			case 0xb0: suba_ex(); break;
			case 0xb1: cmpa_ex(); break;
			case 0xb2: sbca_ex(); break;
			case 0xb3: illegal(); break;
			case 0xb4: anda_ex(); break;
			case 0xb5: bita_ex(); break;
			case 0xb6: lda_ex(); break;
			case 0xb7: sta_ex(); break;
			case 0xb8: eora_ex(); break;
			case 0xb9: adca_ex(); break;
			case 0xba: ora_ex(); break;
			case 0xbb: adda_ex(); break;
			case 0xbc: cmpx_ex(); break;
			case 0xbd: jsr_ex(); break;
			case 0xbe: lds_ex(); break;
			case 0xbf: sts_ex(); break;
			case 0xc0: subb_im(); break;
			case 0xc1: cmpb_im(); break;
			case 0xc2: sbcb_im(); break;
			case 0xc3: illegal(); break;
			case 0xc4: andb_im(); break;
			case 0xc5: bitb_im(); break;
			case 0xc6: ldb_im(); break;
			case 0xc7: stb_im(); break;
			case 0xc8: eorb_im(); break;
			case 0xc9: adcb_im(); break;
			case 0xca: orb_im(); break;
			case 0xcb: addb_im(); break;
			case 0xcc: illegal(); break;
			case 0xcd: illegal(); break;
			case 0xce: ldx_im(); break;
			case 0xcf: stx_im(); break;
			case 0xd0: subb_di(); break;
			case 0xd1: cmpb_di(); break;
			case 0xd2: sbcb_di(); break;
			case 0xd3: illegal(); break;
			case 0xd4: andb_di(); break;
			case 0xd5: bitb_di(); break;
			case 0xd6: ldb_di(); break;
			case 0xd7: stb_di(); break;
			case 0xd8: eorb_di(); break;
			case 0xd9: adcb_di(); break;
			case 0xda: orb_di(); break;
			case 0xdb: addb_di(); break;
			case 0xdc: illegal(); break;
			case 0xdd: illegal(); break;
			case 0xde: ldx_di(); break;
			case 0xdf: stx_di(); break;
			case 0xe0: subb_ix(); break;
			case 0xe1: cmpb_ix(); break;
			case 0xe2: sbcb_ix(); break;
			case 0xe3: illegal(); break;
			case 0xe4: andb_ix(); break;
			case 0xe5: bitb_ix(); break;
			case 0xe6: ldb_ix(); break;
			case 0xe7: stb_ix(); break;
			case 0xe8: eorb_ix(); break;
			case 0xe9: adcb_ix(); break;
			case 0xea: orb_ix(); break;
			case 0xeb: addb_ix(); break;
			case 0xec: illegal(); break;
			case 0xed: illegal(); break;
			case 0xee: ldx_ix(); break;
			case 0xef: stx_ix(); break;
			case 0xf0: subb_ex(); break;
			case 0xf1: cmpb_ex(); break;
			case 0xf2: sbcb_ex(); break;
			case 0xf3: illegal(); break;
			case 0xf4: andb_ex(); break;
			case 0xf5: bitb_ex(); break;
			case 0xf6: ldb_ex(); break;
			case 0xf7: stb_ex(); break;
			case 0xf8: eorb_ex(); break;
			case 0xf9: adcb_ex(); break;
			case 0xfa: orb_ex(); break;
			case 0xfb: addb_ex(); break;
			case 0xfc: illegal(); break;
			case 0xfd: illegal(); break;
			case 0xfe: ldx_ex(); break;
			case 0xff: stx_ex(); break;
		}
		m6800_ICount -= cycles_6800[ireg];
	} while( m6800_ICount>0 );

getout:
	return cycles - m6800_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m680x_info(int subtype, void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6800_Regs *r = context;

	which = ++which % 16;
    buffer[which][0] = '\0';
	if( !context )
		r = &m6800;

	switch( regnum )
	{
		case CPU_INFO_NAME:
			switch( subtype )
			{
				case 6800: return "M6800";
				case 6801: return "M6801";
				case 6802: return "M6802";
				case 6803: return "M6803";
				case 6808: return "M6808";
				default: return "HD63701";
			}
			break;
		case CPU_INFO_FAMILY: return "Motorola 6800";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "The MAME team.\n" \
			"References:\n" \
			"   6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.\n" \
			"   m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from the 6809 Simulator V09)\n" \
			"   6809 Microcomputer Programming & Interfacing with Experiments by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.";
        case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->pc.w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%04X", r->s.w.l); break;
#if MAME_DEBUG
		case CPU_INFO_DASM: r->pc.w.l += Dasm680x(subtype,buffer[which], r->pc.w.l); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "$%02x", ROM[r->pc.w.l]); r->pc.w.l++; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->cc & 0x80 ? '?':'.',
				r->cc & 0x40 ? '?':'.',
				r->cc & 0x20 ? 'H':'.',
				r->cc & 0x10 ? 'I':'.',
				r->cc & 0x08 ? 'N':'.',
				r->cc & 0x04 ? 'Z':'.',
				r->cc & 0x02 ? 'V':'.',
				r->cc & 0x01 ? 'C':'.');
            break;
		case CPU_INFO_REG+M6800_A: sprintf(buffer[which], "A:%02X", r->d.b.h); break;
		case CPU_INFO_REG+M6800_B: sprintf(buffer[which], "B:%02X", r->d.b.l); break;
		case CPU_INFO_REG+M6800_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
        case CPU_INFO_REG+M6800_S: sprintf(buffer[which], "S:%04X", r->s.w.l); break;
		case CPU_INFO_REG+M6800_X: sprintf(buffer[which], "X:%04X", r->x.w.l); break;
		case CPU_INFO_REG+M6800_CC: sprintf(buffer[which], "CC:%02X", r->cc); break;
		case CPU_INFO_REG+M6800_NMI_STATE: sprintf(buffer[which], "NMI:%d", r->nmi_state); break;
		case CPU_INFO_REG+M6800_IRQ_STATE: sprintf(buffer[which], "IRQ:%d", r->irq_state[M6800_IRQ_LINE]); break;
		case CPU_INFO_REG+M6800_OCI_STATE: sprintf(buffer[which], "OCI:%d", r->irq_state[M6800_OCI_LINE]); break;
	}
	return buffer[which];
}

const char *m6800_info(void *context, int regnum)
{
	return m680x_info(6800,context,regnum);
}

/****************************************************************************
 * M6801 almost (fully?) equal to the M6803
 ****************************************************************************/
void m6801_reset(void *param) { m6800_reset(param); }
void m6801_exit(void) { m6800_exit(); }
int  m6801_execute(int cycles) { return m6803_execute(cycles); }
unsigned m6801_get_context(void *dst) { return m6800_get_context(dst); }
void m6801_set_context(void *src) { m6800_set_context(src); }
unsigned m6801_get_pc(void) { return m6800_get_pc(); }
void m6801_set_pc(unsigned val) { m6800_set_pc(val); }
unsigned m6801_get_sp(void) { return m6800_get_sp(); }
void m6801_set_sp(unsigned val) { m6800_set_sp(val); }
unsigned m6801_get_reg(int regnum) { return m6800_get_reg(regnum); }
void m6801_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void m6801_set_nmi_line(int state) { m6800_set_nmi_line(state); }
void m6801_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void m6801_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }
void m6801_state_save(void *file) { state_save(file,"m6801"); }
void m6801_state_load(void *file) { state_load(file,"m6801"); }
const char *m6801_info(void *context, int regnum) { return m680x_info(6801,context,regnum); }

/****************************************************************************
 * M6802 almost (fully?) equal to the M6800
 ****************************************************************************/
void m6802_reset(void *param) { m6800_reset(param); }
void m6802_exit(void) { m6800_exit(); }
int  m6802_execute(int cycles) { return m6800_execute(cycles); }
unsigned m6802_get_context(void *dst) { return m6800_get_context(dst); }
void m6802_set_context(void *src) { m6800_set_context(src); }
unsigned m6802_get_pc(void) { return m6800_get_pc(); }
void m6802_set_pc(unsigned val) { m6800_set_pc(val); }
unsigned m6802_get_sp(void) { return m6800_get_sp(); }
void m6802_set_sp(unsigned val) { m6800_set_sp(val); }
unsigned m6802_get_reg(int regnum) { return m6800_get_reg(regnum); }
void m6802_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void m6802_set_nmi_line(int state) { m6800_set_nmi_line(state); }
void m6802_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void m6802_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }
void m6802_state_save(void *file) { state_save(file,"m6802"); }
void m6802_state_load(void *file) { state_load(file,"m6802"); }
const char *m6802_info(void *context, int regnum) { return m680x_info(6802,context,regnum); }

/****************************************************************************
 * M6803 almost (fully?) equal to the M6801
 ****************************************************************************/
void m6803_reset(void *param) { m6800_reset(param); }
void m6803_exit(void) { m6800_exit(); }
/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
int m6803_execute(int cycles)
{
    UINT8 ireg;
    m6803_ICount = cycles;

    if( m6803.wai_state & M6800_WAI )
    {
        m6803_ICount = 0;
        goto getout;
    }

    do
    {

#if ASGARD680X_DEBUG
{
extern void Asgard63701MiniTrace(unsigned long sx, unsigned long pcab, unsigned long flags, int icount);
    Asgard63701MiniTrace ((S<<16)+X, (PC<<16)+(A<<8)+B, (wai_state<<16)+CC, m6803_ICount);
}
#endif

#if 0
        asg_6808Trace(OP_ROM, PC);
#endif

#ifdef  MAME_DEBUG
        {
            extern int mame_debug;
            if (mame_debug) MAME_Debug();
        }
#endif
        ireg=M_RDOP(PC++);

        switch( ireg )
        {
            case 0x00: illegal(); break;
            case 0x01: nop(); break;
            case 0x02: illegal(); break;
            case 0x03: illegal(); break;
            case 0x04: lsrd(); /* 6803 only */; break;
            case 0x05: asld(); /* 6803 only */; break;
            case 0x06: tap(); break;
            case 0x07: tpa(); break;
            case 0x08: inx(); break;
            case 0x09: dex(); break;
            case 0x0A: CLV; break;
            case 0x0B: SEV; break;
            case 0x0C: CLC; break;
            case 0x0D: SEC; break;
            case 0x0E: CLI; CHECK_IRQ_LINES; break;
            case 0x0F: SEI; break;
            case 0x10: sba(); break;
            case 0x11: cba(); break;
            case 0x12: illegal(); break;
            case 0x13: illegal(); break;
            case 0x14: illegal(); break;
            case 0x15: illegal(); break;
            case 0x16: tab(); break;
            case 0x17: tba(); break;
            case 0x18: illegal(); break;
            case 0x19: daa(); break;
            case 0x1a: illegal(); break;
            case 0x1b: aba(); break;
            case 0x1c: illegal(); break;
            case 0x1d: illegal(); break;
            case 0x1e: illegal(); break;
            case 0x1f: illegal(); break;
            case 0x20: bra(); break;
            case 0x21: brn(); break;
            case 0x22: bhi(); break;
            case 0x23: bls(); break;
            case 0x24: bcc(); break;
            case 0x25: bcs(); break;
            case 0x26: bne(); break;
            case 0x27: beq(); break;
            case 0x28: bvc(); break;
            case 0x29: bvs(); break;
            case 0x2a: bpl(); break;
            case 0x2b: bmi(); break;
            case 0x2c: bge(); break;
            case 0x2d: blt(); break;
            case 0x2e: bgt(); break;
            case 0x2f: ble(); break;
            case 0x30: tsx(); break;
            case 0x31: ins(); break;
            case 0x32: pula(); break;
            case 0x33: pulb(); break;
            case 0x34: des(); break;
            case 0x35: txs(); break;
            case 0x36: psha(); break;
            case 0x37: pshb(); break;
            case 0x38: pulx(); /* 6803 only */ break;
            case 0x39: rts(); break;
            case 0x3a: abx(); /* 6803 only */ break;
            case 0x3b: rti(); break;
            case 0x3c: pshx(); /* 6803 only */ break;
            case 0x3d: mul(); /* 6803 only */ break;
            case 0x3e: wai(); break;
            case 0x3f: swi(); break;
            case 0x40: nega(); break;
            case 0x41: illegal(); break;
            case 0x42: illegal(); break;
            case 0x43: coma(); break;
            case 0x44: lsra(); break;
            case 0x45: illegal(); break;
            case 0x46: rora(); break;
            case 0x47: asra(); break;
            case 0x48: asla(); break;
            case 0x49: rola(); break;
            case 0x4a: deca(); break;
            case 0x4b: illegal(); break;
            case 0x4c: inca(); break;
            case 0x4d: tsta(); break;
            case 0x4e: illegal(); break;
            case 0x4f: clra(); break;
            case 0x50: negb(); break;
            case 0x51: illegal(); break;
            case 0x52: illegal(); break;
            case 0x53: comb(); break;
            case 0x54: lsrb(); break;
            case 0x55: illegal(); break;
            case 0x56: rorb(); break;
            case 0x57: asrb(); break;
            case 0x58: aslb(); break;
            case 0x59: rolb(); break;
            case 0x5a: decb(); break;
            case 0x5b: illegal(); break;
            case 0x5c: incb(); break;
            case 0x5d: tstb(); break;
            case 0x5e: illegal(); break;
            case 0x5f: clrb(); break;
            case 0x60: neg_ix(); break;
            case 0x61: illegal(); break;
            case 0x62: illegal(); break;
            case 0x63: com_ix(); break;
            case 0x64: lsr_ix(); break;
            case 0x65: illegal(); break;
            case 0x66: ror_ix(); break;
            case 0x67: asr_ix(); break;
            case 0x68: asl_ix(); break;
            case 0x69: rol_ix(); break;
            case 0x6a: dec_ix(); break;
            case 0x6b: illegal(); break;
            case 0x6c: inc_ix(); break;
            case 0x6d: tst_ix(); break;
            case 0x6e: jmp_ix(); break;
            case 0x6f: clr_ix(); break;
            case 0x70: neg_ex(); break;
            case 0x71: illegal(); break;
            case 0x72: illegal(); break;
            case 0x73: com_ex(); break;
            case 0x74: lsr_ex(); break;
            case 0x75: illegal(); break;
            case 0x76: ror_ex(); break;
            case 0x77: asr_ex(); break;
            case 0x78: asl_ex(); break;
            case 0x79: rol_ex(); break;
            case 0x7a: dec_ex(); break;
            case 0x7b: illegal(); break;
            case 0x7c: inc_ex(); break;
            case 0x7d: tst_ex(); break;
            case 0x7e: jmp_ex(); break;
            case 0x7f: clr_ex(); break;
            case 0x80: suba_im(); break;
            case 0x81: cmpa_im(); break;
            case 0x82: sbca_im(); break;
            case 0x83: subd_im(); /* 6803 only */ break;
            case 0x84: anda_im(); break;
            case 0x85: bita_im(); break;
            case 0x86: lda_im(); break;
            case 0x87: sta_im(); break;
            case 0x88: eora_im(); break;
            case 0x89: adca_im(); break;
            case 0x8a: ora_im(); break;
            case 0x8b: adda_im(); break;
            case 0x8c: cmpx_im(); break;
            case 0x8d: bsr(); break;
            case 0x8e: lds_im(); break;
            case 0x8f: sts_im(); /* orthogonality */ break;
            case 0x90: suba_di(); break;
            case 0x91: cmpa_di(); break;
            case 0x92: sbca_di(); break;
            case 0x93: subd_di(); /* 6803 only */ break;
            case 0x94: anda_di(); break;
            case 0x95: bita_di(); break;
            case 0x96: lda_di(); break;
            case 0x97: sta_di(); break;
            case 0x98: eora_di(); break;
            case 0x99: adca_di(); break;
            case 0x9a: ora_di(); break;
            case 0x9b: adda_di(); break;
            case 0x9c: cmpx_di(); break;
            case 0x9d: jsr_di(); break;
            case 0x9e: lds_di(); break;
            case 0x9f: sts_di(); break;
            case 0xa0: suba_ix(); break;
            case 0xa1: cmpa_ix(); break;
            case 0xa2: sbca_ix(); break;
            case 0xa3: subd_ix(); /* 6803 only */ break;
            case 0xa4: anda_ix(); break;
            case 0xa5: bita_ix(); break;
            case 0xa6: lda_ix(); break;
            case 0xa7: sta_ix(); break;
            case 0xa8: eora_ix(); break;
            case 0xa9: adca_ix(); break;
            case 0xaa: ora_ix(); break;
            case 0xab: adda_ix(); break;
            case 0xac: cmpx_ix(); break;
            case 0xad: jsr_ix(); break;
            case 0xae: lds_ix(); break;
            case 0xaf: sts_ix(); break;
            case 0xb0: suba_ex(); break;
            case 0xb1: cmpa_ex(); break;
            case 0xb2: sbca_ex(); break;
            case 0xb3: subd_ex(); /* 6803 only */ break;
            case 0xb4: anda_ex(); break;
            case 0xb5: bita_ex(); break;
            case 0xb6: lda_ex(); break;
            case 0xb7: sta_ex(); break;
            case 0xb8: eora_ex(); break;
            case 0xb9: adca_ex(); break;
            case 0xba: ora_ex(); break;
            case 0xbb: adda_ex(); break;
            case 0xbc: cmpx_ex(); break;
            case 0xbd: jsr_ex(); break;
            case 0xbe: lds_ex(); break;
            case 0xbf: sts_ex(); break;
            case 0xc0: subb_im(); break;
            case 0xc1: cmpb_im(); break;
            case 0xc2: sbcb_im(); break;
            case 0xc3: addd_im(); /* 6803 only */ break;
            case 0xc4: andb_im(); break;
            case 0xc5: bitb_im(); break;
            case 0xc6: ldb_im(); break;
            case 0xc7: stb_im(); break;
            case 0xc8: eorb_im(); break;
            case 0xc9: adcb_im(); break;
            case 0xca: orb_im(); break;
            case 0xcb: addb_im(); break;
            case 0xcc: ldd_im(); /* 6803 only */ break;
            case 0xcd: std_im(); /* 6803 only -- orthogonality */ break;
            case 0xce: ldx_im(); break;
            case 0xcf: stx_im(); break;
            case 0xd0: subb_di(); break;
            case 0xd1: cmpb_di(); break;
            case 0xd2: sbcb_di(); break;
            case 0xd3: addd_di(); /* 6803 only */ break;
            case 0xd4: andb_di(); break;
            case 0xd5: bitb_di(); break;
            case 0xd6: ldb_di(); break;
            case 0xd7: stb_di(); break;
            case 0xd8: eorb_di(); break;
            case 0xd9: adcb_di(); break;
            case 0xda: orb_di(); break;
            case 0xdb: addb_di(); break;
            case 0xdc: ldd_di(); /* 6803 only */ break;
            case 0xdd: std_di(); /* 6803 only */ break;
            case 0xde: ldx_di(); break;
            case 0xdf: stx_di(); break;
            case 0xe0: subb_ix(); break;
            case 0xe1: cmpb_ix(); break;
            case 0xe2: sbcb_ix(); break;
            case 0xe3: addd_ix(); /* 6803 only */ break;
            case 0xe4: andb_ix(); break;
            case 0xe5: bitb_ix(); break;
            case 0xe6: ldb_ix(); break;
            case 0xe7: stb_ix(); break;
            case 0xe8: eorb_ix(); break;
            case 0xe9: adcb_ix(); break;
            case 0xea: orb_ix(); break;
            case 0xeb: addb_ix(); break;
            case 0xec: ldd_ix(); /* 6803 only */ break;
            case 0xed: std_ix(); /* 6803 only */ break;
            case 0xee: ldx_ix(); break;
            case 0xef: stx_ix(); break;
            case 0xf0: subb_ex(); break;
            case 0xf1: cmpb_ex(); break;
            case 0xf2: sbcb_ex(); break;
            case 0xf3: addd_ex(); /* 6803 only */ break;
            case 0xf4: andb_ex(); break;
            case 0xf5: bitb_ex(); break;
            case 0xf6: ldb_ex(); break;
            case 0xf7: stb_ex(); break;
            case 0xf8: eorb_ex(); break;
            case 0xf9: adcb_ex(); break;
            case 0xfa: orb_ex(); break;
            case 0xfb: addb_ex(); break;
            case 0xfc: ldd_ex(); /* 6803 only */ break;
            case 0xfd: std_ex(); /* 6803 only */ break;
            case 0xfe: ldx_ex(); break;
            case 0xff: stx_ex(); break;
        }
		m6803_ICount -= cycles_6803[ireg];
    } while( m6803_ICount>0 );

getout:
    return cycles - m6803_ICount;
}
unsigned m6803_get_context(void *dst) { return m6800_get_context(dst); }
void m6803_set_context(void *src) { m6800_set_context(src); }
unsigned m6803_get_pc(void) { return m6800_get_pc(); }
void m6803_set_pc(unsigned val) { m6800_set_pc(val); }
unsigned m6803_get_sp(void) { return m6800_get_sp(); }
void m6803_set_sp(unsigned val) { m6800_set_sp(val); }
unsigned m6803_get_reg(int regnum) { return m6800_get_reg(regnum); }
void m6803_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void m6803_set_nmi_line(int state) { m6800_set_nmi_line(state); }
void m6803_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void m6803_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }
void m6803_state_save(void *file) { state_save(file,"m6803"); }
void m6803_state_load(void *file) { state_load(file,"m6803"); }
const char *m6803_info(void *context, int regnum) { return m680x_info(6803,context,regnum); }

/****************************************************************************
 * M6808 almost (fully?) equal to the M6800
 ****************************************************************************/
void m6808_reset(void *param) { m6800_reset(param); }
void m6808_exit(void) { m6800_exit(); }
int  m6808_execute(int cycles) { return m6800_execute(cycles); }
unsigned m6808_get_context(void *dst) { return m6800_get_context(dst); }
void m6808_set_context(void *src) { m6800_set_context(src); }
unsigned m6808_get_pc(void) { return m6800_get_pc(); }
void m6808_set_pc(unsigned val) { m6800_set_pc(val); }
unsigned m6808_get_sp(void) { return m6800_get_sp(); }
void m6808_set_sp(unsigned val) { m6800_set_sp(val); }
unsigned m6808_get_reg(int regnum) { return m6800_get_reg(regnum); }
void m6808_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void m6808_set_nmi_line(int state) { m6800_set_nmi_line(state); }
void m6808_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void m6808_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }
void m6808_state_save(void *file) { state_save(file,"m6808"); }
void m6808_state_load(void *file) { state_load(file,"m6808"); }
const char *m6808_info(void *context, int regnum) { return m680x_info(6808,context,regnum); }

/****************************************************************************
 * HD63701 similiar to the M6800
 ****************************************************************************/
void hd63701_reset(void *param) { m6800_reset(param); }
void hd63701_exit(void) { m6800_exit(); }
/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
int hd63701_execute(int cycles)
{
    UINT8 ireg;
    hd63701_ICount = cycles;

    if( hd63701.wai_state & M6808_WAI )
    {
        hd63701_ICount = 0;
        goto getout;
    }

    do
    {

#if ASGARD680X_DEBUG
{
extern void Asgard63701MiniTrace(unsigned long sx, unsigned long pcab, unsigned long flags, int icount);
    Asgard63701MiniTrace ((S<<16)+X, (PC<<16)+(A<<8)+B, (wai_state<<16)+CC, hd63701_ICount);
}
#endif

#if 0
        asg_6808Trace(OP_ROM, PC);
#endif

#ifdef  MAME_DEBUG
        {
            extern int mame_debug;
            if (mame_debug) MAME_Debug();
        }
#endif
        ireg=M_RDOP(PC++);

        switch( ireg )
        {
            case 0x00: illegal(); break;
            case 0x01: nop(); break;
            case 0x02: illegal(); break;
            case 0x03: illegal(); break;
            case 0x04: lsrd(); /* 6803 only */; break;
            case 0x05: asld(); /* 6803 only */; break;
            case 0x06: tap(); break;
            case 0x07: tpa(); break;
            case 0x08: inx(); break;
            case 0x09: dex(); break;
            case 0x0A: CLV; break;
            case 0x0B: SEV; break;
            case 0x0C: CLC; break;
            case 0x0D: SEC; break;
            case 0x0E: CLI; CHECK_IRQ_LINES; break;
            case 0x0F: SEI; break;
            case 0x10: sba(); break;
            case 0x11: cba(); break;
			case 0x12: undoc1(); break;
			case 0x13: undoc2(); break;
            case 0x14: illegal(); break;
            case 0x15: illegal(); break;
            case 0x16: tab(); break;
            case 0x17: tba(); break;
            case 0x18: xgdx(); /* HD63701YO only */; break;
            case 0x19: daa(); break;
            case 0x1a: illegal(); break;
            case 0x1b: aba(); break;
            case 0x1c: illegal(); break;
            case 0x1d: illegal(); break;
            case 0x1e: illegal(); break;
            case 0x1f: illegal(); break;
            case 0x20: bra(); break;
            case 0x21: brn(); break;
            case 0x22: bhi(); break;
            case 0x23: bls(); break;
            case 0x24: bcc(); break;
            case 0x25: bcs(); break;
            case 0x26: bne(); break;
            case 0x27: beq(); break;
            case 0x28: bvc(); break;
            case 0x29: bvs(); break;
            case 0x2a: bpl(); break;
            case 0x2b: bmi(); break;
            case 0x2c: bge(); break;
            case 0x2d: blt(); break;
            case 0x2e: bgt(); break;
            case 0x2f: ble(); break;
            case 0x30: tsx(); break;
            case 0x31: ins(); break;
            case 0x32: pula(); break;
            case 0x33: pulb(); break;
            case 0x34: des(); break;
            case 0x35: txs(); break;
            case 0x36: psha(); break;
            case 0x37: pshb(); break;
            case 0x38: pulx(); /* 6803 only */ break;
            case 0x39: rts(); break;
            case 0x3a: abx(); /* 6803 only */ break;
            case 0x3b: rti(); break;
            case 0x3c: pshx(); /* 6803 only */ break;
            case 0x3d: mul(); /* 6803 only */ break;
            case 0x3e: wai(); break;
            case 0x3f: swi(); break;
            case 0x40: nega(); break;
            case 0x41: illegal(); break;
            case 0x42: illegal(); break;
            case 0x43: coma(); break;
            case 0x44: lsra(); break;
            case 0x45: illegal(); break;
            case 0x46: rora(); break;
            case 0x47: asra(); break;
            case 0x48: asla(); break;
            case 0x49: rola(); break;
            case 0x4a: deca(); break;
            case 0x4b: illegal(); break;
            case 0x4c: inca(); break;
            case 0x4d: tsta(); break;
            case 0x4e: illegal(); break;
            case 0x4f: clra(); break;
            case 0x50: negb(); break;
            case 0x51: illegal(); break;
            case 0x52: illegal(); break;
            case 0x53: comb(); break;
            case 0x54: lsrb(); break;
            case 0x55: illegal(); break;
            case 0x56: rorb(); break;
            case 0x57: asrb(); break;
            case 0x58: aslb(); break;
            case 0x59: rolb(); break;
            case 0x5a: decb(); break;
            case 0x5b: illegal(); break;
            case 0x5c: incb(); break;
            case 0x5d: tstb(); break;
            case 0x5e: illegal(); break;
            case 0x5f: clrb(); break;
            case 0x60: neg_ix(); break;
            case 0x61: aim_ix(); /* HD63701YO only */; break;
            case 0x62: oim_ix(); /* HD63701YO only */; break;
            case 0x63: com_ix(); break;
            case 0x64: lsr_ix(); break;
            case 0x65: eim_ix(); /* HD63701YO only */; break;
            case 0x66: ror_ix(); break;
            case 0x67: asr_ix(); break;
            case 0x68: asl_ix(); break;
            case 0x69: rol_ix(); break;
            case 0x6a: dec_ix(); break;
            case 0x6b: tim_ix(); /* HD63701YO only */; break;
            case 0x6c: inc_ix(); break;
            case 0x6d: tst_ix(); break;
            case 0x6e: jmp_ix(); break;
            case 0x6f: clr_ix(); break;
            case 0x70: neg_ex(); break;
            case 0x71: aim_di(); /* HD63701YO only */; break;
            case 0x72: oim_di(); /* HD63701YO only */; break;
            case 0x73: com_ex(); break;
            case 0x74: lsr_ex(); break;
            case 0x75: eim_di(); /* HD63701YO only */; break;
            case 0x76: ror_ex(); break;
            case 0x77: asr_ex(); break;
            case 0x78: asl_ex(); break;
            case 0x79: rol_ex(); break;
            case 0x7a: dec_ex(); break;
            case 0x7b: tim_di(); /* HD63701YO only */; break;
            case 0x7c: inc_ex(); break;
            case 0x7d: tst_ex(); break;
            case 0x7e: jmp_ex(); break;
            case 0x7f: clr_ex(); break;
            case 0x80: suba_im(); break;
            case 0x81: cmpa_im(); break;
            case 0x82: sbca_im(); break;
            case 0x83: subd_im(); /* 6803 only */ break;
            case 0x84: anda_im(); break;
            case 0x85: bita_im(); break;
            case 0x86: lda_im(); break;
            case 0x87: sta_im(); break;
            case 0x88: eora_im(); break;
            case 0x89: adca_im(); break;
            case 0x8a: ora_im(); break;
            case 0x8b: adda_im(); break;
            case 0x8c: cmpx_im(); break;
            case 0x8d: bsr(); break;
            case 0x8e: lds_im(); break;
            case 0x8f: sts_im(); /* orthogonality */ break;
            case 0x90: suba_di(); break;
            case 0x91: cmpa_di(); break;
            case 0x92: sbca_di(); break;
            case 0x93: subd_di(); /* 6803 only */ break;
            case 0x94: anda_di(); break;
            case 0x95: bita_di(); break;
            case 0x96: lda_di(); break;
            case 0x97: sta_di(); break;
            case 0x98: eora_di(); break;
            case 0x99: adca_di(); break;
            case 0x9a: ora_di(); break;
            case 0x9b: adda_di(); break;
            case 0x9c: cmpx_di(); break;
            case 0x9d: jsr_di(); break;
            case 0x9e: lds_di(); break;
            case 0x9f: sts_di(); break;
            case 0xa0: suba_ix(); break;
            case 0xa1: cmpa_ix(); break;
            case 0xa2: sbca_ix(); break;
            case 0xa3: subd_ix(); /* 6803 only */ break;
            case 0xa4: anda_ix(); break;
            case 0xa5: bita_ix(); break;
            case 0xa6: lda_ix(); break;
            case 0xa7: sta_ix(); break;
            case 0xa8: eora_ix(); break;
            case 0xa9: adca_ix(); break;
            case 0xaa: ora_ix(); break;
            case 0xab: adda_ix(); break;
            case 0xac: cmpx_ix(); break;
            case 0xad: jsr_ix(); break;
            case 0xae: lds_ix(); break;
            case 0xaf: sts_ix(); break;
            case 0xb0: suba_ex(); break;
            case 0xb1: cmpa_ex(); break;
            case 0xb2: sbca_ex(); break;
            case 0xb3: subd_ex(); /* 6803 only */ break;
            case 0xb4: anda_ex(); break;
            case 0xb5: bita_ex(); break;
            case 0xb6: lda_ex(); break;
            case 0xb7: sta_ex(); break;
            case 0xb8: eora_ex(); break;
            case 0xb9: adca_ex(); break;
            case 0xba: ora_ex(); break;
            case 0xbb: adda_ex(); break;
            case 0xbc: cmpx_ex(); break;
            case 0xbd: jsr_ex(); break;
            case 0xbe: lds_ex(); break;
            case 0xbf: sts_ex(); break;
            case 0xc0: subb_im(); break;
            case 0xc1: cmpb_im(); break;
            case 0xc2: sbcb_im(); break;
            case 0xc3: addd_im(); /* 6803 only */ break;
            case 0xc4: andb_im(); break;
            case 0xc5: bitb_im(); break;
            case 0xc6: ldb_im(); break;
            case 0xc7: stb_im(); break;
            case 0xc8: eorb_im(); break;
            case 0xc9: adcb_im(); break;
            case 0xca: orb_im(); break;
            case 0xcb: addb_im(); break;
            case 0xcc: ldd_im(); /* 6803 only */ break;
            case 0xcd: std_im(); /* 6803 only -- orthogonality */ break;
            case 0xce: ldx_im(); break;
            case 0xcf: stx_im(); break;
            case 0xd0: subb_di(); break;
            case 0xd1: cmpb_di(); break;
            case 0xd2: sbcb_di(); break;
            case 0xd3: addd_di(); /* 6803 only */ break;
            case 0xd4: andb_di(); break;
            case 0xd5: bitb_di(); break;
            case 0xd6: ldb_di(); break;
            case 0xd7: stb_di(); break;
            case 0xd8: eorb_di(); break;
            case 0xd9: adcb_di(); break;
            case 0xda: orb_di(); break;
            case 0xdb: addb_di(); break;
            case 0xdc: ldd_di(); /* 6803 only */ break;
            case 0xdd: std_di(); /* 6803 only */ break;
            case 0xde: ldx_di(); break;
            case 0xdf: stx_di(); break;
            case 0xe0: subb_ix(); break;
            case 0xe1: cmpb_ix(); break;
            case 0xe2: sbcb_ix(); break;
            case 0xe3: addd_ix(); /* 6803 only */ break;
            case 0xe4: andb_ix(); break;
            case 0xe5: bitb_ix(); break;
            case 0xe6: ldb_ix(); break;
            case 0xe7: stb_ix(); break;
            case 0xe8: eorb_ix(); break;
            case 0xe9: adcb_ix(); break;
            case 0xea: orb_ix(); break;
            case 0xeb: addb_ix(); break;
            case 0xec: ldd_ix(); /* 6803 only */ break;
            case 0xed: std_ix(); /* 6803 only */ break;
            case 0xee: ldx_ix(); break;
            case 0xef: stx_ix(); break;
            case 0xf0: subb_ex(); break;
            case 0xf1: cmpb_ex(); break;
            case 0xf2: sbcb_ex(); break;
            case 0xf3: addd_ex(); /* 6803 only */ break;
            case 0xf4: andb_ex(); break;
            case 0xf5: bitb_ex(); break;
            case 0xf6: ldb_ex(); break;
            case 0xf7: stb_ex(); break;
            case 0xf8: eorb_ex(); break;
            case 0xf9: adcb_ex(); break;
            case 0xfa: orb_ex(); break;
            case 0xfb: addb_ex(); break;
            case 0xfc: ldd_ex(); /* 6803 only */ break;
            case 0xfd: std_ex(); /* 6803 only */ break;
            case 0xfe: ldx_ex(); break;
            case 0xff: stx_ex(); break;
        }
		hd63701_ICount -= cycles_63701[ireg];
    } while( hd63701_ICount>0 );

getout:
    return cycles - hd63701_ICount;
}

unsigned hd63701_get_context(void *dst) { return m6800_get_context(dst); }
void hd63701_set_context(void *src) { m6800_set_context(src); }
unsigned hd63701_get_pc(void) { return m6800_get_pc(); }
void hd63701_set_pc(unsigned val) { m6800_set_pc(val); }
unsigned hd63701_get_sp(void) { return m6800_get_sp(); }
void hd63701_set_sp(unsigned val) { m6800_set_sp(val); }
unsigned hd63701_get_reg(int regnum) { return m6800_get_reg(regnum); }
void hd63701_set_reg(int regnum, unsigned val) { m6800_set_reg(regnum,val); }
void hd63701_set_nmi_line(int state) { m6800_set_nmi_line(state); }
void hd63701_set_irq_line(int irqline, int state) { m6800_set_irq_line(irqline,state); }
void hd63701_set_irq_callback(int (*callback)(int irqline)) { m6800_set_irq_callback(callback); }
void hd63701_state_save(void *file) { state_save(file,"hd63701"); }
void hd63701_state_load(void *file) { state_load(file,"hd63701"); }
const char *hd63701_info(void *context, int regnum) { return m680x_info(63701,context,regnum); }

