/*** m6809: Portable 6809 emulator ******************************************

	Copyright (C) John Butler 1997

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

	History:
990312 HJB:
	Added bugfixes according to Aaron's findings.
	Reset only sets CC_II and CC_IF, DP to zero and PC from reset vector.
990311 HJB:
	Added _info functions. Now uses static m6808_Regs struct instead
	of single statics. Changed the 16 bit registers to use the generic
	PAIR union. Registers defined using macros. Split the core into
	four execution loops for M6802, M6803, M6808 and HD63701.
    TST, TSTA and TSTB opcodes reset carry flag.
	Modified the read/write stack handlers to push LSB first then MSB
	and pull MSB first then LSB.

990228 HJB:
	Changed the interrupt handling again. Now interrupts are taken
	either right at the moment the lines are asserted or whenever
	an interrupt is enabled and the corresponding line is still
	asserted. That way the pending_interrupts checks are not
	needed anymore. However, the CWAI and SYNC flags still need
	some flags, so I changed the name to 'int_state'.
	This core also has the code for the old interrupt system removed.

990225 HJB:
	Cleaned up the code here and there, added some comments.
	Slightly changed the SAR opcodes (similiar to other CPU cores).
	Added symbolic names for the flag bits.
	Changed the way CWAI/Interrupt() handle CPU state saving.
	A new flag M6809_STATE in pending_interrupts is used to determine
	if a state save is needed on interrupt entry or already done by CWAI.
	Added M6809_IRQ_LINE and M6809_FIRQ_LINE defines to m6809.h
	Moved the internal interrupt_pending flags from m6809.h to m6809.c
	Changed CWAI cycles2[0x3c] to be 2 (plus all or at least 19 if
	CWAI actually pushes the entire state).
	Implemented undocumented TFR/EXG for undefined source and mixed 8/16
	bit transfers (they should transfer/exchange the constant $ff).
	Removed unused jmp/jsr _slap functions from 6809ops.c,
	m6809_slapstick check moved into the opcode functions.

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "osd_dbg.h"
#include "cpuintrf.h"
#include "state.h"
#include "M6809.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog )	fprintf x
#else
#define LOG(x)
#endif

extern FILE *errorlog;

INLINE UINT8 fetch_effective_address( void );

/* 6809 Registers */
typedef struct
{
    PAIR    pc;     /* Program counter */
    PAIR    u, s;   /* Stack pointers */
    PAIR    x, y;   /* Index registers */
    PAIR    d;      /* Accumulatora a and b */
    UINT8   dp;     /* Direct Page register */
    UINT8   cc;
	UINT8	int_state;	/* SYNC and CWAI flags */
	UINT8	nmi_state;
	UINT8	irq_state[2];
    int     (*irq_callback)(int irqline);
    int     extra_cycles; /* cycles used up by interrupts */
} m6809_Regs;

/* flag bits in the cc register */
#define CC_C    0x01        /* Carry */
#define CC_V    0x02        /* Overflow */
#define CC_Z    0x04        /* Zero */
#define CC_N    0x08        /* Negative */
#define CC_II   0x10        /* Inhibit IRQ */
#define CC_H    0x20        /* Half (auxiliary) carry */
#define CC_IF   0x40        /* Inhibit FIRQ */
#define CC_E    0x80        /* entire state pushed */

/* 6809 registers */
static m6809_Regs m6809;

#define pPC m6809.pc
#define pU	m6809.u
#define pS	m6809.s
#define pX	m6809.x
#define pY	m6809.y
#define pD	m6809.d

#define PC  m6809.pc.w.l
#define U	m6809.u.w.l
#define S	m6809.s.w.l
#define X	m6809.x.w.l
#define Y	m6809.y.w.l
#define D   m6809.d.w.l
#define A   m6809.d.b.h
#define B	m6809.d.b.l
#define DP	m6809.dp
#define CC  m6809.cc

static PAIR ea;         /* effective address */
#define EA	ea.w.l
#define EAD ea.d

/* NS 980101 */
#define M6809_CWAI		8	/* set when CWAI is waiting for an interrupt */
#define M6809_SYNC		16	/* set when SYNC is waiting for an interrupt */
#define M6809_LDS		32	/* set when LDS occured at least once */

#define CHECK_IRQ_LINES 												\
	if( m6809.irq_state[M6809_IRQ_LINE] != CLEAR_LINE ||				\
		m6809.irq_state[M6809_FIRQ_LINE] != CLEAR_LINE )				\
		m6809.int_state &= ~M6809_SYNC; /* clear SYNC flag */			\
	if( m6809.irq_state[M6809_FIRQ_LINE]!=CLEAR_LINE && !(CC & CC_IF) ) \
	{																	\
		/* fast IRQ */													\
		/* HJB 990225: state already saved by CWAI? */					\
		if( m6809.int_state & M6809_CWAI )								\
		{																\
			m6809.int_state &= ~M6809_CWAI;  /* clear CWAI */			\
			m6809.extra_cycles += 7;		 /* subtract +7 cycles */	\
        }                                                               \
		else															\
		{																\
			CC &= ~CC_E;				/* save 'short' state */        \
			PUSHWORD(pPC);												\
			PUSHBYTE(CC);												\
			m6809.extra_cycles += 10;	/* subtract +10 cycles */		\
		}																\
		CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */		\
		RM16(0xfff6,&m6809.pc); 										\
		change_pc(PC);					/* TS 971002 */ 				\
		(void)(*m6809.irq_callback)(M6809_FIRQ_LINE);					\
	}																	\
	else																\
	if( m6809.irq_state[M6809_IRQ_LINE]!=CLEAR_LINE && !(CC & CC_II) )	\
	{																	\
		/* standard IRQ */												\
		/* HJB 990225: state already saved by CWAI? */					\
		if( m6809.int_state & M6809_CWAI )								\
		{																\
			m6809.int_state &= ~M6809_CWAI;  /* clear CWAI flag */		\
			m6809.extra_cycles += 7;		 /* subtract +7 cycles */	\
		}																\
		else															\
		{																\
			CC |= CC_E; 				/* save entire state */ 		\
			PUSHWORD(pPC);												\
			PUSHWORD(pU);												\
			PUSHWORD(pY);												\
			PUSHWORD(pX);												\
			PUSHBYTE(DP);												\
			PUSHBYTE(B);												\
			PUSHBYTE(A);												\
			PUSHBYTE(CC);												\
			m6809.extra_cycles += 19;	 /* subtract +19 cycles */		\
		}																\
		CC |= CC_II;					/* inhibit IRQ */				\
		RM16(0xfff8,&m6809.pc); 										\
		change_pc(PC);					/* TS 971002 */ 				\
		(void)(*m6809.irq_callback)(M6809_IRQ_LINE);					\
	}

/* public globals */
int m6809_ICount=50000;
int m6809_Flags;	/* flags for speed optimization */
int m6809_slapstic;

/* flag, handlers for speed optimization */
static void (*rd_u_handler_b)(UINT8 *r);
static void (*rd_u_handler_w)(PAIR *r);
static void (*rd_s_handler_b)(UINT8 *r);
static void (*rd_s_handler_w)(PAIR *r);
static void (*wr_u_handler_b)(UINT8 *r);
static void (*wr_u_handler_w)(PAIR *r);
static void (*wr_s_handler_b)(UINT8 *r);
static void (*wr_s_handler_w)(PAIR *r);

/* these are re-defined in m6809.h TO RAM, ROM or functions in cpuintrf.c */
#define RM(Addr)			M6809_RDMEM(Addr)
#define WM(Addr,Value)		M6809_WRMEM(Addr,Value)
#define M_RDOP(Addr)		M6809_RDOP(Addr)
#define M_RDOP_ARG(Addr)	M6809_RDOP_ARG(Addr)

/* macros to access memory */
#define IMMBYTE(b)	{b = M_RDOP_ARG(PC++);}
#define IMMWORD(w)	{w.d = 0; w.b.h = M_RDOP_ARG(PC); w.b.l = M_RDOP_ARG(PC+1); PC+=2;}

/* pre-clear a PAIR union; clearing h2 and h3 only might be faster? */
#define CLEAR_PAIR(p)   p->d = 0

#define PUSHBYTE(b) (*wr_s_handler_b)(&b)
#define PUSHWORD(w) (*wr_s_handler_w)(&w)
#define PULLBYTE(b) (*rd_s_handler_b)(&b)
#define PULLWORD(w) (*rd_s_handler_w)(&w)
#define PSHUBYTE(b) (*wr_u_handler_b)(&b)
#define PSHUWORD(w) (*wr_u_handler_w)(&w)
#define PULUBYTE(b) (*rd_u_handler_b)(&b)
#define PULUWORD(w) (*rd_u_handler_w)(&w)

#define CLR_HNZVC	CC&=~(CC_H|CC_N|CC_Z|CC_V|CC_C)
#define CLR_NZV 	CC&=~(CC_N|CC_Z|CC_V)
#define CLR_HNZC	CC&=~(CC_H|CC_N|CC_Z|CC_C)
#define CLR_NZVC	CC&=~(CC_N|CC_Z|CC_V|CC_C)
#define CLR_Z		CC&=~(CC_Z)
#define CLR_NZC 	CC&=~(CC_N|CC_Z|CC_C)
#define CLR_ZC		CC&=~(CC_Z|CC_C)

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
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
CC_N|CC_V,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
static UINT8 flags8d[256]= /* decrement */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,CC_V,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
#define SET_FLAGS8I(a)		{CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an unsigned byte as a signed word */
#define SIGNED(b) ((UINT16)(b&0x80?b|0xff00:b))

/* macros for addressing modes (postbytes have their own code) */
#define DIRECT { ea.d=0; IMMBYTE(ea.b.l); ea.b.h=DP; }
#define IMM8 EA=PC++
#define IMM16 { EA=PC; PC+=2; }
#define EXTENDED IMMWORD(ea)

/* macros to set status flags */
#define SEC CC|=CC_C
#define CLC CC&=~CC_C
#define SEZ CC|=CC_Z
#define CLZ CC&=~CC_Z
#define SEN CC|=CC_N
#define CLN CC&=~CC_N
#define SEV CC|=CC_V
#define CLV CC&=~CC_V
#define SEH CC|=CC_H
#define CLH CC&=~CC_H

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;RM16(EAD,&w);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;RM16(EAD,&w);}

/* macros for branch instructions */
#define BRANCH(f) { 					\
	UINT8 t;							\
	IMMBYTE(t); 						\
	if( f ) 							\
	{									\
		PC += SIGNED(t);				\
		change_pc(PC);	/* TS 971002 */ \
	}									\
}

#define LBRANCH(f) {                    \
	PAIR t; 							\
	IMMWORD(t); 						\
	if( f ) 							\
	{									\
		m6809_ICount -= 1;				\
		PC += t.w.l;					\
		change_pc(PC);	/* TS 971002 */ \
	}									\
}

#define NXORV  ((CC&CC_N)^((CC&CC_V)<<2))

/* macros for setting/getting registers in TFR/EXG instructions */
#define GETREG(val,reg) 		\
	switch(reg) {				\
	case 0: val = D;	break;	\
	case 1: val = X; 	break;	\
	case 2: val = Y; 	break;	\
	case 3: val = U; 	break;	\
	case 4: val = S; 	break;	\
	case 5: val = PC;	break;	\
	case 8: val = A; 	break;	\
	case 9: val = B; 	break;	\
	case 10: val = CC;	break;	\
	case 11: val = DP;	break;	\
	default: val = 0xff; /* HJB 990225 */ \
}

#define SETREG(val,reg) 		\
	switch(reg) {				\
	case 0: D = val;	break;	\
	case 1: X = val; 	break;	\
	case 2: Y = val; 	break;	\
	case 3: U = val; 	break;	\
	case 4: S = val;			\
		m6809.int_state |= M6809_LDS; \
        break;                  \
	case 5: PC = val;			\
		change_pc(PC);			\
		break;	/* TS 971002 */ \
	case 8: A = val; 	break;	\
	case 9: B = val; 	break;	\
	case 10: CC = val;			\
		CHECK_IRQ_LINES;		\
		break;	/* HJB 990116 */\
	case 11: DP = val;	break;	\
}

#define EOP 0xff			/* 0xff = extend op code          */
#define E   0x80			/* 0x80 = fetch effective address */
/* timings for 1-byte opcodes */
static UINT8 cycles1[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  6,  0,  0,  6,  6,  0,  6,  6,  6,  6,  6,  0,  6,  6,  3,  6,
  /*1*/ 255,255,  2,  4,  0,  0,  5,  9,  0,  2,  3,  0,  3,  2,  8,  6,
  /*2*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
  /*3*/ E+4,E+4,E+4,E+4,  5,  5,  5,  5,  0,  5,  3,  6, 20, 11,  0, 19,
  /*4*/	  2,  0,  0,  2,  2,  0,  2,  2,  2,  2,  2,  0,  2,  2,  0,  2,
  /*5*/	  2,  0,  0,  2,  2,  0,  2,  2,  2,  2,  2,  0,  2,  2,  0,  2,
  /*6*/	E+6,E+0,E+0,E+6,E+6,E+0,E+6,E+6,E+6,E+6,E+6,E+0,E+6,E+6,E+3,E+6,
  /*7*/	  7,  0,  0,  7,  7,  0,  7,  7,  7,  7,  7,  0,  7,  7,  4,  7,
  /*8*/	  2,  2,  2,  4,  2,  2,  2,  2,  2,  2,  2,  2,  4,  7,  3,  0,
  /*9*/	  4,  4,  4,  6,  4,  4,  4,  4,  4,  4,  4,  4,  6,  7,  5,  5,
  /*A*/	E+4,E+4,E+4,E+6,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+6,E+7,E+5,E+5,
  /*B*/	  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  5,  7,  8,  6,  6,
  /*C*/	  2,  2,  2,  4,  2,  2,  2,  2,  2,  2,  2,  2,  3,  0,  3,  3,
  /*D*/	  4,  4,  4,  6,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,
  /*E*/	E+4,E+4,E+4,E+6,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+5,E+5,E+5,E+5,
  /*F*/	  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6
};

/* timings for 2-byte opcodes */
static UINT8 cycles2[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*1*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*2*/   0,  5,  5,  5,  5,  5,  6,  5,  5,  5,  5,  5,  5,  5,  5,  5,
  /*3*/ E+0,E+0,E+0,E+0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0, 20,
  /*4*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*5*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*6*/	E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,
  /*7*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*8*/	  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  4,  0,
  /*9*/	  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  7,  0,  6,  6,
  /*A*/	E+0,E+0,E+0,E+7,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+7,E+0,E+6,E+6,
  /*B*/	  0,  0,  0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  7,  7,
  /*C*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,
  /*D*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,
  /*E*/	E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+6,E+6,
  /*F*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  7,  7
};
#undef EOP
#undef E


static void rd_s_slow_b( UINT8 *b )
{
	*b = RM( m6809.s.d );
	m6809.s.w.l++;
}

static void rd_s_slow_w( PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM( m6809.s.d );
    m6809.s.w.l++;
	p->b.l = RM( m6809.s.d );
    m6809.s.w.l++;
}

static void rd_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	*b = RAM[ m6809.s.d ];
    m6809.s.w.l++;
}

static void rd_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	CLEAR_PAIR(p);
    p->b.h = RAM[ m6809.s.d ];
    m6809.s.w.l++;
	p->b.l = RAM[ m6809.s.d ];
    m6809.s.w.l++;
}

static void wr_s_slow_b( UINT8 *b )
{
	--m6809.s.w.l;
	WM( m6809.s.d, *b );
}

static void wr_s_slow_w( PAIR *p )
{
	--m6809.s.w.l;
	WM( m6809.s.d, p->b.l );
	--m6809.s.w.l;
	WM( m6809.s.d, p->b.h );
}

static void wr_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	--m6809.s.w.l;
	RAM[ m6809.s.d ] = *b;
}

static void wr_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	--m6809.s.w.l;
	RAM[ m6809.s.d ] = p->b.l;
	--m6809.s.w.l;
	RAM[ m6809.s.d ] = p->b.h;
}

static void rd_u_slow_b( UINT8 *b )
{
	*b = RM( m6809.u.d );
	m6809.u.w.l++;
}

static void rd_u_slow_w( PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM( m6809.u.d );
	m6809.u.w.l++;
	p->b.l = RM( m6809.u.d );
	m6809.u.w.l++;
}

static void rd_u_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	*b = RAM[ m6809.u.d ];
	m6809.u.w.l++;
}

static void rd_u_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	CLEAR_PAIR(p);
    p->b.h = RAM[ m6809.u.d ];
	m6809.u.w.l++;
	p->b.l = RAM[ m6809.u.d ];
	m6809.u.w.l++;
}

static void wr_u_slow_b( UINT8 *b )
{
	--m6809.u.w.l;
	WM( m6809.u.d, *b );
}

static void wr_u_slow_w( PAIR *p )
{
	--m6809.u.w.l;
	WM( m6809.u.d, p->b.l );
	--m6809.u.w.l;
	WM( m6809.u.d, p->b.h );
}

static void wr_u_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	--m6809.u.w.l;
	RAM[ m6809.u.d ] = *b;
}

static void wr_u_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	--m6809.u.w.l;
	RAM[ m6809.u.d ] = p->b.l;
	--m6809.u.w.l;
	RAM[ m6809.u.d ] = p->b.h;
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
 * Get all registers in given buffer
 ****************************************************************************/
unsigned m6809_get_context(void *dst)
{
	if( dst )
		*(m6809_Regs*)dst = m6809;
	return sizeof(m6809_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void m6809_set_context(void *src)
{
	if( src )
		m6809 = *(m6809_Regs*)src;
    change_pc(PC);    /* TS 971002 */

    CHECK_IRQ_LINES;
}

/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned m6809_get_pc(void)
{
	return PC;
}


/****************************************************************************
 * Set program counter
 ****************************************************************************/
void m6809_set_pc(unsigned val)
{
	PC = val;
	change_pc(PC);
}


/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned m6809_get_sp(void)
{
	return S;
}


/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void m6809_set_sp(unsigned val)
{
	S = val;
}


/****************************************************************************/
/* Return a specific register                                               */
/****************************************************************************/
unsigned m6809_get_reg(int regnum)
{
	switch( regnum )
	{
		case M6809_A: return m6809.d.b.h;
		case M6809_B: return m6809.d.b.l;
		case M6809_PC: return m6809.pc.w.l;
		case M6809_S: return m6809.s.w.l;
		case M6809_U: return m6809.u.w.l;
		case M6809_X: return m6809.x.w.l;
		case M6809_Y: return m6809.y.w.l;
		case M6809_CC: return m6809.cc;
		case M6809_NMI_STATE: return m6809.nmi_state;
		case M6809_IRQ_STATE: return m6809.irq_state[M6809_IRQ_LINE];
		case M6809_FIRQ_STATE: return m6809.irq_state[M6809_FIRQ_LINE];
	}
	return 0;
}


/****************************************************************************/
/* Set a specific register                                                  */
/****************************************************************************/
void m6809_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M6809_A: m6809.d.b.h = val; break;
		case M6809_B: m6809.d.b.l = val; break;
		case M6809_PC: m6809.pc.w.l = val; break;
		case M6809_S: m6809.s.w.l = val; break;
		case M6809_U: m6809.u.w.l = val; break;
		case M6809_X: m6809.x.w.l = val; break;
		case M6809_Y: m6809.y.w.l = val; break;
		case M6809_CC: m6809.cc = val; break;
		case M6809_NMI_STATE: m6809.nmi_state = val; break;
		case M6809_IRQ_STATE: m6809.irq_state[M6809_IRQ_LINE] = val; break;
		case M6809_FIRQ_STATE: m6809.irq_state[M6809_FIRQ_LINE] = val; break;
    }
}


/****************************************************************************/
/* Reset registers to their initial values									*/
/****************************************************************************/
void m6809_reset(void *param)
{
	/* default to unoptimized memory access */
	rd_u_handler_b = rd_u_slow_b;
	rd_u_handler_w = rd_u_slow_w;
	rd_s_handler_b = rd_s_slow_b;
	rd_s_handler_w = rd_s_slow_w;
	wr_u_handler_b = wr_u_slow_b;
	wr_u_handler_w = wr_u_slow_w;
	wr_s_handler_b = wr_s_slow_b;
	wr_s_handler_w = wr_s_slow_w;

	DP = 0; 			/* Reset direct page register */

    CC |= CC_II;        /* IRQ disabled */
    CC |= CC_IF;        /* FIRQ disabled */

    RM16(0xfffe,&m6809.pc);
    change_pc(PC);    /* TS 971002 */

	/* optimize memory access according to flags */
	if( m6809_Flags & M6809_FAST_U )
	{
		rd_u_handler_b = rd_u_fast_b;
		rd_u_handler_w = rd_u_fast_w;
		wr_u_handler_b = wr_u_fast_b;
		wr_u_handler_w = wr_u_fast_w;
	}
	if( m6809_Flags & M6809_FAST_S )
	{
		rd_s_handler_b = rd_s_fast_b;
		rd_s_handler_w = rd_s_fast_w;
		wr_s_handler_b = wr_s_fast_b;
		wr_s_handler_w = wr_s_fast_w;
	}
}

void m6809_exit(void)
{
	/* nothing to do ? */
}

/* Generate interrupts */
/****************************************************************************
 * Set NMI line state
 ****************************************************************************/
void m6809_set_nmi_line(int state)
{
	if (m6809.nmi_state == state) return;
	m6809.nmi_state = state;
	LOG((errorlog, "M6809#%d set_nmi_line %d\n", cpu_getactivecpu(), state));
	if( state == CLEAR_LINE ) return;

	/* if the stack was not yet initialized */
    if( !(m6809.int_state & M6809_LDS) ) return;

    m6809.int_state &= ~M6809_SYNC;
	/* HJB 990225: state already saved by CWAI? */
	if( m6809.int_state & M6809_CWAI )
	{
		m6809.int_state &= ~M6809_CWAI;
		m6809.extra_cycles += 7;	/* subtract +7 cycles next time */
    }
	else
	{
		CC |= CC_E; 				/* save entire state */
		PUSHWORD(pPC);
		PUSHWORD(pU);
		PUSHWORD(pY);
		PUSHWORD(pX);
		PUSHBYTE(DP);
		PUSHBYTE(B);
		PUSHBYTE(A);
		PUSHBYTE(CC);
		m6809.extra_cycles += 19;	/* subtract +19 cycles next time */
	}
	CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */
	RM16(0xfffc,&m6809.pc);
	change_pc(PC);					/* TS 971002 */
}

/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void m6809_set_irq_line(int irqline, int state)
{
    LOG((errorlog, "M6809#%d set_irq_line %d, %d\n", cpu_getactivecpu(), irqline, state));
	m6809.irq_state[irqline] = state;
	if (state == CLEAR_LINE) return;
	CHECK_IRQ_LINES;
}

/****************************************************************************
 * Set IRQ vector callback
 ****************************************************************************/
void m6809_set_irq_callback(int (*callback)(int irqline))
{
	m6809.irq_callback = callback;
}

/****************************************************************************
 * Save CPU state
 ****************************************************************************/
static void state_save(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file, module, cpu, "PC", &m6809.pc.w.l, 1);
	state_save_UINT16(file, module, cpu, "U", &m6809.u.w.l, 1);
	state_save_UINT16(file, module, cpu, "S", &m6809.s.w.l, 1);
	state_save_UINT16(file, module, cpu, "X", &m6809.x.w.l, 1);
	state_save_UINT16(file, module, cpu, "Y", &m6809.y.w.l, 1);
	state_save_UINT8(file, module, cpu, "DP", &m6809.dp, 1);
	state_save_UINT8(file, module, cpu, "CC", &m6809.cc, 1);
	state_save_UINT8(file, module, cpu, "INT_STATE", &m6809.int_state, 1);
	state_save_UINT8(file, module, cpu, "NMI_STATE", &m6809.nmi_state, 1);
	state_save_UINT8(file, module, cpu, "IRQ_STATE", m6809.irq_state, 2);
}

/****************************************************************************
 * Load CPU state
 ****************************************************************************/
static void state_load(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file, module, cpu, "PC", &m6809.pc.w.l, 1);
	state_load_UINT16(file, module, cpu, "U", &m6809.u.w.l, 1);
	state_load_UINT16(file, module, cpu, "S", &m6809.s.w.l, 1);
	state_load_UINT16(file, module, cpu, "X", &m6809.x.w.l, 1);
	state_load_UINT16(file, module, cpu, "Y", &m6809.y.w.l, 1);
	state_load_UINT8(file, module, cpu, "DP", &m6809.dp, 1);
	state_load_UINT8(file, module, cpu, "CC", &m6809.cc, 1);
	state_load_UINT8(file, module, cpu, "INT_STATE", &m6809.int_state, 1);
	state_load_UINT8(file, module, cpu, "NMI_STATE", &m6809.nmi_state, 1);
	state_load_UINT8(file, module, cpu, "IRQ_STATE", m6809.irq_state, 2);
}

void m6809_state_save(void *file) { state_save(file, "m6809"); }
void m6809_state_load(void *file) { state_load(file, "m6809"); }

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m6809_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6809_Regs *r = context;

	which = ++which % 16;
    buffer[which][0] = '\0';
	if( !context )
		r = &m6809;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "M6809";
		case CPU_INFO_FAMILY: return "Motorola 6809";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) John Butler 1997";
		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->pc.w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%04X", r->s.w.l); break;
#if MAME_DEBUG
		case CPU_INFO_DASM: r->pc.w.l += Dasm6809(buffer[which], r->pc.w.l); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "$%02x", ROM[r->pc.w.l]); r->pc.w.l++; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->cc & 0x80 ? 'E':'.',
				r->cc & 0x40 ? 'F':'.',
                r->cc & 0x20 ? 'H':'.',
                r->cc & 0x10 ? 'I':'.',
                r->cc & 0x08 ? 'N':'.',
                r->cc & 0x04 ? 'Z':'.',
                r->cc & 0x02 ? 'V':'.',
                r->cc & 0x01 ? 'C':'.');
            break;
		case CPU_INFO_REG+M6809_A: sprintf(buffer[which], "A:%02X", r->d.b.h); break;
		case CPU_INFO_REG+M6809_B: sprintf(buffer[which], "B:%02X", r->d.b.l); break;
		case CPU_INFO_REG+M6809_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+M6809_S: sprintf(buffer[which], "S:%04X", r->s.w.l); break;
		case CPU_INFO_REG+M6809_U: sprintf(buffer[which], "U:%04X", r->u.w.l); break;
		case CPU_INFO_REG+M6809_X: sprintf(buffer[which], "X:%04X", r->x.w.l); break;
		case CPU_INFO_REG+M6809_Y: sprintf(buffer[which], "Y:%04X", r->y.w.l); break;
		case CPU_INFO_REG+M6809_CC: sprintf(buffer[which], "CC:%02X", r->cc); break;
		case CPU_INFO_REG+M6809_NMI_STATE: sprintf(buffer[which], "NMI:%d", r->nmi_state); break;
		case CPU_INFO_REG+M6809_IRQ_STATE: sprintf(buffer[which], "IRQ:%d", r->irq_state[M6809_IRQ_LINE]); break;
		case CPU_INFO_REG+M6809_FIRQ_STATE: sprintf(buffer[which], "FIRQ:%d", r->irq_state[M6809_FIRQ_LINE]); break;
	}
	return buffer[which];
}

/* includes the static function prototypes and the master opcode table */
#include "6809tbl.c"

/* includes the actual opcode implementations */
#include "6809ops.c"

/* execute instructions on this CPU until icount expires */
int m6809_execute(int cycles)	/* NS 970908 */
{
	UINT8 op_count;  /* op code clock count */
	UINT8 ireg;
	m6809_ICount = cycles;	/* NS 970908 */

	/* HJB 990226: subtract additional cycles for interrupts */
	m6809_ICount -= m6809.extra_cycles;
	m6809.extra_cycles = 0;

	if (m6809.int_state & (M6809_CWAI | M6809_SYNC))
	{
		m6809_ICount = 0;
		goto getout;
	}

	do
	{
		extern int previouspc;

		previouspc = PC;

#ifdef	MAME_DEBUG
		if (mame_debug) MAME_Debug();
#endif
		ireg=M_RDOP(PC++);

		if( (op_count = cycles1[ireg]) != 0xff ) {
			if( op_count &0x80 )
				op_count += fetch_effective_address();

			(*m6809_main[ireg])();
		}
		else
		{
			UINT8 ireg2;
			ireg2 = M_RDOP(PC++);

			if( (op_count=cycles2[ireg2]) & 0x80 )
				op_count += fetch_effective_address();

			if( ireg == 0x10 )
				(*m6809_pref10[ireg2])();
			else
				(*m6809_pref11[ireg2])();
		}
		m6809_ICount -= op_count & 0x7f;

	} while( m6809_ICount>0 );

getout:
	/* HJB 990226: subtract additional cycles for interrupts */
	m6809_ICount -= m6809.extra_cycles;
	m6809.extra_cycles = 0;

    return cycles - m6809_ICount;   /* NS 970908 */
}

INLINE UINT8 fetch_effective_address( void )
{
	UINT8 postbyte, ec=0;


	postbyte=M_RDOP_ARG(PC++);

	switch(postbyte)
	{
	case 0x00: EA=X;												ec=1;	break;
	case 0x01: EA=X+1;												ec=1;	break;
	case 0x02: EA=X+2;												ec=1;	break;
	case 0x03: EA=X+3;												ec=1;	break;
	case 0x04: EA=X+4;												ec=1;	break;
	case 0x05: EA=X+5;												ec=1;	break;
	case 0x06: EA=X+6;												ec=1;	break;
	case 0x07: EA=X+7;												ec=1;	break;
	case 0x08: EA=X+8;												ec=1;	break;
	case 0x09: EA=X+9;												ec=1;	break;
	case 0x0A: EA=X+10; 											ec=1;	break;
	case 0x0B: EA=X+11; 											ec=1;	break;
	case 0x0C: EA=X+12; 											ec=1;	break;
	case 0x0D: EA=X+13; 											ec=1;	break;
	case 0x0E: EA=X+14; 											ec=1;	break;
	case 0x0F: EA=X+15; 											ec=1;	break;

	case 0x10: EA=X-16; 											ec=1;	break;
	case 0x11: EA=X-15; 											ec=1;	break;
	case 0x12: EA=X-14; 											ec=1;	break;
	case 0x13: EA=X-13; 											ec=1;	break;
	case 0x14: EA=X-12; 											ec=1;	break;
	case 0x15: EA=X-11; 											ec=1;	break;
	case 0x16: EA=X-10; 											ec=1;	break;
	case 0x17: EA=X-9;												ec=1;	break;
	case 0x18: EA=X-8;												ec=1;	break;
	case 0x19: EA=X-7;												ec=1;	break;
	case 0x1A: EA=X-6;												ec=1;	break;
	case 0x1B: EA=X-5;												ec=1;	break;
	case 0x1C: EA=X-4;												ec=1;	break;
	case 0x1D: EA=X-3;												ec=1;	break;
	case 0x1E: EA=X-2;												ec=1;	break;
	case 0x1F: EA=X-1;												ec=1;	break;

	case 0x20: EA=Y;												ec=1;	break;
	case 0x21: EA=Y+1;												ec=1;	break;
	case 0x22: EA=Y+2;												ec=1;	break;
	case 0x23: EA=Y+3;												ec=1;	break;
	case 0x24: EA=Y+4;												ec=1;	break;
	case 0x25: EA=Y+5;												ec=1;	break;
	case 0x26: EA=Y+6;												ec=1;	break;
	case 0x27: EA=Y+7;												ec=1;	break;
	case 0x28: EA=Y+8;												ec=1;	break;
	case 0x29: EA=Y+9;												ec=1;	break;
	case 0x2A: EA=Y+10; 											ec=1;	break;
	case 0x2B: EA=Y+11; 											ec=1;	break;
	case 0x2C: EA=Y+12; 											ec=1;	break;
	case 0x2D: EA=Y+13; 											ec=1;	break;
	case 0x2E: EA=Y+14; 											ec=1;	break;
	case 0x2F: EA=Y+15; 											ec=1;	break;

	case 0x30: EA=Y-16; 											ec=1;	break;
	case 0x31: EA=Y-15; 											ec=1;	break;
	case 0x32: EA=Y-14; 											ec=1;	break;
	case 0x33: EA=Y-13; 											ec=1;	break;
	case 0x34: EA=Y-12; 											ec=1;	break;
	case 0x35: EA=Y-11; 											ec=1;	break;
	case 0x36: EA=Y-10; 											ec=1;	break;
	case 0x37: EA=Y-9;												ec=1;	break;
	case 0x38: EA=Y-8;												ec=1;	break;
	case 0x39: EA=Y-7;												ec=1;	break;
	case 0x3A: EA=Y-6;												ec=1;	break;
	case 0x3B: EA=Y-5;												ec=1;	break;
	case 0x3C: EA=Y-4;												ec=1;	break;
	case 0x3D: EA=Y-3;												ec=1;	break;
	case 0x3E: EA=Y-2;												ec=1;	break;
	case 0x3F: EA=Y-1;												ec=1;	break;

	case 0x40: EA=U;												ec=1;	break;
	case 0x41: EA=U+1;												ec=1;	break;
	case 0x42: EA=U+2;												ec=1;	break;
	case 0x43: EA=U+3;												ec=1;	break;
	case 0x44: EA=U+4;												ec=1;	break;
	case 0x45: EA=U+5;												ec=1;	break;
	case 0x46: EA=U+6;												ec=1;	break;
	case 0x47: EA=U+7;												ec=1;	break;
	case 0x48: EA=U+8;												ec=1;	break;
	case 0x49: EA=U+9;												ec=1;	break;
	case 0x4A: EA=U+10; 											ec=1;	break;
	case 0x4B: EA=U+11; 											ec=1;	break;
	case 0x4C: EA=U+12; 											ec=1;	break;
	case 0x4D: EA=U+13; 											ec=1;	break;
	case 0x4E: EA=U+14; 											ec=1;	break;
	case 0x4F: EA=U+15; 											ec=1;	break;

	case 0x50: EA=U-16; 											ec=1;	break;
	case 0x51: EA=U-15; 											ec=1;	break;
	case 0x52: EA=U-14; 											ec=1;	break;
	case 0x53: EA=U-13; 											ec=1;	break;
	case 0x54: EA=U-12; 											ec=1;	break;
	case 0x55: EA=U-11; 											ec=1;	break;
	case 0x56: EA=U-10; 											ec=1;	break;
	case 0x57: EA=U-9;												ec=1;	break;
	case 0x58: EA=U-8;												ec=1;	break;
	case 0x59: EA=U-7;												ec=1;	break;
	case 0x5A: EA=U-6;												ec=1;	break;
	case 0x5B: EA=U-5;												ec=1;	break;
	case 0x5C: EA=U-4;												ec=1;	break;
	case 0x5D: EA=U-3;												ec=1;	break;
	case 0x5E: EA=U-2;												ec=1;	break;
	case 0x5F: EA=U-1;												ec=1;	break;

	case 0x60: EA=S;												ec=1;	break;
	case 0x61: EA=S+1;												ec=1;	break;
	case 0x62: EA=S+2;												ec=1;	break;
	case 0x63: EA=S+3;												ec=1;	break;
	case 0x64: EA=S+4;												ec=1;	break;
	case 0x65: EA=S+5;												ec=1;	break;
	case 0x66: EA=S+6;												ec=1;	break;
	case 0x67: EA=S+7;												ec=1;	break;
	case 0x68: EA=S+8;												ec=1;	break;
	case 0x69: EA=S+9;												ec=1;	break;
	case 0x6A: EA=S+10; 											ec=1;	break;
	case 0x6B: EA=S+11; 											ec=1;	break;
	case 0x6C: EA=S+12; 											ec=1;	break;
	case 0x6D: EA=S+13; 											ec=1;	break;
	case 0x6E: EA=S+14; 											ec=1;	break;
	case 0x6F: EA=S+15; 											ec=1;	break;

	case 0x70: EA=S-16; 											ec=1;	break;
	case 0x71: EA=S-15; 											ec=1;	break;
	case 0x72: EA=S-14; 											ec=1;	break;
	case 0x73: EA=S-13; 											ec=1;	break;
	case 0x74: EA=S-12; 											ec=1;	break;
	case 0x75: EA=S-11; 											ec=1;	break;
	case 0x76: EA=S-10; 											ec=1;	break;
	case 0x77: EA=S-9;												ec=1;	break;
	case 0x78: EA=S-8;												ec=1;	break;
	case 0x79: EA=S-7;												ec=1;	break;
	case 0x7A: EA=S-6;												ec=1;	break;
	case 0x7B: EA=S-5;												ec=1;	break;
	case 0x7C: EA=S-4;												ec=1;	break;
	case 0x7D: EA=S-3;												ec=1;	break;
	case 0x7E: EA=S-2;												ec=1;	break;
	case 0x7F: EA=S-1;												ec=1;	break;

	case 0x80: EA=X;	X++;										ec=2;	break;
	case 0x81: EA=X;	X+=2;										ec=3;	break;
	case 0x82: X--; 	EA=X;										ec=2;	break;
	case 0x83: X-=2;	EA=X;										ec=3;	break;
	case 0x84: EA=X;														break;
	case 0x85: EA=X+SIGNED(B);										ec=1;	break;
	case 0x86: EA=X+SIGNED(A);										ec=1;	break;
	case 0x87: EA=0;														break; /*	ILLEGAL*/
	case 0x88: IMMBYTE(EA); 	EA=X+SIGNED(EA);					ec=1;	break; /* this is a hack to make Vectrex work. It should be ec=1. Dunno where the cycle was lost :( */
	case 0x89: IMMWORD(ea); 	EA+=X;								ec=4;	break;
	case 0x8A: EA=0;														break; /*	ILLEGAL*/
	case 0x8B: EA=X+D;												ec=4;	break;
	case 0x8C: IMMBYTE(EA); 	EA=PC+SIGNED(EA);					ec=1;	break;
	case 0x8D: IMMWORD(ea); 	EA+=PC; 							ec=5;	break;
	case 0x8E: EA=0;														break; /*	ILLEGAL*/
	case 0x8F: IMMWORD(ea); 										ec=5;	break;

	case 0x90: EA=X;	X++;						RM16(EAD,&ea);	ec=5;	break; /* Indirect ,R+ not in my specs */
	case 0x91: EA=X;	X+=2;						RM16(EAD,&ea);	ec=6;	break;
	case 0x92: X--; 	EA=X;						RM16(EAD,&ea);	ec=5;	break;
	case 0x93: X-=2;	EA=X;						RM16(EAD,&ea);	ec=6;	break;
	case 0x94: EA=X;								RM16(EAD,&ea);	ec=3;	break;
	case 0x95: EA=X+SIGNED(B);						RM16(EAD,&ea);	ec=4;	break;
	case 0x96: EA=X+SIGNED(A);						RM16(EAD,&ea);	ec=4;	break;
	case 0x97: EA=0;														break; /*	ILLEGAL*/
	case 0x98: IMMBYTE(EA); 	EA=X+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0x99: IMMWORD(ea); 	EA+=X;				RM16(EAD,&ea);	ec=7;	break;
	case 0x9A: EA=0;														break; /*	ILLEGAL*/
	case 0x9B: EA=X+D;								RM16(EAD,&ea);	ec=7;	break;
	case 0x9C: IMMBYTE(EA); 	EA=PC+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0x9D: IMMWORD(ea); 	EA+=PC; 			RM16(EAD,&ea);	ec=8;	break;
	case 0x9E: EA=0;														break; /*	ILLEGAL*/
	case 0x9F: IMMWORD(ea); 						RM16(EAD,&ea);	ec=8;	break;

	case 0xA0: EA=Y;	Y++;										ec=2;	break;
	case 0xA1: EA=Y;	Y+=2;										ec=3;	break;
	case 0xA2: Y--; 	EA=Y;										ec=2;	break;
	case 0xA3: Y-=2;	EA=Y;										ec=3;	break;
	case 0xA4: EA=Y;														break;
	case 0xA5: EA=Y+SIGNED(B);										ec=1;	break;
	case 0xA6: EA=Y+SIGNED(A);										ec=1;	break;
	case 0xA7: EA=0;														break; /*	ILLEGAL*/
	case 0xA8: IMMBYTE(EA); 	EA=Y+SIGNED(EA);					ec=1;	break;
	case 0xA9: IMMWORD(ea); 	EA+=Y;								ec=4;	break;
	case 0xAA: EA=0;														break; /*	ILLEGAL*/
	case 0xAB: EA=Y+D;												ec=4;	break;
	case 0xAC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);					ec=1;	break;
	case 0xAD: IMMWORD(ea); 	EA+=PC; 							ec=5;	break;
	case 0xAE: EA=0;														break; /*	ILLEGAL*/
	case 0xAF: IMMWORD(ea); 										ec=5;	break;

	case 0xB0: EA=Y;	Y++;						RM16(EAD,&ea);	ec=5;	break;
	case 0xB1: EA=Y;	Y+=2;						RM16(EAD,&ea);	ec=6;	break;
	case 0xB2: Y--; 	EA=Y;						RM16(EAD,&ea);	ec=5;	break;
	case 0xB3: Y-=2;	EA=Y;						RM16(EAD,&ea);	ec=6;	break;
	case 0xB4: EA=Y;								RM16(EAD,&ea);	ec=3;	break;
	case 0xB5: EA=Y+SIGNED(B);						RM16(EAD,&ea);	ec=4;	break;
	case 0xB6: EA=Y+SIGNED(A);						RM16(EAD,&ea);	ec=4;	break;
	case 0xB7: EA=0;														break; /*	ILLEGAL*/
	case 0xB8: IMMBYTE(EA); 	EA=Y+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xB9: IMMWORD(ea); 	EA+=Y;				RM16(EAD,&ea);	ec=7;	break;
	case 0xBA: EA=0;														break; /*	ILLEGAL*/
	case 0xBB: EA=Y+D;								RM16(EAD,&ea);	ec=7;	break;
	case 0xBC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xBD: IMMWORD(ea); 	EA+=PC; 			RM16(EAD,&ea);	ec=8;	break;
	case 0xBE: EA=0;														break; /*	ILLEGAL*/
	case 0xBF: IMMWORD(ea); 						RM16(EAD,&ea);	ec=8;	break;

	case 0xC0: EA=U;			U++;								ec=2;	break;
	case 0xC1: EA=U;			U+=2;								ec=3;	break;
	case 0xC2: U--; 			EA=U;								ec=2;	break;
	case 0xC3: U-=2;			EA=U;								ec=3;	break;
	case 0xC4: EA=U;														break;
	case 0xC5: EA=U+SIGNED(B);										ec=1;	break;
	case 0xC6: EA=U+SIGNED(A);										ec=1;	break;
	case 0xC7: EA=0;														break; /*ILLEGAL*/
	case 0xC8: IMMBYTE(EA); 	EA=U+SIGNED(EA);					ec=1;	break;
	case 0xC9: IMMWORD(ea); 	EA+=U;								ec=4;	break;
	case 0xCA: EA=0;														break; /*ILLEGAL*/
	case 0xCB: EA=U+D;												ec=4;	break;
	case 0xCC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);					ec=1;	break;
	case 0xCD: IMMWORD(ea); 	EA+=PC; 							ec=5;	break;
	case 0xCE: EA=0;														break; /*ILLEGAL*/
	case 0xCF: IMMWORD(ea); 										ec=5;	break;

	case 0xD0: EA=U;	U++;						RM16(EAD,&ea);	ec=5;	break;
	case 0xD1: EA=U;	U+=2;						RM16(EAD,&ea);	ec=6;	break;
	case 0xD2: U--; 	EA=U;						RM16(EAD,&ea);	ec=5;	break;
	case 0xD3: U-=2;	EA=U;						RM16(EAD,&ea);	ec=6;	break;
	case 0xD4: EA=U;								RM16(EAD,&ea);	ec=3;	break;
	case 0xD5: EA=U+SIGNED(B);						RM16(EAD,&ea);	ec=4;	break;
	case 0xD6: EA=U+SIGNED(A);						RM16(EAD,&ea);	ec=4;	break;
	case 0xD7: EA=0;														break; /*ILLEGAL*/
	case 0xD8: IMMBYTE(EA); 	EA=U+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xD9: IMMWORD(ea); 	EA+=U;				RM16(EAD,&ea);	ec=7;	break;
	case 0xDA: EA=0;														break; /*ILLEGAL*/
	case 0xDB: EA=U+D;								RM16(EAD,&ea);	ec=7;	break;
	case 0xDC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xDD: IMMWORD(ea); 	EA+=PC; 			RM16(EAD,&ea);	ec=8;	break;
	case 0xDE: EA=0;														break; /*ILLEGAL*/
	case 0xDF: IMMWORD(ea); 						RM16(EAD,&ea);	ec=8;	break;

	case 0xE0: EA=S;	S++;										ec=2;	break;
	case 0xE1: EA=S;	S+=2;										ec=3;	break;
	case 0xE2: S--; 	EA=S;										ec=2;	break;
	case 0xE3: S-=2;	EA=S;										ec=3;	break;
	case 0xE4: EA=S;														break;
	case 0xE5: EA=S+SIGNED(B);										ec=1;	break;
	case 0xE6: EA=S+SIGNED(A);										ec=1;	break;
	case 0xE7: EA=0;														break; /*ILLEGAL*/
	case 0xE8: IMMBYTE(EA); 	EA=S+SIGNED(EA);					ec=1;	break;
	case 0xE9: IMMWORD(ea); 	EA+=S;								ec=4;	break;
	case 0xEA: EA=0;														break; /*ILLEGAL*/
	case 0xEB: EA=S+D;												ec=4;	break;
	case 0xEC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);					ec=1;	break;
	case 0xED: IMMWORD(ea); 	EA+=PC; 							ec=5;	break;
	case 0xEE: EA=0;														break;	/*ILLEGAL*/
	case 0xEF: IMMWORD(ea); 										ec=5;	break;

	case 0xF0: EA=S;	S++;						RM16(EAD,&ea);	ec=5;	break;
	case 0xF1: EA=S;	S+=2;						RM16(EAD,&ea);	ec=6;	break;
	case 0xF2: S--; 	EA=S;						RM16(EAD,&ea);	ec=5;	break;
	case 0xF3: S-=2;	EA=S;						RM16(EAD,&ea);	ec=6;	break;
	case 0xF4: EA=S;								RM16(EAD,&ea);	ec=3;	break;
	case 0xF5: EA=S+SIGNED(B);						RM16(EAD,&ea);	ec=4;	break;
	case 0xF6: EA=S+SIGNED(A);						RM16(EAD,&ea);	ec=4;	break;
	case 0xF7: EA=0;														break; /*ILLEGAL*/
	case 0xF8: IMMBYTE(EA); 	EA=S+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xF9: IMMWORD(ea); 	EA+=S;				RM16(EAD,&ea);	ec=7;	break;
	case 0xFA: EA=0;														break; /*ILLEGAL*/
	case 0xFB: EA=S+D;								RM16(EAD,&ea);	ec=7;	break;
	case 0xFC: IMMBYTE(EA); 	EA=PC+SIGNED(EA);	RM16(EAD,&ea);	ec=4;	break;
	case 0xFD: IMMWORD(ea); 	EA+=PC; 			RM16(EAD,&ea);	ec=8;	break;
	case 0xFE: EA=0;														break; /*ILLEGAL*/
	case 0xFF: IMMWORD(ea); 						RM16(EAD,&ea);	ec=8;	break;
	}
	return (ec);
}

/****************************************************************************
 * M6309 section
 ****************************************************************************/
void m6309_reset(void *param) { m6809_reset(param); }
void m6309_exit(void) { m6809_exit(); }
int m6309_execute(int cycles) { return m6809_execute(cycles); }
unsigned m6309_get_context(void *dst) { return m6809_get_context(dst); }
void m6309_set_context(void *src) { m6809_set_context(src); }
unsigned m6309_get_pc(void) { return m6809_get_pc(); }
void m6309_set_pc(unsigned val) { m6809_set_pc(val); }
unsigned m6309_get_sp(void) { return m6809_get_sp(); }
void m6309_set_sp(unsigned val) { m6809_set_sp(val); }
unsigned m6309_get_reg(int regnum) { return m6809_get_reg(regnum); }
void m6309_set_reg(int regnum, unsigned val) { m6809_set_reg(regnum,val); }
void m6309_set_nmi_line(int state) { m6809_set_nmi_line(state); }
void m6309_set_irq_line(int irqline, int state) { m6809_set_irq_line(irqline,state); }
void m6309_set_irq_callback(int (*callback)(int irqline)) { m6809_set_irq_callback(callback); }
void m6309_state_save(void *file) { state_save(file, "m6309"); }
void m6309_state_load(void *file) { state_load(file, "m6309"); }
const char *m6309_info(void *context, int regnum)
{
    switch( regnum )
    {
        case CPU_INFO_NAME: return "M6309";
    }
    return m6809_info(context,regnum);
}
