/***************************************************************************

  Leland driver

    Driver provided by Paul Leaman (paul@vortexcomputing.demon.co.uk)

  2 x Z80 processors
  1 x AY8910 FM chip
  1 x AY8912 FM chip
  2 x 8 BIT DAC driven from video memory

  Some games are equipped with a "new generation" sound daughter-board
  which also contains
  1 x 10 BIT DAC
  6 x 8 BIT DAC
  1 x 80186 processor

  To get into service mode:
  offroad  = Service & nitro 3 button (blue)
  basebal2 = Service & 1P start
  strkzone = Service & 1P start
  dblplay  = Service & 1P start

***************************************************************************/

#include "driver.h"

#include "vidhrdw/generic.h"
#include "machine/8254pit.h"

//#define NOISY_CONTROLS
//#define NOISY_CPU
//#define NOISY_SOUND_CPU
//#define NOISY_SLAVE

/* Helps document the input ports. */
#define IPT_SLAVEHALT IPT_UNKNOWN

#define LELAND  "Leland Corp."
#define CINEMAT "Cinematronics"

#include "cpu/z80/z80.h"


/***********************************************************************

   Slave halt lines.

   Master interrogates the halt status of the slave CPU. This is mainly
   used in the self test.

   The slave CPU is reset and told to perform a memory test. The master
   then waits a number of vblanks (sufficient for the slave to
   do the memory test) and checks to see if the slave has halted. If the
   slave hasn't halted then it isn't responding.

************************************************************************/

static int leland_slave_halt=0;

void leland_slave_halt_w(int offset, int data)
{
	/* Only used for self-test */
/*
	char baf[40];
	sprintf(baf,"Z80 HALT=%d",data);
	usrintf_showmessage(baf);
*/
	if (data)
		leland_slave_halt=0;
	else
		leland_slave_halt=1;
}

int cpu_get_halt_line(int num)
{
#if 0
	return leland_slave_halt;
#else
	/* Kludge to return 0 */
	return 0;
#endif
}

void cpu_set_reset_line(int num, int reset)
{
	if (reset==HOLD_LINE )
	{
#ifdef NOISY_CPU
		/* Hold reset line (drop it low). Suspend CPU. */
		if (errorlog)
		{
			fprintf(errorlog, "CPU#%d (PC=%04x) RESET LINE HELD\n", num, cpu_get_pc());
		}
#endif
		if (cpu_getstatus(num))
		{
			cpu_reset(num);
			cpu_halt(num, 0);
		}
	}
	else
	{
		if (!cpu_getstatus(num))
		{
#ifdef NOISY_CPU
			/* Resume CPU when reset line has been raised */
			if (errorlog)
			{
				fprintf(errorlog, "CPU#%d RESET LINE CLEARED... RESUMING\n", num);
			}
#endif
			cpu_halt(num, 1);
		}
	}
}

void cpu_set_test_line(int num, int test)
{
	/* TEST flag for 80186 sync instrucntion */
	if (test==HOLD_LINE )
	{
		/* Set test line */
	}
	else
	{
		/* Reset test line */
	}
}

/* Video routines */
int leland_vh_start(void);
void leland_vh_stop(void);
void leland_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void pigout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* Video RAM ports */
void leland_master_video_addr_w(int offset, int data);
void leland_slave_video_addr_w(int offset, int data);
void leland_mvram_port_w(int offset, int data);
void leland_svram_port_w(int offset, int data);
int  leland_mvram_port_r(int offset);
int  leland_svram_port_r(int offset);

void leland_sound_port_w(int offset, int data);
int  leland_sound_port_r(int offset);

void leland_gfx_port_w(int offset, int data);

void leland_bk_xlow_w(int offset, int data);
void leland_bk_xhigh_w(int offset, int data);
void leland_bk_ylow_w(int offset, int data);
void leland_bk_yhigh_w(int offset, int data);

static int  leland_slave_cmd;

void (*leland_update_master_bank)(void);

void leland_onboard_dac(void);

int  leland_sh_start(const struct MachineSound *msound);
void leland_sh_stop(void);
void leland_sh_update(void);


/***********************************************************************

   ATAXX

************************************************************************/

extern int ataxx_vram_port_r(int offset, int num);
extern void ataxx_vram_port_w(int offset, int data, int num);
extern void ataxx_mvram_port_w(int offset, int data);
extern void ataxx_svram_port_w(int offset, int data);
extern int  ataxx_mvram_port_r(int offset);
extern int  ataxx_svram_port_r(int offset);

/***********************************************************************

   EEPROM

   EEPROM accessed via serial protocol.
   ATAXX may be different.

************************************************************************/

static int leland_eeprom[0x0100]; /* Unknown size */

/*
	0x80 = Analog control ready bit. Game sits in tight loop waiting
		   before reading the control.
	0x40 = Clock?
	0x06 = ?
*/


int leland_eeprom_r(int offset)
{
	static int s;
	s=s^0x01;
#if 0
	if (errorlog)
	{
		fprintf(errorlog, "PC=%04x EEPROM read\n", cpu_get_pc());
	}
#endif
	return s;
}

void leland_eeprom_w(int offset, int data)
{
	/*
	ATAXX - Routine at 0x5ab9 tests the EEPROM. Appears to be 16 bits.
	ATAXX - Routine at 0x7587 also does something with the EEPROM
	*/
#if 0
	if (errorlog)
	{
		fprintf(errorlog, "PC=%04x EEPROM write = %02x\n",
			cpu_get_pc(), data);
	}
#endif
}

/***********************************************************************

   BATTERY BACKED RAM

   Usually at 0xa000-0xdfff.

************************************************************************/

#define leland_battery_ram_size 0x4000
static unsigned char leland_battery_ram[leland_battery_ram_size];

void leland_battery_w(int offset, int data)
{
	leland_battery_ram[offset]=data;
}

static void leland_hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		/* Battery backed RAM */
		osd_fwrite (f, leland_battery_ram, leland_battery_ram_size);
		/* EEPROM */
		osd_fwrite_msbfirst (f, leland_eeprom, sizeof(leland_eeprom));
		osd_fclose (f);
	}
}

static int leland_hiload (void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		/* Battery backed RAM */
		osd_fread (f, leland_battery_ram, leland_battery_ram_size);
		/* EEPROM */
		osd_fread_msbfirst (f, leland_eeprom, sizeof(leland_eeprom));
		osd_fclose (f);
	}
	else
	{
		memset(leland_battery_ram, 0, leland_battery_ram_size);
		memset(leland_eeprom, 0, sizeof(leland_eeprom));
	}
	/*
	Clear down RAM.
	*/
	memset(&RAM[0xe000], 0, 0x1000);

	return 1;
}

#ifdef MAME_DEBUG
void leland_debug_dump_driver(void)
{
    if (keyboard_pressed(KEYCODE_M))
	{
		static int marker=1;
        while (keyboard_pressed(KEYCODE_M))       ;

		if (errorlog)
		{
			fprintf(errorlog, "Marker %d\n", marker);
			marker++;
		}
	}
    if (keyboard_pressed(KEYCODE_F))
	{
		FILE *fp=fopen("MASTER.DMP", "w+b");
		if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
			fwrite(RAM, 0x10000, 1, fp);
			fclose(fp);
		}
		fp=fopen("SLAVE.DMP", "w+b");
		if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
			fwrite(RAM, 0x10000, 1, fp);
			fclose(fp);
		}
		fp=fopen("SOUND.DMP", "w+b");
		if (fp)
		{
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
			int size=Machine->memory_region_length[Machine->drv->cpu[2].memory_region];
			if (size != 1)
			{
				fwrite(RAM, size, 1, fp);
			}
			fclose(fp);
		}

		fp=fopen("BATTERY.DMP", "w+b");
		if (fp)
		{
			fwrite(leland_battery_ram, leland_battery_ram_size, 1, fp);
			fclose(fp);
		}

	}
}
#else
#define leland_debug_dump_driver()
#endif

#define MACHINE_DRIVER(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                \
	{                                          \
		{                                   \
			CPU_Z80,        /* Master game processor */ \
			6000000,        /* 6.000 Mhz  */           \
			0,                                         \
			MR,MW,            \
			MRP,MWP,                                   \
			leland_master_interrupt,17                 \
		},                                                 \
		{                                                  \
			CPU_Z80, /* Slave graphics processor*/     \
			6000000, /* 6.000 Mhz */                   \
			2,       /* memory region #2 */            \
			SLR,SLW,              \
			slave_readport,slave_writeport,            \
			leland_slave_interrupt,1                   \
		},                                                 \
		{                                                  \
			CPU_I86,         /* Sound processor */     \
			16000000,        /* 16 Mhz  */             \
			3,                                         \
			leland_i86_readmem,leland_i86_writemem,\
			leland_i86_readport,leland_i86_writeport, \
			leland_i86_interrupt,1,                    \
			0,0,&leland_addrmask                    \
		},                                                 \
	},                                                         \
	60, 2000,2,                                                \
	INITMAC,                                       \
	0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
	GFXD,                                             \
	1024,1024,                                                 \
	0,                                                         \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
	0,                                                         \
	leland_vh_start,leland_vh_stop,VRF,    \
	0,0,0,0,\
	{   \
		{ SOUND_AY8910, &ay8910_interface }, \
		{ SOUND_DAC,    &dac_interface },     \
		{ SOUND_CUSTOM, &custom_interface }     \
	}                 \
}

/*
In this context, no sound means no 80186 sound processor.
*/

#define MACHINE_DRIVER_NO_SOUND(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                \
	{                                          \
		{                                   \
			CPU_Z80,        /* Master game processor */ \
			6000000,        /* 6.000 Mhz  */           \
			0,                                         \
			MR,MW,            \
			MRP,MWP,                                   \
			leland_master_interrupt,16                 \
		},                                                 \
		{                                                  \
			CPU_Z80, /* Slave graphics processor*/     \
			6000000, /* 6.000 Mhz */                   \
			2,       /* memory region #2 */            \
			SLR, SLW,               \
			slave_readport,slave_writeport,            \
			leland_slave_interrupt,1                   \
		},                                                 \
	},                                                         \
	60, 2000,2,                                                \
	INITMAC,                                       \
	0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
	GFXD,                                             \
	1024,1024,                                                 \
	0,                                                         \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
	0,                                                         \
	leland_vh_start,leland_vh_stop,VRF,    \
	0,0,0,0,\
	{   \
		{ SOUND_AY8910, &ay8910_interface }, \
		{ SOUND_CUSTOM, &custom_interface },    \
	}                 \
}


/*
   2 AY8910 chips - Actually, one of these is an 8912
   (8910 with only 1 output port)

   Port A of both chips is connected to a banking control
   register.
*/

static struct AY8910interface ay8910_interface =
{
	2,
	10000000/6, /* 1.666 MHz */
	{ 25, 25 },
	AY8910_DEFAULT_GAIN,
	{ leland_sound_port_r, leland_sound_port_r },
	{ 0 },
	{ leland_sound_port_w, leland_sound_port_w },
	{ 0 }
};

/*
There are:
 2x  8 bit DACs (connected to video board)
 6x  8 bit DACs (on I/O daughter board) Ataxx uses 3x8bit DACs
 1x 10 bit DAC  (on I/O daughter board)
*/

static struct DACinterface dac_interface =
{
	MAX_DAC,  /* 8 chips */
	{255,255,255,255,},    /* Volume */
};

static struct CustomSound_interface custom_interface =
{
	leland_sh_start,
	leland_sh_stop,
	leland_sh_update
};


/***********************************************************************

   SLAVE (16MHz 80186 MUSIC CPU)

   6 x  8 bit DAC
   1 x 10 bit DAC

************************************************************************/

static int leland_sound_cmd_low;
static int leland_sound_cmd_high;
static int leland_sound_response;

void leland_sound_init(void)
{
	leland_sound_cmd_low=0x55;
	leland_sound_cmd_high=0x55;
	leland_sound_response=0x55;
}

void leland_sound_cmd_low_w(int offset, int data)
{
	/* Z80 sound command low byte write */
	leland_sound_cmd_low=data;
}

void leland_sound_cmd_high_w(int offset, int data)
{
	/* Z80 sound command high byte write */
	leland_sound_cmd_high=data;
}

int leland_sound_cmd_r(int offset)
{
	/* 80186 sound command word read */
	if (!offset)
	{
		return leland_sound_cmd_low;
	}
	else
	{
		return leland_sound_cmd_high;
	}
}


int leland_sound_response_r(int offset)
{
	/* Z80 sound response byte read */
	return leland_sound_response;
}

void leland_sound_response_w(int offset, int data)
{
	/* 80186 sound response byte write */
	if (!offset)
	{
		leland_sound_response=data;
	}
}

void leland_sound_cpu_control_w(int data)
{
	int intnum;
	/*
	Sound control = data & 0xf7
	===========================
		0x80 = CPU Reset
		0x40 = NMI
		0x20 = INT0
		0x10 = TEST
		0x08 = INT1
	*/

	cpu_set_reset_line(2, data&0x80  ? CLEAR_LINE : HOLD_LINE);
    cpu_set_nmi_line(2,   data&0x40  ? CLEAR_LINE : HOLD_LINE);
    cpu_set_test_line(2,  data&0x20  ? CLEAR_LINE : HOLD_LINE);

	/* No idea about the int 0 and int 1 pins (do they give IRQ number?) */
	intnum =(data&0x20)>>6;  /* Int 0 */
	intnum|=(data&0x08)>>2;  /* Int 1 */

#ifdef NOISY_SOUND_CPU
	if (errorlog)
	{
		 fprintf(errorlog, "PC=%04x Sound CPU intnum=%02x\n",
			cpu_get_pc(), intnum);
	}
#endif
}

/***********************************************************************

   DAC

	There are 6x8 Bit DACs. 2 bytes for each
	(high=volume, low=value).

    The DACs are driven by 8254 pit chips

************************************************************************/

#define leland_max_dac 6
static int leland_dac_value[leland_max_dac];
static int leland_dac_volume[leland_max_dac];

void leland_dac_w(int offset, int data)
{
	int dacnum=(offset/2)+1;
	if (offset & 0x01)
	{
        leland_dac_volume[dacnum]=data;
	}
	else
	{
        leland_dac_value[dacnum]=data;
	}
}

void leland_pit1_callback(int which)
{
//    int dacnum=which;
//    DAC_set_volume(dacnum, leland_dac_volume[dacnum], 255);
    //DAC_data_w(dacnum, leland_dac_value[dacnum]);
}

void leland_pit2_callback(int which)
{
    int dacnum=which+3;
    if (dacnum >= MAX_DAC)
    {
        return;
    }
    //DAC_set_volume(dacnum, leland_dac_volume[dacnum], 255);
    //DAC_data_w(dacnum, leland_dac_value[dacnum]);
}

static pit8254_interface leland_pit8254_interface=
{
    2,          /* 2 PITs */
    {4000000, 4000000},     /* Clock 1 = 4 MHZ */
    {4000000, 4000000},     /* Clock 2 = 4 MHZ */
    {4000000, 4000000},     /* Clock 3 = 4 MHZ */
    {leland_pit1_callback, leland_pit2_callback},
};


void leland_pit8254_0_w(int offset, int data)
{
	if (!(offset & 0x01))
	{
		pit8254_w (0, offset/2, data);
	}
}

void leland_pit8254_1_w(int offset, int data)
{
	if (!(offset & 0x01))
	{
		pit8254_w (1, offset/2, data);
	}
}


int leland_i86_interrupt(void)
{
	return ignore_interrupt();
}

static struct IOReadPort leland_i86_readport[] =
{
    { -1 }  /* end of table */
};

static struct IOWritePort leland_i86_writeport[] =
{
	{ 0x0000, 0x000c, leland_dac_w },       /* 6x8 bit DACs */
	{ -1 }  /* end of table */
};

int leland_i86_ram_r(int offset)
{
	/*
	Not very tidy, but it works for now...
	*/
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	return RAM[0x1c000+offset];
}

void leland_i86_ram_w(int offset, int data)
{
	/*
	Not very tidy, but it works for now...
	*/
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	RAM[0x1c000+offset]=data;
}


static int leland_i86_unknown1_r(int offset)
{
	/*
	This must have 0x10 set, within a certain time period,
	otherwise the NMI routine reboots the processor
	*/
	return 0x10;
}


static struct MemoryReadAddress leland_i86_readmem[] =
{
	{ 0x020000, 0x020001, MRA_NOP }, /* Interrupt status register ??? */
	{ 0x020080, 0x020081, leland_sound_cmd_r },	/* Sound command word */

	{ 0x020324, 0x020325, MRA_NOP },
	{ 0x02032e, 0x02032f, leland_i86_unknown1_r },
	{ 0x020328, 0x020329, MRA_NOP },    /* AND 0xfffb, AND 0xfff7, OR 0x0004, OR 0x0008*/
	{ 0x02032c, 0x02032d, MRA_NOP },    /* AND 0xfffe, AND 0xfffb, AND 0xfff7 */
	{ 0x020330, 0x020331, MRA_NOP },    /* AND 0xfffd, AND 0xfffe, AND 0xfffb */

	{ 0x000000, 0x003fff, leland_i86_ram_r },   /* vectors live here */
	{ 0x01c000, 0x01ffff, MRA_RAM },
	//{ 0x020000, 0x0203ff, MRA_RAM },    /* Catch all io ports */
	{ 0x040000, 0x0fffff, MRA_ROM },    /* Program ROM */
	{ -1 }  /* end of table */
};



static void leland_i86_unknown2_w(int offset, int data)
{

}


static struct MemoryWriteAddress leland_i86_writemem[] =
{
	{ 0x020080, 0x020081, leland_sound_response_w },	/* Sound response byte */
	{ 0x020100, 0x020107, leland_pit8254_0_w },	/* PIT 1 */
	{ 0x020180, 0x020187, leland_pit8254_1_w },	/* PIT 2 */

	{ 0x020322, 0x020323, MWA_NOP },    /* Value always 0x8000 (int clear?) */
	{ 0x020328, 0x020329, MWA_NOP },    /* AND 0xfffb, AND 0xfff7, OR 0x0004, OR 0x0008*/
	{ 0x02032c, 0x02032d, MWA_NOP },    /* AND 0xfffe, AND 0xfffb, AND 0xfff7 */
	{ 0x020330, 0x020331, MWA_NOP },    /* AND 0xfffd, AND 0xfffe, AND 0xfffb */
	{ 0x020338, 0x020339, MWA_NOP },	/* Values 0x0000, 0x0006 */
	{ 0x020350, 0x020351, MWA_NOP },    /* Value  0x0000 */
	{ 0x020356, 0x020357, MWA_NOP },	/* Values 0x4001, 0xc001 */
	{ 0x020358, 0x020359, MWA_NOP },	/* Value  0x0000 */
	{ 0x02035a, 0x02035b, MWA_NOP },	/* Value  0xea60 */

	{ 0x02035e, 0x02035f, MWA_NOP },	/* Values 0x4001, 0xe001 */
	{ 0x020362, 0x020363, MWA_NOP },	/* Value  ????? */
	{ 0x020366, 0x020367, MWA_NOP },	/* Values 0x4001, 0xe001, 0x4000 */

	/*
		0x43ca=0x1684
		0x43c8=????
		0x43c0=????
		0x43c2=????
		0x43c8=0x1786
	*/
	{ 0x0203c0, 0x0203c3, MWA_NOP },	/* ??? */
	{ 0x0203c4, 0x0203c5, MWA_NOP },	/* ??? */
	{ 0x0203c8, 0x0203c9, MWA_NOP },	/* ??? */
	{ 0x0203ca, 0x0203cb, MWA_NOP },	/* Values 0x1684, 0x1786 */
	/*
		0x43da=0x1684
		0x43d8=????
		0x43d0=????
		0x43d2=????
		0x43d8=0x1786
	*/
	{ 0x0203d0, 0x0203d3, MWA_NOP },	/* ??? */
	{ 0x0203d4, 0x0203d5, MWA_NOP },	/* ??? */
	{ 0x0203d8, 0x0203d9, MWA_NOP },	/* ??? */
	{ 0x0203da, 0x0203db, MWA_NOP },	/* Values 0x1684, 0x1786 */


	{ 0x000000, 0x003fff, leland_i86_ram_w },   /* vectors live here */
	{ 0x01c000, 0x01ffff, MWA_RAM }, /* 64K RAM (also appears at 0x00000) */
	{ 0x020000, 0x0203ff, MWA_RAM }, /* Catch all IO ports */
	{ 0x040000, 0x0fffff, MWA_ROM }, /* Program ROM */
	{ -1 }  /* end of table */
};

/***********************************************************************

   MASTER (Z80 MAIN CPU)

   There appear to be 16 interrupts per frame. The interrupts
   are generated by the graphics hardware (probably every 16 scan
   lines).

   There is a video refresh flag in one of the input ports (0x02 of
   port 0x51 in Pigout). When this is set (clear?), the game resets
   the interrupt counter and performs some extra processing.

   The interrupt routine increments a counter every time an interrupt
   is triggered. The counter wraps around at 16. When it reaches 0,
   it does the same as if the refresh flag was set. I don't think that
   it ever reaches that far (it doesn't make sense to do all that expensive
   processing twice in quick succession).

************************************************************************/

int leland_raster;

int leland_master_interrupt(void)
{
	leland_raster++;
	leland_raster&=0x0f;

	/* Debug dumping */
	leland_debug_dump_driver();

	/* Generate an interrupt */
	return interrupt();
}

void leland_slave_cmd_w(int offset, int data)
{
#if 0
	if (errorlog)
	{
		fprintf(errorlog, "SLAVECMD=%02x RES=%d NMI=%d IRQ=%d\n", data,
			data&0x01,
			data&0x04,
			data&0x08);
	}
#endif

	cpu_set_reset_line(1, data&0x01  ? CLEAR_LINE : HOLD_LINE);
	/* 0x02=colour write */
	cpu_set_nmi_line(1,    data&0x04 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_irq_line(1, 0, data&0x08 ? CLEAR_LINE : HOLD_LINE);

	/*
	0x10, 0x20, 0x40 = Unknown (connected to bit 0 of control read
	halt detect bit for slave CPU)
	*/
	leland_slave_cmd=data;
}

/***********************************************************************

 There is a bit in one of the input port registers that indicates
 the halt status of the slave Z80.

************************************************************************/

INLINE int leland_slavebit_r(int bit)
{
	int ret=input_port_0_r(0);
	int halt=0;
	if (cpu_get_halt_line(1))
	{
		halt=bit;  /* CPU halted */
	}
	ret=(ret&(~bit))|halt;
	return ret;
}

int leland_slavebit0_r(int offset)
{
	return leland_slavebit_r(0x01);
}

int leland_slavebit1_r(int offset)
{
	return leland_slavebit_r(0x02);
}

/***********************************************************************

 Analog ports.

************************************************************************/

static int leland_current_analog;

void leland_analog_w(int offset, int data)
{
	/* Set the current analog port number */
	leland_current_analog=data&0x0f;
#ifdef NOISY_CONTROLS
	if (errorlog)
	{
		fprintf(errorlog, "Analog joystick %02x (device#%d)\n", data, leland_current_analog);
	}
#endif
}

int leland_analog_r(int offset)
{
#ifdef NOISY_CONTROLS
	if (errorlog)
	{
		fprintf(errorlog, "Analog joystick (device#%d)\n", leland_current_analog);
	}
#endif
	return readinputport(leland_current_analog+4+offset);
}

/***********************************************************************

   SLAVE (Z80 GFX CPU)

   A big graphics blitter.

************************************************************************/

int leland_slave_interrupt(void)
{
	/* Slave's interrupts are driven by the master CPU */
	return ignore_interrupt();
}

int leland_slave_cmd_r(int offset)
{
	return leland_slave_cmd;
}

void leland_slave_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;
    bankaddress=0x10000+0xc000*(data&0x0f);
	cpu_setbank(3, &RAM[bankaddress]);

#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU #1 %04x BANK SWITCH %02x\n",
			cpu_get_pc(), data);
	}
#endif
}

void leland_slave_large_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress=0x10000+0x8000*(data&0x0f);
	cpu_setbank(3, &RAM[bankaddress]);

#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "CPU #1 %04x BIG BANK SWITCH %02x\n",
			cpu_get_pc(), data);
	}
#endif
}

void leland_rearrange_bank_swap(int cpu, int startaddr)
{
	/*
	This function is for banks in the following format:
		ROM       Z80 address range
		0x0000    0x4000-0x7fff
		0x8000    0x2000-0x3fff
		...
	This is easy for the hardware to decode, but not so easy for MAME.
	Here, the banks are rearranged so that they are contiguous.
	*/

	int i;
	int region=Machine->drv->cpu[cpu].memory_region;
	unsigned char *RAM = Machine->memory_region[region];
	/* Calculate the number of banks in the remaining memory */
	int banks=(Machine->memory_region_length[region]-startaddr)/0x8000;
	unsigned char *p=malloc(0x8000);
	if (p)
	{
		if (errorlog)
		{
			fprintf(errorlog, "Region %d Swapping %d banks (start=0x%04x)\n",
				region, banks, startaddr);
		}

		for (i=0; i<banks;i++)
		{
			memcpy(p, &RAM[startaddr+0x2000], 0x6000);
			memcpy(p+0x6000, &RAM[startaddr], 0x2000);
			memcpy(&RAM[startaddr], p, 0x8000);
			startaddr+=0x8000;
		}
		free(p);
	}
	else
	{
		/* Should really abort */
	}
}

void leland_rearrange_bank(int cpu, int start)
{
	/*
	This function is for banks in the following format:
		ROM       Z80 address range
		0x0000    0x2000-0x2fff
		0x2000    0x2000-0x2fff
		0x0000    0x4000-0x5fff
		...
	Each ROM contains two banks for the same address.
	Rearrange the banks so that they are contiguous
	*/

	int i;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[cpu].memory_region];
	unsigned char *p=&RAM[start];
	char *buffer=malloc(0x18000);
	if (buffer)
	{
		memcpy(buffer, p, 0x18000);
		for (i=0; i<6; i++)
		{
			memcpy(p,         buffer+0x4000*i,         0x2000);
			memcpy(p+0x0c000, buffer+0x4000*i+0x02000, 0x2000);
			p+=0x2000;
		}
		free(buffer);
	}
	else
	{
		/* should really abort */
	}
}

int leland_unknown_slave_video_r(int offset)
{
#if 0
	if (errorlog)
	{
		fprintf(errorlog, "Unknown slave port PC=%04x\n", cpu_get_pc());
	}
#endif
	/* I don't know what to do with this! */
	return (leland_raster << 4)+15;
}

static struct MemoryReadAddress slave_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },        /* Resident program ROM */
	{ 0x2000, 0xdfff, MRA_BANK3 },      /* Paged graphics ROM */
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf802, 0xf802, leland_unknown_slave_video_r },
	{ 0xf000, 0xffff, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress slave_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf800, 0xf801, leland_slave_video_addr_w },
	{ 0xf803, 0xf803, leland_slave_banksw_w },
	{ 0xfffc, 0xfffd, leland_slave_video_addr_w }, /* Ataxx */
	/*
	Alley Master's slave routine clears this on startup
	It might be a bug in the game.
	*/
	{ 0xf000, 0xffff, MWA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress slave_readmem2[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },        /* Resident program ROM */
	{ 0x4000, 0xbfff, MRA_BANK3 },      /* Paged graphics ROM */
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf802, 0xf802, leland_unknown_slave_video_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress slave_writemem2[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, leland_slave_large_banksw_w },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf800, 0xf801, leland_slave_video_addr_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort slave_readport[] =
{
	{ 0x00, 0x1f, leland_svram_port_r }, /* Video ports (some games) */
	{ 0x40, 0x5f, leland_svram_port_r }, /* Video ports (other games) */
	{ 0x60, 0x7b, ataxx_svram_port_r },  /* Ataxx ports */
	{ -1 }  /* end of table */
};

static struct IOWritePort slave_writeport[] =
{
	{ 0x00, 0x1f, leland_svram_port_w }, /* Video ports (some games) */
	{ 0x40, 0x5f, leland_svram_port_w }, /* Video ports (other games) */
	{ 0x60, 0x7b, ataxx_svram_port_w },  /* Ataxx ports */
	{ Z80_HALT_PORT, Z80_HALT_PORT, leland_slave_halt_w },
	{ -1 }  /* end of table */
};

static struct GfxLayout bklayout =
{
	8,8,    /* 8*8 characters */
	4096, /* ??? characters */
	3,      /* 3 bits per pixel */
	{ 0*0x08000*8,1*0x08000*8,2*0x08000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8    /* every char takes 8*8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/*   start    pointer       colour start   number of colours */
	{ 1, 0x000000, &bklayout,          0, 32 },    /* 32 */
	{ -1 } /* end of array */
};


/* 80186 1 MB address mask */
int leland_addrmask=0x0fffff;

void leland_init_machine(void)
{
	   leland_update_master_bank=NULL;     /* No custom master banking */
	   leland_slave_halt=0;
       pit8254_init (&leland_pit8254_interface);
	   leland_sound_init();
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

/***************************************************************************

  Strike Zone

***************************************************************************/

INPUT_PORTS_START( input_ports_strkzone )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x40 */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Extra Base", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Go Back", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "R Run/Steal", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1, "L Run/Steal", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1, "Run/Aim", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1, "Run/Cutoff", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* Analog joystick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog joystick 2 */
	PORT_START
	PORT_START      /* Analog joystick 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 10, 0, 0, 255 )
INPUT_PORTS_END

static int leland_bank;

void strkzone_update_bank(void)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	if (leland_bank & 0x80 )
	{
		if (leland_bank & 0x40)
		{
			bankaddress=0x30000;
		}
		else
		{
			bankaddress=0x28000;
		}

		if ( leland_sound_port_r(0) & 0x20 )
		{
			/* ROM */
			cpu_setbank(2, &RAM[bankaddress+(0xa000-0x2000)]);
		}
		else
		{
			/* Battery RAM */
			cpu_setbank(2, leland_battery_ram);
		}
	}
	else
	{
		if (leland_sound_port_r(0) & 0x04)
		{
			bankaddress=0x1c000;
		}
		else
		{
			bankaddress=0x10000;
		}

		cpu_setbank(2, &RAM[bankaddress+(0xa000-0x2000)]);
	}
	cpu_setbank(1, &RAM[bankaddress]);
}

void strkzone_banksw_w(int offset, int data)
{
	leland_bank=data;
	strkzone_update_bank();
}

static struct MemoryReadAddress strkzone_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },        /* ROM */
	{ 0x2000, 0x9fff, MRA_BANK1 },      /* Paged ROM */
	{ 0xa000, 0xdfff, MRA_BANK2 },      /* Battery RAM / ROM */
	{ 0xf000, 0xf3ff, paletteram_r },   /* Palette RAM */
	{ 0xe000, 0xefff, MRA_RAM },        /* RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress strkzone_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM   },
	{ 0x2000, 0x9fff, MWA_BANK1 },
	{ 0xa000, 0xdfff, leland_battery_w }, /* Battery RAM */
	{ 0xf000, 0xf3ff, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xf800, 0xf801, leland_master_video_addr_w },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort strkzone_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort strkzone_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4f, leland_gfx_port_w },   /* Video ports */
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xff, 0xff, leland_eeprom_w },
	{ -1 }  /* end of table */
};

#define STRKZONE_CODE_SIZE 0x3c000

void strkzone_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	leland_update_master_bank=strkzone_update_bank;
	leland_rearrange_bank(0, 0x10000);  /* Master bank */

	/* Chip 4 is banked in */
	memcpy(&RAM[0x30000], &RAM[0x28000], 0x6000);
	memcpy(&RAM[0x34000], &RAM[0x2e000], 0x2000);
	leland_rearrange_bank(1, 0x10000);  /* Slave bank */
}

MACHINE_DRIVER_NO_SOUND(strkzone_machine, strkzone_readport, strkzone_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( strkzone_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)   /* 64k for code + banked ROMs images */
	ROM_LOAD("strkzone.101",   0x00000, 0x04000, 0x8d83a611 ) /* 0x0000 */

	ROM_LOAD("strkzone.102",   0x10000, 0x04000, 0x3859e67d ) /* 0x2000 (2 pages) */
	ROM_LOAD("strkzone.103",   0x14000, 0x04000, 0xcdd83bfb ) /* 0x4000 (2 pages)*/
	ROM_LOAD("strkzone.104",   0x18000, 0x04000, 0xbe280212 ) /* 0x6000 (2 pages)*/
	ROM_LOAD("strkzone.105",   0x1c000, 0x04000, 0xafb63390 ) /* 0x8000 (2 pages)*/
	ROM_LOAD("strkzone.106",   0x20000, 0x04000, 0xe853b9f6 ) /* 0xA000 (2 pages)*/
	ROM_LOAD("strkzone.107",   0x24000, 0x04000, 0x1b4b6c2d ) /* 0xC000 (2 pages)*/
	/* Extra banks (referred to as the "top" board). Probably an add-on */
    ROM_LOAD("strkzone.u2t",   0x28000, 0x02000, 0x8e0af06f ) /* 2000-3fff (1 page) */
    ROM_LOAD("strkzone.u3t",   0x2a000, 0x02000, 0x909d35f3 ) /* 4000-5fff (1 page) */
    ROM_LOAD("strkzone.u4t",   0x2c000, 0x04000, 0x9b1e72e9 ) /* 6000-7fff (2 pages) */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("strkzone.u93", 0x00000, 0x04000, 0x8ccb1404 )
    ROM_LOAD("strkzone.u94", 0x08000, 0x04000, 0x9941a55b )
    ROM_LOAD("strkzone.u95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
    ROM_LOAD("strkzone.u3",  0x00000, 0x02000, 0x40258fbe ) /* 0000-1fff */
    ROM_LOAD("strkzone.u4",  0x10000, 0x04000, 0xdf7f2604 ) /* 2000-3fff */
    ROM_LOAD("strkzone.u5",  0x14000, 0x04000, 0x37885206 ) /* 4000-3fff */
    ROM_LOAD("strkzone.u6",  0x18000, 0x04000, 0x6892dc4f ) /* 6000-3fff */
    ROM_LOAD("strkzone.u7",  0x1c000, 0x04000, 0x6ac8f87c ) /* 8000-3fff */
    ROM_LOAD("strkzone.u8",  0x20000, 0x04000, 0x4b6d3725 ) /* a000-3fff */
    ROM_LOAD("strkzone.u9",  0x24000, 0x04000, 0xab3aac49 ) /* c000-3fff */

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
    ROM_LOAD( "strkzone.u92",  0x04000, 0x4000, 0x2508a9ad )
    ROM_LOAD( "strkzone.u69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
    ROM_LOAD( "strkzone.u68",  0x10000, 0x4000, 0xa1a51383 )
    ROM_LOAD( "strkzone.u90",  0x14000, 0x4000, 0xef01d997 )
    ROM_LOAD( "strkzone.u67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver strkzone_driver =
{
	__FILE__,
	0,
	"strkzone",
	"Strike Zone",
	"1988",
	LELAND,
	"Paul Leaman",
	0,
	&strkzone_machine,
	0,

	strkzone_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};



/***************************************************************************

  DblPlay

  Input ports the same as strike zone, apart from the position of the
  video RAM ports.

***************************************************************************/

static struct IOReadPort dblplay_readport[] =
{
	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },
	{ 0x80, 0x9f, leland_mvram_port_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort dblplay_writeport[] =
{
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4f, leland_gfx_port_w },   /* Video ports */
	{ 0x80, 0x9f, leland_mvram_port_w }, /* Video RAM ports (double play) */
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xff, 0xff, leland_eeprom_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(dblplay_machine, dblplay_readport, dblplay_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( dblplay_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15018-01.101",   0x00000, 0x02000, 0x17b6af29 )
	ROM_LOAD("15019-01.102",   0x10000, 0x04000, 0x9fc8205e ) /* 0x2000 */
	ROM_LOAD("15020-01.103",   0x14000, 0x04000, 0x4edcc091 ) /* 0x4000 */
	ROM_LOAD("15021-01.104",   0x18000, 0x04000, 0xa0eba1c7 ) /* 0x6000 */
	ROM_LOAD("15022-01.105",   0x1c000, 0x04000, 0x7bbfe0b7 ) /* 0x8000 */
	ROM_LOAD("15023-01.106",   0x20000, 0x04000, 0xbbedae34 ) /* 0xA000 */
	ROM_LOAD("15024-01.107",   0x24000, 0x04000, 0x02afcf52 ) /* 0xC000 */
	/* Extra banks (referred to as the "top" board). Probably an add-on */
    ROM_LOAD("15025-01.u2t",   0x28000, 0x02000, 0x1c959895 ) /* 2000-3fff (1 page) */
    ROM_LOAD("15026-01.u3t",   0x2a000, 0x02000, 0xed5196d6 ) /* 4000-5fff (1 page) */
    ROM_LOAD("15027-01.u4t",   0x2c000, 0x04000, 0x9b1e72e9 ) /* 6000-7fff (2 pages) */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("15015-01.u93", 0x00000, 0x04000, 0x8ccb1404 )
    ROM_LOAD("15016-01.u94", 0x08000, 0x04000, 0x9941a55b )
    ROM_LOAD("15017-01.u95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x26000)     /* Z80 slave CPU */
    ROM_LOAD("15000-01.u03",  0x00000, 0x02000, 0x208a920a )
    ROM_LOAD("15001-01.u04",  0x10000, 0x04000, 0x751c40d6 )
    ROM_LOAD("14402-01.u05",  0x14000, 0x04000, 0x5ffaec36 )
    ROM_LOAD("14403-01.u06",  0x18000, 0x04000, 0x48d6d9d3 )
    ROM_LOAD("15004-01.u07",  0x1c000, 0x04000, 0x6a7acebc )
    ROM_LOAD("15005-01.u08",  0x20000, 0x04000, 0x69d487c9 )
    ROM_LOAD("15006-01.u09",  0x22000, 0x04000, 0xab3aac49 )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
    ROM_LOAD( "15014-01.u92",  0x04000, 0x4000, 0x2508a9ad )
    ROM_LOAD( "15009-01.u69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
    ROM_LOAD( "15008-01.u68",  0x10000, 0x4000, 0xa1a51383 )
	/* U90 = Empty */
    ROM_LOAD( "15007-01.u67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver dblplay_driver =
{
	__FILE__,
	0,
	"dblplay",
	"Super Baseball Double Play Home Run Derby",
	"1987",
	LELAND "/Tradewest",
	"Paul Leaman",
	GAME_NOT_WORKING,
	&dblplay_machine,
	0,

	dblplay_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

 World Series

 Same IO ports as strkzone (Aim and cutoff buttons switched)
 Some buttons not used

***************************************************************************/

INPUT_PORTS_START( input_ports_wseries )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x40 */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Extra Base", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Go Back", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1, "Aim", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* Analog joystick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog joystick 2 */
	PORT_START
	PORT_START      /* Analog joystick 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 10, 0, 0, 255 )
INPUT_PORTS_END


static struct IOReadPort wseries_readport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, leland_slavebit0_r },
	{ 0x83, 0x83, AY8910_read_port_0_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0x91, 0x91, input_port_2_r },

	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort wseries_writeport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x89, 0x89, leland_slave_cmd_w },
	{ 0x8a, 0x8f, leland_gfx_port_w },   /* Video ports */
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xff, 0xff, leland_eeprom_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(wseries_machine, wseries_readport, wseries_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( wseries_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13409-01.101",   0x00000, 0x02000, 0xb5eccf5c )
	ROM_LOAD("13410-01.102",   0x10000, 0x04000, 0xdd1ec091 ) /* 0x2000 */
	ROM_LOAD("13411-01.103",   0x14000, 0x04000, 0xec867a0e ) /* 0x4000 */
	ROM_LOAD("13412-01.104",   0x18000, 0x04000, 0x2977956d ) /* 0x6000 */
	ROM_LOAD("13413-01.105",   0x1c000, 0x04000, 0x569468a6 ) /* 0x8000 */
	ROM_LOAD("13414-01.106",   0x20000, 0x04000, 0xb178632d ) /* 0xA000 */
	ROM_LOAD("13415-01.107",   0x24000, 0x04000, 0x20b92eff ) /* 0xC000 */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("13401-00.u93", 0x00000, 0x04000, 0x4ea3e641 )
    ROM_LOAD("13402-00.u94", 0x08000, 0x04000, 0x71a8a56c )
    ROM_LOAD("13403-00.u95", 0x10000, 0x04000, 0x8077ae25 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
    ROM_LOAD("13416-00.u3",  0x00000, 0x02000, 0x37c960cf )
    ROM_LOAD("13417-00.u4",  0x10000, 0x04000, 0x97f044b5 )
    ROM_LOAD("13418-00.u5",  0x14000, 0x04000, 0x0931cfc0 )
    ROM_LOAD("13419-00.u6",  0x18000, 0x04000, 0xa7962b5a )
    ROM_LOAD("13420-00.u7",  0x1c000, 0x04000, 0x3c275262 )
    ROM_LOAD("13421-00.u8",  0x20000, 0x04000, 0x86f57c80 )
    ROM_LOAD("13422-00.u9",  0x24000, 0x04000, 0x222e8405 )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
    ROM_LOAD( "13404-00.u92",  0x04000, 0x4000, 0x22da40aa )
    ROM_LOAD( "13405-00.u69",  0x08000, 0x4000, 0x6f65b313 )
	/* U91 = Empty */
	/*
	U68 is a little strange. I would normally expect it to be
	0x4000 long. There is nothing missing since all the data
	is in the second half.
	*/
    ROM_LOAD( "13406-00.u68",  0x12000, 0x2000, 0xbb568693 )
    ROM_LOAD( "13407-00.u90",  0x14000, 0x4000, 0xe46ca57f )
    ROM_LOAD( "13408-00.u67",  0x18000, 0x4000, 0xbe637305 )
	/* 89 = Empty */
ROM_END

struct GameDriver wseries_driver =
{
	__FILE__,
	0,
	"wseries",
	"World Series Baseball",
	"1985",
	CINEMAT,
	"Paul Leaman",
	0,
	&wseries_machine,
	0,

	wseries_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_wseries,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};



/***************************************************************************

  Baseball 2

***************************************************************************/

static struct IOReadPort basebal2_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xff, 0xff, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort basebal2_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */
	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0xca, 0xcf, leland_gfx_port_w },   /* Video ports */
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfe, 0xfe, strkzone_banksw_w },
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xff, 0xff, leland_eeprom_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER_NO_SOUND(basebal2_machine, basebal2_readport, basebal2_writeport,
	strkzone_readmem, strkzone_writemem, strkzone_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( basebal2_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */
	ROM_LOAD("14115-00.101",   0x00000, 0x02000, 0x05231fee )
	ROM_LOAD("14116-00.102",   0x10000, 0x04000, 0xe1482ea3 ) /* 0x2000 */
	ROM_LOAD("14117-01.103",   0x14000, 0x04000, 0x677181dd ) /* 0x4000 */
	ROM_LOAD("14118-01.104",   0x18000, 0x04000, 0x5f570264 ) /* 0x6000 */
	ROM_LOAD("14119-01.105",   0x1c000, 0x04000, 0x90822145 ) /* 0x8000 */
	ROM_LOAD("14120-00.106",   0x20000, 0x04000, 0x4d2b7217 ) /* 0xA000 */
	ROM_LOAD("14121-01.107",   0x24000, 0x04000, 0xb987b97c ) /* 0xC000 */

	/* Extra banks (referred to as the "top" board). Probably an add-on */
    ROM_LOAD("14122-01.u2t",   0x28000, 0x02000, 0xa89882d8 ) /* 2000-3fff (1 page) */
    ROM_LOAD("14123-01.u3t",   0x2a000, 0x02000, 0xf9c51e5a ) /* 4000-5fff (1 page) */
							/* 6000-7fff (2 pages) - EMPTY */
	/* Remember to leave 0xc000 bytes here for paging */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("14112-00.u93", 0x00000, 0x04000, 0x8ccb1404 )
    ROM_LOAD("14113-00.u94", 0x08000, 0x04000, 0x9941a55b )
    ROM_LOAD("14114-00.u95", 0x10000, 0x04000, 0xb68baf47 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
    ROM_LOAD("14100-01.u3",  0x00000, 0x02000, 0x1dffbdaf )
    ROM_LOAD("14101-01.u4",  0x10000, 0x04000, 0xc585529c )
    ROM_LOAD("14102-01.u5",  0x14000, 0x04000, 0xace3f918 )
    ROM_LOAD("14103-01.u6",  0x18000, 0x04000, 0xcd41cf7a )
    ROM_LOAD("14104-01.u7",  0x1c000, 0x04000, 0x9b169e78 )
    ROM_LOAD("14105-01.u8",  0x20000, 0x04000, 0xec596b43 )
    ROM_LOAD("14106-01.u9",  0x24000, 0x04000, 0xb9656baa )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
    ROM_LOAD( "14111-01.u92",  0x04000, 0x4000, 0x2508a9ad )
    ROM_LOAD( "14109-00.u69",  0x08000, 0x4000, 0xb123a28e )
	/* U91 = Empty */
    ROM_LOAD( "14108-01.u68",  0x10000, 0x4000, 0xa1a51383 )
    ROM_LOAD( "14110-01.u90",  0x14000, 0x4000, 0xef01d997 )
    ROM_LOAD( "14107-00.u67",  0x18000, 0x4000, 0x976334e6 )
	/* 89 = Empty */
ROM_END

struct GameDriver basebal2_driver =
{
	__FILE__,
	0,
	"basebal2",
	"Baseball The Season II",
	"19878",
	CINEMAT,
	"Paul Leaman",
	0,
	&basebal2_machine,
	0,

	basebal2_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_strkzone,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Alley Master

  Uses the same hardware as basebal2

***************************************************************************/

INPUT_PORTS_START( input_ports_alleymas )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x40 */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Man Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1, "Man Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT)

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Man Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1, "Man Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT)

	/*
	A byte at 0xe0ca determines whether the joystick is
	available or not.
	The routine at 0x1825 clears the byte. I can't find anything
	that sets it.
	If after bootup, you change the value to non-zero the game will work.
	*/
	PORT_START      /* Analog joystick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
INPUT_PORTS_END

void alleymas_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	strkzone_init_machine();

	/* HACK!!!! Patch the code to get the controls working */
	RAM[0x1826]=1;
}

MACHINE_DRIVER_NO_SOUND(alleymas_machine, basebal2_readport, basebal2_writeport,
	strkzone_readmem, strkzone_writemem, alleymas_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);


ROM_START( alleymas_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */
	ROM_LOAD("101",   0x00000, 0x02000, 0x4273e260 )
	ROM_LOAD("102",   0x10000, 0x04000, 0xeb6575aa ) /* 0x2000 */
	ROM_LOAD("103",   0x14000, 0x04000, 0xcc9d778c ) /* 0x4000 */
	ROM_LOAD("104",   0x18000, 0x04000, 0x8edb129b ) /* 0x6000 */
	ROM_LOAD("105",   0x1c000, 0x04000, 0xa342dc8e ) /* 0x8000 */
	ROM_LOAD("106",   0x20000, 0x04000, 0xb396c254 ) /* 0xA000 */
	ROM_LOAD("107",   0x24000, 0x04000, 0x3ca13e8c ) /* 0xb000 */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("093", 0x00000, 0x02000, 0x54456e6f )
	ROM_LOAD("094", 0x08000, 0x02000, 0xedc240da )
	ROM_LOAD("095", 0x10000, 0x02000, 0x19793ed0 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("003",  0x00000, 0x02000, 0x3fee63ae )
	ROM_LOAD("004",  0x10000, 0x04000, 0xd302b5d1 )
	ROM_LOAD("005",  0x14000, 0x04000, 0x79bdb24d )
	ROM_LOAD("006",  0x18000, 0x04000, 0xf0b15d68 )
	ROM_LOAD("007",  0x1c000, 0x04000, 0x6974036c )
	ROM_LOAD("008",  0x20000, 0x04000, 0xa4357b5a )
	ROM_LOAD("009",  0x24000, 0x04000, 0x6d74274e )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/*
	This is a bit strange, the self test claims that the
	game uses U70, U92, U69, U91
	*/

	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "092",  0x04000, 0x2000, 0xa020eab5 )
	ROM_LOAD( "069",  0x08000, 0x2000, 0x79abb979 )
	/* U91 = Empty */
	ROM_LOAD( "068",  0x10000, 0x2000, 0x0c583385 )
	ROM_LOAD( "090",  0x14000, 0x2000, 0x0e1769e3 )
	/* U67 = Empty */
	/* U89 = Empty */
ROM_END

struct GameDriver alleymas_driver =
{
	__FILE__,
	0,
	"alleymas",
	"Alley Master",
	"198?",
	CINEMAT,
	"Paul Leaman",
	0,
	&alleymas_machine,
	0,

	alleymas_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_alleymas,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Mayhem 2002

***************************************************************************/

INPUT_PORTS_START( input_ports_mayhem )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x40 */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "Shoot", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "Check Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Check Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "Shoot", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Check Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Check Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)

	PORT_START      /* Analog joystick 1 */
	PORT_START
	PORT_START      /* Analog joystick 2 */
	PORT_START
	PORT_START      /* Analog joystick 3 */
	PORT_START
INPUT_PORTS_END

//MACHINE_DRIVER_NO_SOUND(mayhem_machine, basebal2_readport, basebal2_writeport,
//	strkzone_readmem, strkzone_writemem, strkzone_init_machine, gfxdecodeinfo,
//	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( mayhem_rom )
	ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13208.101",   0x00000, 0x04000, 0x04306973 )
	ROM_LOAD("13215.102",   0x10000, 0x04000, 0x06e689ae ) /* 0x2000 */
	ROM_LOAD("13216.103",   0x14000, 0x04000, 0x6452a82c ) /* 0x4000 */
	ROM_LOAD("13217.104",   0x18000, 0x04000, 0x62f6036e ) /* 0x6000 */
	ROM_LOAD("13218.105",   0x1c000, 0x04000, 0x162f5eb1 ) /* 0x8000 */
	ROM_LOAD("13219.106",   0x20000, 0x04000, 0xc0a74d6f ) /* 0xA000 */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("13204.93", 0x00000, 0x04000, 0xde183518 )
	ROM_LOAD("13205.94", 0x08000, 0x04000, 0xc61f63ac )
	ROM_LOAD("13206.95", 0x10000, 0x04000, 0x8e7bd2fd )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("13207.3",  0x00000, 0x04000, 0xbe1df6aa ) /* DO NOT TRIM THIS ROM */
	ROM_LOAD("13209.4",  0x10000, 0x04000, 0x39fcd7c6 ) /* 0x2000 */
	ROM_LOAD("13210.5",  0x14000, 0x04000, 0x630ed136 ) /* 0x4000 */
	ROM_LOAD("13211.6",  0x18000, 0x04000, 0x28b4aecd ) /* 0x6000 */
	ROM_LOAD("13212.7",  0x1c000, 0x04000, 0x1d6b39ab ) /* 0x8000 */
	ROM_LOAD("13213.8",  0x20000, 0x04000, 0xf3b2ea05 ) /* 0xa000 */
	ROM_LOAD("13214.9",  0x24000, 0x04000, 0x96f3e8d9 ) /* 0xc000 */

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */

	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* U70 = Empty */
	ROM_LOAD( "13203.92",  0x04000, 0x4000, 0x121ed5bf )
	ROM_LOAD( "13201.69",  0x08000, 0x4000, 0x90283e29 )
	/* U91 = Empty */
	/* U68 = Empty */
	/* U90 = Empty */
	/* U67 = Empty */
	ROM_LOAD( "13202.89",  0x1c000, 0x4000, 0xc5eaa4e3 )
ROM_END

struct GameDriver mayhem_driver =
{
	__FILE__,
	0,
	"mayhem",
	"Mayhem 2002",
	"1985",
	CINEMAT,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&basebal2_machine,
	0,

	mayhem_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_mayhem,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};



/***************************************************************************

  Cerberus

***************************************************************************/

INPUT_PORTS_START( input_ports_cerberus )
	PORT_START      /* 0x41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x40 */
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "Shoot", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "Check Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Check Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "Shoot", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "Check Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Check Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Double play */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x50 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)

	PORT_START      /* Analog joystick 1 */
	PORT_START
	PORT_START      /* Analog joystick 2 */
	PORT_START
	PORT_START      /* Analog joystick 3 */
	PORT_START
INPUT_PORTS_END

//MACHINE_DRIVER_NO_SOUND(cerberus_machine, basebal2_readport, basebal2_writeport,
//	strkzone_readmem, strkzone_writemem, strkzone_init_machine, gfxdecodeinfo,
//	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( cerberus_rom )
    ROM_REGION(STRKZONE_CODE_SIZE)     /* 64k for code + banked ROMs images */

    ROM_LOAD("3-23u101", 0x00000, 0x02000, 0xd78210df ) /*  */
    ROM_LOAD("3-23u102", 0x10000, 0x02000, 0xeed121ef ) /*  */
    ROM_LOAD("3-23u103", 0x14000, 0x02000, 0x45b82bf7 ) /*  */
    ROM_LOAD("3-23u104", 0x18000, 0x02000, 0xe133d6bf ) /*  */
    ROM_LOAD("3-23u105", 0x1c000, 0x02000, 0xa12c2c79 ) /*  */
    ROM_LOAD("3-23u106", 0x20000, 0x02000, 0xd64110d2 ) /*  */
    ROM_LOAD("3-23u107", 0x24000, 0x02000, 0x24e41c34 ) /*  */
	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("3-23u93", 0x00000, 0x02000, 0x14a1a4b0 )
    ROM_LOAD("3-23u94", 0x08000, 0x02000, 0x207a1709 )
    ROM_LOAD("3-23u95", 0x10000, 0x02000, 0xe9c86267 )


    ROM_REGION(0x28000)     /* Z80 slave CPU */
    ROM_LOAD("3-23u3",  0x00000, 0x02000, 0xb0579138 )
    ROM_LOAD("3-23u4",  0x10000, 0x02000, 0xba0dc990 ) /* 0x2000 */
    ROM_LOAD("3-23u5",  0x14000, 0x02000, 0xf8d6cc5d ) /* 0x4000 */
    ROM_LOAD("3-23u6",  0x18000, 0x02000, 0x42cdd393 ) /* 0x6000 */
    ROM_LOAD("3-23u7",  0x1c000, 0x02000, 0xc020148a ) /* 0x8000 */
    ROM_LOAD("3-23u8",  0x20000, 0x02000, 0xdbabdbde ) /* 0xa000 */
    ROM_LOAD("3-23u9",  0x24000, 0x02000, 0xeb992385 ) /* 0xc000 */


	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "3-23u70",  0x00000, 0x2000, 0x96499983 )
    ROM_LOAD( "3-23_u92", 0x04000, 0x2000, 0x497bb717 )
    ROM_LOAD( "3-23u69",  0x08000, 0x2000, 0xebd14d9e )
    ROM_LOAD( "3-23u91",  0x0c000, 0x2000, 0xb592d2e5 )
    ROM_LOAD( "3-23u68",  0x10000, 0x2000, 0xcfa7b8bf )
    ROM_LOAD( "3-23u90",  0x14000, 0x2000, 0xb7566f8a )
    ROM_LOAD( "3-23u67",  0x18000, 0x2000, 0x02b079a8 )
    ROM_LOAD( "3-23u89",  0x1c000, 0x2000, 0x7e5e82bb )
ROM_END

struct GameDriver cerberus_driver =
{
	__FILE__,
	0,
    "cerberus",
    "Cerberus",
    "198?",
	CINEMAT,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&basebal2_machine,
	0,

    cerberus_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_cerberus,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};


/***************************************************************************

  Pigout

***************************************************************************/

INPUT_PORTS_START( input_ports_pigout )
	PORT_START      /* GIN1 (0x41) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN0 (0x40) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* GIN1 (0x50) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2  )

	PORT_START      /* GIN3 (0x7f) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3|IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1  )
INPUT_PORTS_END

void pigout_init_machine(void)
{
	leland_init_machine();
	leland_rearrange_bank_swap(0, 0x10000);
}

void pigout_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *battery_bank=&RAM[0xa000];

	bank=data&0x07;

	if (bank<=1)
	{
		/* 0 = resident */
		bankaddress =0x2000;
		/* 1 = battery RAM */
		if (bank==1)
		{
			battery_bank=leland_battery_ram;
		}
	}
	else
	{
		bank-=2;
		bankaddress = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(2,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static struct MemoryReadAddress master_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },      /* Resident ROM */
	{ 0x2000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xdfff, MRA_BANK2 },  /* BATTERY RAM / ROM */
	{ 0xf000, 0xf3ff, paletteram_r },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress master_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_NOP },
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xdfff, leland_battery_w },  /* BATTERY RAM */
	{ 0xf000, 0xf3ff, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xf800, 0xf801, leland_master_video_addr_w },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};


static struct IOReadPort pigout_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },

	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0x7f, 0x7f, input_port_4_r },         /* Player 1 */
	{ -1 }  /* end of table */
};

static struct IOWritePort pigout_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */
	{ 0x80, 0x9f, leland_mvram_port_w }, /* track pack Video RAM ports */

	{ 0x40, 0x46, leland_mvram_port_w }, /* (stray) video RAM ports */
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4f, leland_gfx_port_w },   /* Video ports */

	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER(pigout_machine, pigout_readport, pigout_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	pigout_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( pigout_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("poutu58t.bin",  0x00000, 0x10000, 0x8fe4b683 ) /* CODE */
	ROM_LOAD("poutu59t.bin",  0x10000, 0x10000, 0xab907762 ) /* Banked code */
	ROM_LOAD("poutu57t.bin",  0x20000, 0x10000, 0xc22be0ff ) /* Banked code */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("poutu93.bin", 0x000000, 0x08000, 0xf102a04d ) /* Plane 3 */
	ROM_LOAD("poutu94.bin", 0x008000, 0x08000, 0xec63c015 ) /* Plane 2 */
	ROM_LOAD("poutu95.bin", 0x010000, 0x08000, 0xba6e797e ) /* Plane 1 */

	ROM_REGION(0x080000)     /* Z80 slave CPU */
	ROM_LOAD("poutu3.bin",   0x00000, 0x02000, 0xaf213cb7 ) /* Resident */
	ROM_LOAD("poutu2t.bin",  0x10000, 0x10000, 0xb23164c6 ) /* U2=0 & 1 */
	ROM_LOAD("poutu3t.bin",  0x20000, 0x10000, 0xd93f105f ) /* U3=2 & 3 */
	ROM_LOAD("poutu4t.bin",  0x30000, 0x10000, 0xb7c47bfe ) /* U4=4 & 5 */
	ROM_LOAD("poutu5t.bin",  0x40000, 0x10000, 0xd9b9dfbf ) /* U5=6 & 7 */
	ROM_LOAD("poutu6t.bin",  0x50000, 0x10000, 0x728c7c1a ) /* U6=8 & 9 */
	ROM_LOAD("poutu7t.bin",  0x60000, 0x10000, 0x393bd990 ) /* U7=a & b */
	ROM_LOAD("poutu8t.bin",  0x70000, 0x10000, 0xcb9ffaad ) /* U8=c & d */
															/* U9=e & f */

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("poutu25t.bin", 0x040000, 0x10000, 0x92cd2617 )
	ROM_LOAD_ODD ("poutu13t.bin", 0x040000, 0x10000, 0x9448c389 )
	ROM_LOAD_EVEN("poutu26t.bin", 0x060000, 0x10000, 0xab57de8f )
	ROM_LOAD_ODD ("poutu14t.bin", 0x060000, 0x10000, 0x30678e93 )
	ROM_LOAD_EVEN("poutu27t.bin", 0x0e0000, 0x10000, 0x37a8156e )
	ROM_LOAD_ODD ("poutu15t.bin", 0x0e0000, 0x10000, 0x1c60d58b )

	ROM_REGION(0x40000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "poutu70.bin",  0x00000, 0x4000, 0x7db4eaa1 )
	ROM_LOAD( "poutu92.bin",  0x04000, 0x4000, 0x20fa57bb )
	ROM_LOAD( "poutu69.bin",  0x08000, 0x4000, 0xa16886f3 )
	ROM_LOAD( "poutu91.bin",  0x0c000, 0x4000, 0x482a3581 )
	ROM_LOAD( "poutu68.bin",  0x10000, 0x4000, 0x7b62a3ed )
	ROM_LOAD( "poutu90.bin",  0x14000, 0x4000, 0x9615d710 )
	ROM_LOAD( "poutu67.bin",  0x18000, 0x4000, 0xaf85ce79 )
	ROM_LOAD( "poutu89.bin",  0x1c000, 0x4000, 0x6c874a05 )
ROM_END

struct GameDriver pigout_driver =
{
	__FILE__,
	0,
	"pigout",
	"Pigout",
	"1990",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&pigout_machine,
	0,

	pigout_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_pigout,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

ROM_START( pigoutj_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
	ROM_LOAD( "03-29020.01", 0x00000, 0x10000, 0x6c815982 ) /* CODE */
	ROM_LOAD( "03-29021.01", 0x10000, 0x10000, 0x9de7a763 ) /* Banked code */
	ROM_LOAD("poutu57t.bin", 0x20000, 0x10000, 0xc22be0ff ) /* Banked code */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("poutu95.bin", 0x000000, 0x08000, 0xba6e797e )
	ROM_LOAD("poutu94.bin", 0x008000, 0x08000, 0xec63c015 )
	ROM_LOAD("poutu93.bin", 0x010000, 0x08000, 0xf102a04d )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
	ROM_LOAD("poutu3.bin",   0x00000, 0x02000, 0xaf213cb7 )
	ROM_LOAD("poutu2t.bin",  0x10000, 0x10000, 0xb23164c6 ) /* U2=0 & 1 */
	ROM_LOAD("poutu3t.bin",  0x20000, 0x10000, 0xd93f105f ) /* U3=2 & 3 */
	ROM_LOAD("poutu4t.bin",  0x30000, 0x10000, 0xb7c47bfe ) /* U4=4 & 5 */
	ROM_LOAD("poutu5t.bin",  0x40000, 0x10000, 0xd9b9dfbf ) /* U5=6 & 7 */
	ROM_LOAD("poutu6t.bin",  0x50000, 0x10000, 0x728c7c1a ) /* U6=8 & 9 */
	ROM_LOAD("poutu7t.bin",  0x60000, 0x10000, 0x393bd990 ) /* U7=a & b */
	ROM_LOAD("poutu8t.bin",  0x70000, 0x10000, 0xcb9ffaad ) /* U8=c & d */

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("poutu25t.bin", 0x040000, 0x10000, 0x92cd2617 )
	ROM_LOAD_ODD ("poutu13t.bin", 0x040000, 0x10000, 0x9448c389 )
	ROM_LOAD_EVEN("poutu26t.bin", 0x060000, 0x10000, 0xab57de8f )
	ROM_LOAD_ODD ("poutu14t.bin", 0x060000, 0x10000, 0x30678e93 )
	ROM_LOAD_EVEN("poutu27t.bin", 0x0e0000, 0x10000, 0x37a8156e )
	ROM_LOAD_ODD ("poutu15t.bin", 0x0e0000, 0x10000, 0x1c60d58b )

	ROM_REGION(0x40000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "poutu70.bin",  0x00000, 0x4000, 0x7db4eaa1 ) /* 00 */
	ROM_LOAD( "poutu92.bin",  0x04000, 0x4000, 0x20fa57bb )
	ROM_LOAD( "poutu69.bin",  0x08000, 0x4000, 0xa16886f3 )
	ROM_LOAD( "poutu91.bin",  0x0c000, 0x4000, 0x482a3581 )
	ROM_LOAD( "poutu68.bin",  0x10000, 0x4000, 0x7b62a3ed )
	ROM_LOAD( "poutu90.bin",  0x14000, 0x4000, 0x9615d710 )
	ROM_LOAD( "poutu67.bin",  0x18000, 0x4000, 0xaf85ce79 )
	ROM_LOAD( "poutu89.bin",  0x1c000, 0x4000, 0x6c874a05 )
ROM_END

struct GameDriver pigoutj_driver =
{
	__FILE__,
	&pigout_driver,
	"pigoutj",
	"Pigout (Japanese)",
	"1990",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&pigout_machine,
	0,

	pigoutj_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_pigout,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Super Offroad

***************************************************************************/

INPUT_PORTS_START( input_ports_offroad )
	PORT_START      /* IN3 (0xc1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Unused */

	PORT_START      /* IN0 (0xc0)*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK ) /* Track pack */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog pedal 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog pedal 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER3, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER3, 100, 10, 0, 0, 255 )
INPUT_PORTS_END


static struct IOReadPort offroad_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0xf9, 0xf9, input_port_7_r },
	{ 0xfb, 0xfb, input_port_9_r },
	{ 0xf8, 0xf8, input_port_8_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort offroad_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */
	{ 0x40, 0x46, leland_mvram_port_w }, /* More video RAM ports */
	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xcf, leland_gfx_port_w },   /* Video ports */

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};


MACHINE_DRIVER(offroad_machine, offroad_readport, offroad_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	pigout_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( offroad_rom )
	ROM_REGION(0x040000)     /* 64k for code + banked ROMs images */
    ROM_LOAD("22121-04.u58",   0x00000, 0x10000, 0xc5790988 )
    ROM_LOAD("22122-03.u59",   0x10000, 0x10000, 0xae862fdc )
    ROM_LOAD("22120-01.u57",   0x20000, 0x10000, 0xe9f0f175 )
    ROM_LOAD("22119-02.u56",   0x30000, 0x10000, 0x38642f22 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("22105-01.u93", 0x00000, 0x08000, 0x4426e367 )
    ROM_LOAD("22106-02.u94", 0x08000, 0x08000, 0x687dc1fc )
    ROM_LOAD("22107-02.u95", 0x10000, 0x08000, 0xcee6ee5f )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("22100-01.u2",  0x00000, 0x02000, 0x08c96a4b )
	/* Strange, but two of these aren't accessed in the self test */
															/* U2=0 & 1 */
															/* U3=2 & 3 */
    ROM_LOAD("22108-02.u4",  0x30000, 0x10000, 0x0d72780a ) /* U4=4 & 5 ???NOT USED??? */
    ROM_LOAD("22109-02.u5",  0x40000, 0x10000, 0x5429ce2c ) /* U5=6 & 7 */
    ROM_LOAD("22110-02.u6",  0x50000, 0x10000, 0xf97bad5c ) /* U6=8 & 9 */
    ROM_LOAD("22111-01.u7",  0x60000, 0x10000, 0xf79157a1 ) /* U7=a & b ???NOT USED??? */
    ROM_LOAD("22112-01.u8",  0x70000, 0x10000, 0x3eef38d3 ) /* U8=c & d */

	ROM_REGION(0x100000)     /* 80186 CPU */
    ROM_LOAD_EVEN("22116-03.u25", 0x040000, 0x10000, 0x95bb31d3 )
    ROM_LOAD_ODD ("22113-03.u13", 0x040000, 0x10000, 0x71b28df6 )
    ROM_LOAD_EVEN("22117-03.u26", 0x060000, 0x10000, 0x703d81ce )
    ROM_LOAD_ODD ("22114-03.u14", 0x060000, 0x10000, 0xf8b31bf8 )
    ROM_LOAD_EVEN("22118-03.u27", 0x0e0000, 0x10000, 0x806ccf8b )
    ROM_LOAD_ODD ("22115-03.u15", 0x0e0000, 0x10000, 0xc8439a7a )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* 70 = empty */
    ROM_LOAD( "22104-01.u92",  0x04000, 0x4000, 0x03e0497d )
    ROM_LOAD( "22102-01.u69",  0x08000, 0x4000, 0xc3f2e443 )
	/* 91 = empty */
	/* 68 = empty */
    ROM_LOAD( "22103-02.u90",  0x14000, 0x4000, 0x2266757a )
    ROM_LOAD( "22101-01.u67",  0x18000, 0x4000, 0xecab0527 )
	/* 89 = empty */
ROM_END

struct GameDriver offroad_driver =
{
	__FILE__,
	0,
	"offroad",
	"Super Off-road Racer",
	"1989",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&offroad_machine,
	0,

	offroad_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_offroad,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};


ROM_START( offroadt_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("ortpu58.bin",   0x00000, 0x10000, 0xadbc6211 )
	ROM_LOAD("ortpu59.bin",   0x10000, 0x10000, 0x296dd3b6 )
	ROM_LOAD("ortpu57.bin",   0x20000, 0x10000, 0xe9f0f175 )  /* Identical to offroad */
	ROM_LOAD("ortpu56.bin",   0x30000, 0x10000, 0x2c1a22b3 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("ortpu93b.bin", 0x00000, 0x08000, 0xf0c1d8b0 )
    ROM_LOAD("ortpu94b.bin", 0x08000, 0x08000, 0x7460d8c0 )
    ROM_LOAD("ortpu95b.bin", 0x10000, 0x08000, 0x081ee7a8 )

	ROM_REGION(0x90000)     /* Z80 slave CPU */
    ROM_LOAD("ortpu3b.bin", 0x00000, 0x02000, 0x95abb9f1 )
	ROM_LOAD("ortpu2.bin",  0x10000, 0x10000, 0xc46c1627 )
	ROM_LOAD("ortpu3.bin",  0x20000, 0x10000, 0x2276546f )
	ROM_LOAD("ortpu4.bin",  0x30000, 0x10000, 0xaa4b5975 )
	ROM_LOAD("ortpu5.bin",  0x40000, 0x10000, 0x69100b06 )
	ROM_LOAD("ortpu6.bin",  0x50000, 0x10000, 0xb75015b8 )
	ROM_LOAD("ortpu7.bin",  0x60000, 0x10000, 0xa5af5b4f )
	ROM_LOAD("ortpu8.bin",  0x70000, 0x10000, 0x0f735078 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("ortpu25.bin", 0x040000, 0x10000, 0xf952f800 )
	ROM_LOAD_ODD ("ortpu13.bin", 0x040000, 0x10000, 0x7beec9fc )
	ROM_LOAD_EVEN("ortpu26.bin", 0x060000, 0x10000, 0x6227ea94 )
	ROM_LOAD_ODD ("ortpu14.bin", 0x060000, 0x10000, 0x0a44331d )
	ROM_LOAD_EVEN("ortpu27.bin", 0x0e0000, 0x10000, 0xb80c5f99 )
	ROM_LOAD_ODD ("ortpu15.bin", 0x0e0000, 0x10000, 0x2a1a1c3c )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	/* 70 = empty */
    ROM_LOAD( "ortpu92b.bin",  0x04000, 0x4000, 0xf9988e28 )
    ROM_LOAD( "ortpu69b.bin",  0x08000, 0x4000, 0xfe5f8d8f )
	/* 91 = empty */
	/* 68 = empty */
    ROM_LOAD( "ortpu90b.bin",  0x14000, 0x4000, 0xbda2ecb1 )
    ROM_LOAD( "ortpu67b.bin",  0x1c000, 0x4000, 0x38c9bf29 )
	/* 89 = empty */
ROM_END

struct GameDriver offroadt_driver =
{
	__FILE__,
	0,
	"offroadt",
	"Super Off-road Racer (Track Pack)",
	"????",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&pigout_machine,
	0,

	offroadt_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_offroad,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};


/***************************************************************************

  Team QB

***************************************************************************/

INPUT_PORTS_START( input_ports_teamqb )
	PORT_START      /* GIN1 (0x81) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN0 (0x80) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x91) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN1 (0x90) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog spring stick 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog spring stick 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog spring stick 3 */
	PORT_START      /* Analog spring stick 4 */
	PORT_START      /* Analog spring stick 5 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog spring stick 5 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 10, 0, 0, 255 )

	PORT_START      /* GIN3 (0x7c) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* GIN3 (0x7f) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1  | IPF_PLAYER1)
INPUT_PORTS_END

static struct IOReadPort teamqb_readport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_r }, /* Video RAM ports */
	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, leland_slavebit0_r },
	{ 0x83, 0x83, AY8910_read_port_0_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0x91, 0x91, input_port_2_r },
	{ 0x7c, 0x7c, input_port_9_r },
	{ 0x7f, 0x7f, input_port_10_r },
	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_analog_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort teamqb_writeport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x89, 0x89, leland_slave_cmd_w },
	{ 0x8a, 0x8f, leland_gfx_port_w },   /* Video ports */

	{ 0xf0, 0xf0, pigout_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, leland_analog_w },
	{ 0xfe, 0xfe, leland_analog_w },

	{ -1 }  /* end of table */
};

MACHINE_DRIVER(teamqb_machine, teamqb_readport, teamqb_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( teamqb_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15618-03.58t",   0x00000, 0x10000, 0xb32568dc )
	/* One of these is suspect (or the banking is wrong) */
	ROM_LOAD("15619-02.59t",   0x10000, 0x10000, 0x6d533714 )
	ROM_LOAD("15619-03.59t",   0x20000, 0x10000, 0x40b3319f )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("15615-01.u93", 0x00000, 0x04000, 0xa7ea6a87 )
    ROM_LOAD("15616-01.u94", 0x08000, 0x04000, 0x4a9b3900 )
    ROM_LOAD("15617-01.u95", 0x10000, 0x04000, 0x2cd95edb )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("15600-01.u3",   0x00000, 0x02000, 0x46615844 )
    ROM_LOAD("15601-01.u2t",  0x10000, 0x10000, 0x8e523c58 )
    ROM_LOAD("15602-01.u3t",  0x20000, 0x10000, 0x545b27a1 )
    ROM_LOAD("15603-01.u4t",  0x30000, 0x10000, 0xcdc9c09d )
    ROM_LOAD("15604-01.u5t",  0x40000, 0x10000, 0x3c03e92e )
    ROM_LOAD("15605-01.u6t",  0x50000, 0x10000, 0xcdf7d19c )
    ROM_LOAD("15606-01.u7t",  0x60000, 0x10000, 0x8eeb007c )
    ROM_LOAD("15607-01.u8t",  0x70000, 0x10000, 0x57cb6d2d )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("15623-01.25t", 0x040000, 0x10000, 0x710bdc76 )
	ROM_LOAD_ODD ("15620-01.13t", 0x040000, 0x10000, 0x7e5cb8ad )
	ROM_LOAD_EVEN("15624-01.26t", 0x060000, 0x10000, 0xdd090d33 )
	ROM_LOAD_ODD ("15621-01.14t", 0x060000, 0x10000, 0xf68c68c9 )
	ROM_LOAD_EVEN("15625-01.27t", 0x0e0000, 0x10000, 0xac442523 )
	ROM_LOAD_ODD ("15622-01.15t", 0x0e0000, 0x10000, 0x9e84509a )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "15611-01.u70",  0x00000, 0x4000, 0xbf2695fb )
    ROM_LOAD( "15614-01.u92",  0x04000, 0x4000, 0xc93fd870 )
    ROM_LOAD( "15610-01.u69",  0x08000, 0x4000, 0x3e5b786f )
	/* 91 = empty */
    ROM_LOAD( "15609-01.u68",  0x10000, 0x4000, 0x0319aec7 )
    ROM_LOAD( "15613-01.u90",  0x14000, 0x4000, 0x4805802e )
    ROM_LOAD( "15608-01.u67",  0x18000, 0x4000, 0x78f0fd2b )
	/* 89 = empty */
ROM_END

struct GameDriver teamqb_driver =
{
	__FILE__,
	0,
	"teamqb",
	"Team Quaterback",
	"198?",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&teamqb_machine,
	0,

	teamqb_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_teamqb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};


/***************************************************************************

  Redline Racer 2 Player

***************************************************************************/

INPUT_PORTS_START( input_ports_redlin2p )
	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x00, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )

	PORT_START      /* IN0 (0xc0)*/
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE|IPF_TOGGLE, "Service", KEYCODE_F2, IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )  /* Unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )  /* Unused */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_COIN3 )  /* Unused */

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_VBLANK )
//    PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_START1 )
//    PORT_ANALOG ( 0xfe, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_DIAL|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_DIAL|IPF_PLAYER1, 100, 10, 0, 0, 255 )
INPUT_PORTS_END

void redlin2p_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
//	unsigned char *battery_bank=&RAM[0xa000];

/*    char baf[40];
	sprintf(baf,"Bank=%d",data&0x03);
	usrintf_showmessage(baf);*/

	bank=data&0x03;


	if (bank & 0x02)
	{
		/* resident */
		bankaddress =0x2000;
	}
	else
	{
		bankaddress  = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(2,&RAM[bankaddress+(0xa000-0x2000)]);    /* 0x2000-0x9fff */
//    cpu_setbank(2,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static int redlin2p_kludge_r(int offset)
{
static int s;
s=~s;
return s;
}


static struct IOReadPort redlin2p_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc1, 0xc1, input_port_1_r },
	{ 0xc0, 0xc0, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, redlin2p_kludge_r },
//    { 0xf2, 0xf2, leland_sound_comm_r },
	{ 0xf8, 0xf8, input_port_4_r },         /* Wheel */
	{ 0xfb, 0xfb, input_port_4_r },         /* Wheel */
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort redlin2p_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xcf, leland_gfx_port_w },   /* Video ports */

	{ 0xf0, 0xf0, redlin2p_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};

void redlin2p_init_machine(void)
{
//	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	leland_init_machine();
	leland_rearrange_bank_swap(0, 0x10000);
	leland_rearrange_bank(1, 0x10000);

	/* Kludge ... jump to self test */
/*    RAM[0x1e]=0x52;
	RAM[0x1f]=0x9d;*/
}

MACHINE_DRIVER_NO_SOUND(redlin2p_machine, redlin2p_readport, redlin2p_writeport,
	master_readmem, master_writemem, redlin2p_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( redlin2p_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13932-01.23t", 0x00000, 0x10000, 0xecdf0fbe )
	ROM_LOAD("13931-01.22t", 0x10000, 0x10000, 0x16d01978 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("13930-01.u93", 0x00000, 0x04000, 0x0721f42e )
    ROM_LOAD("13929-01.u94", 0x08000, 0x04000, 0x1522e7b2 )
    ROM_LOAD("13928-01.u95", 0x10000, 0x04000, 0xc321b5d1 )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("13907-01.u3",  0x00000, 0x04000, 0xb760d63e )
    ROM_LOAD("13908-01.u4",  0x10000, 0x04000, 0xa30739d3 )
    ROM_LOAD("13909-01.u5",  0x10000, 0x04000, 0xaaf16ad7 )
    ROM_LOAD("13910-01.u6",  0x10000, 0x04000, 0xd03469eb )
    ROM_LOAD("13911-01.u7",  0x10000, 0x04000, 0x8ee1f547 )
    ROM_LOAD("13912-01.u8",  0x10000, 0x04000, 0xe5b57eac )
    ROM_LOAD("13913-01.u9",  0x10000, 0x04000, 0x02886071 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN("28t",    0x0e0000, 0x10000, 0x7aa21b2c )
	ROM_LOAD_ODD ("17t",    0x0e0000, 0x10000, 0x8d26f221 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "13920-01.u70",  0x00000, 0x4000, 0xf343d34a )
    ROM_LOAD( "13921-01.u92",  0x04000, 0x4000, 0xc9ba8d41 )
    ROM_LOAD( "13922-01.u69",  0x08000, 0x4000, 0x276cfba0 )
    ROM_LOAD( "13923-01.u91",  0x0c000, 0x4000, 0x4a88ea34 )
    ROM_LOAD( "13924-01.u68",  0x10000, 0x4000, 0x3995cb7e )
	/* 90 = empty / missing */
    ROM_LOAD( "13926-01.u67",  0x18000, 0x4000, 0xdaa30add )
    ROM_LOAD( "13927-01.u89",  0x1c000, 0x4000, 0x30e60fb5 )
ROM_END

struct GameDriver redlin2p_driver =
{
	__FILE__,
	0,
	"redlin2p",
	"Redline Racer (2 player version)",
	"1987",
	"Cinematronics",
	"Paul Leaman",
	GAME_NOT_WORKING,
	&redlin2p_machine,
	0,

	redlin2p_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_redlin2p,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Danger Zone

***************************************************************************/

INPUT_PORTS_START( input_ports_dangerz )
	PORT_START      /* IN3 (0x81)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN0 (0x80)*/
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2, "Missile", IP_KEY_DEFAULT, IP_JOY_DEFAULT  )

	PORT_START      /* IN1 (0x91)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* in2 (0x90) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog 1 */
	PORT_ANALOG ( 0x03, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 3 )
	PORT_START      /* Analog 2 */
	PORT_ANALOG ( 0x03, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 3 )
INPUT_PORTS_END

void dangerz_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress=(data&0x01)*0x10000;
	if (errorlog)
	{
		fprintf(errorlog, "DANGERZ BANK=%02x\n", data);
	}
	cpu_setbank(1, &RAM[0x02000+bankaddress]);
	cpu_setbank(2, &RAM[0x0a000+bankaddress]);
}

static struct IOReadPort dangerz_readport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, leland_slavebit0_r },
	{ 0x83, 0x83, AY8910_read_port_0_r },
	{ 0x90, 0x90, input_port_3_r },
	{ 0x91, 0x91, input_port_2_r },
//    { 0xf2, 0xf2, redlin2p_kludge_r },
	{ 0xf4, 0xf4, input_port_4_r },
	{ 0xf8, 0xf8, input_port_5_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort dangerz_writeport[] =
{
	{ 0x40, 0x5f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0x89, 0x89, leland_slave_cmd_w },
	{ 0x8a, 0x8f, leland_gfx_port_w },   /* Video ports */

	{ 0xf0, 0xf0, dangerz_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, leland_analog_w },
	{ -1 }  /* end of table */
};


void dangerz_init_machine(void)
{
	leland_init_machine();
	leland_rearrange_bank(1, 0x10000);
}

MACHINE_DRIVER_NO_SOUND(dangerz_machine, dangerz_readport, dangerz_writeport,
	master_readmem, master_writemem, dangerz_init_machine, gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem,slave_writemem);

ROM_START( dangerz_rom )
	ROM_REGION(0x020000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("13823.12t",   0x00000, 0x10000, 0x31604634 )
	ROM_LOAD("13824.13t",   0x10000, 0x10000, 0x381026c6 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("13801.93", 0x00000, 0x04000, 0xf9ff55ec )
	ROM_LOAD("13802.94", 0x08000, 0x04000, 0xd4adbcbb )
	ROM_LOAD("13803.95", 0x10000, 0x04000, 0x9178ed76 )

	ROM_REGION(0x28000)     /* Z80 slave CPU */
	ROM_LOAD("13818.3",   0x00000, 0x04000, 0x71863c5b )
	ROM_LOAD("13817.4",   0x10000, 0x04000, 0x924bead3 )
	ROM_LOAD("13818.5",   0x14000, 0x04000, 0x403bdfea )
	ROM_LOAD("13819.6",   0x18000, 0x04000, 0x1fee5f10 )
	ROM_LOAD("13820.7",   0x1c000, 0x04000, 0x42657a1e )
	ROM_LOAD("13821.8",   0x20000, 0x04000, 0x92f3e006 )

	ROM_REGION(0x1)     /* 80186 CPU */

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
	ROM_LOAD( "13809.70",  0x00000, 0x4000, 0xe44eb9f5 )
	ROM_LOAD( "13804.92",  0x04000, 0x4000, 0x6c23f1a5 )
	ROM_LOAD( "13805.69",  0x08000, 0x4000, 0xe9c9f38b )
	ROM_LOAD( "13808.91",  0x0c000, 0x4000, 0x035534ad )
	ROM_LOAD( "13806.68",  0x10000, 0x4000, 0x2dbd64d2 )
	ROM_LOAD( "13808.90",  0x14000, 0x4000, 0xd5b4985d )
	ROM_LOAD( "13822.67",  0x18000, 0x4000, 0x00ff3033 )
	ROM_LOAD( "13810.89",  0x1c000, 0x4000, 0x4f645973 )
ROM_END

struct GameDriver dangerz_driver =
{
	__FILE__,
	0,
	"dangerz",
	"Danger Zone",
	"1986",
	CINEMAT,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&dangerz_machine,
	0,
	dangerz_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_dangerz,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};




INPUT_PORTS_START( input_ports_viper )
	PORT_START      /* IN3 (0xc1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )  /* Unused */

	PORT_START      /* IN0 (0xc0)*/
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON1 )  /* Unused */

	PORT_START      /* IN1 (0xd1)*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* in2 (0xd0) */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Analog pedal 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog pedal 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog pedal 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y|IPF_PLAYER3, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER1, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 2 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER2, 100, 10, 0, 0, 255 )
	PORT_START      /* Analog wheel 3 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X|IPF_PLAYER3, 100, 10, 0, 0, 255 )
INPUT_PORTS_END

void viper_banksw_w(int offset, int data)
{
	int bank;
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *battery_bank=&RAM[0xa000];

	bank=data&0x07;
#ifdef NOISY_CPU
	if (errorlog)
	{
		fprintf(errorlog, "BANK=%02x\n", bank);
	}
#endif
	if (!bank)
	{
		/* 0 = resident */
		bankaddress =0x2000;
	}
	else
	{
		bank--;
		bankaddress  = 0x10000 + bank * 0x8000;
	}

	cpu_setbank(1,&RAM[bankaddress]);    /* 0x2000-0x9fff */
	cpu_setbank(2,battery_bank);            /* 0xa000-0xdfff */

	leland_sound_cpu_control_w(data);
}

static struct IOReadPort viper_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0xf9, 0xf9, input_port_7_r },
	{ 0xfb, 0xfb, input_port_9_r },
	{ 0xf8, 0xf8, input_port_8_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort viper_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xcf, leland_gfx_port_w },   /* Video ports */

	{ 0xf0, 0xf0, viper_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER(viper_machine, viper_readport, viper_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( viper_rom )
	ROM_REGION(0x050000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("15617-03.49t",   0x00000, 0x10000, 0x7e4688a6 )
	ROM_LOAD("15616-03.48t",   0x10000, 0x10000, 0x3fe2f0bf )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("15609-01.u93", 0x00000, 0x04000, 0x08ad92e9 )
    ROM_LOAD("15610-01.u94", 0x08000, 0x04000, 0xd4e56dfb )
    ROM_LOAD("15611-01.u95", 0x10000, 0x04000, 0x3a2c46fb )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("15600-02.u3", 0x00000, 0x02000, 0x0f57f68a )
    ROM_LOAD("viper.u2t",   0x10000, 0x10000, 0x4043d4ee )
    ROM_LOAD("viper.u3t",   0x20000, 0x10000, 0x213bc02b )
    ROM_LOAD("viper.u4t",   0x30000, 0x10000, 0xce0b95b4 )

	ROM_REGION(0x100000)     /* 80186 CPU */
	ROM_LOAD_EVEN( "15620-02.45t", 0x040000, 0x10000, 0x7380ece1 )
	ROM_LOAD_ODD ( "15623-02.62t", 0x040000, 0x10000, 0x2921d8f9 )
	ROM_LOAD_EVEN( "15619-02.44t", 0x060000, 0x10000, 0xc8507cc2 )
	ROM_LOAD_ODD ( "15622-02.61t", 0x060000, 0x10000, 0x32dfda37 )
	ROM_LOAD_EVEN( "15618-02.43t", 0x0e0000, 0x10000, 0x5562e0c3 )
	ROM_LOAD_ODD ( "15621-02.60t", 0x0e0000, 0x10000, 0xcb468f2b )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "15604-01.u70",  0x00000, 0x4000, 0x7e3b0cce )
    ROM_LOAD( "15608-01.u92",  0x04000, 0x4000, 0xa9bde0ef )
    ROM_LOAD( "15603-01.u69",  0x08000, 0x4000, 0xaecc9516 )
    ROM_LOAD( "15607-01.u91",  0x0c000, 0x4000, 0x14f06f88 )
    ROM_LOAD( "15602-01.u68",  0x10000, 0x4000, 0x4ef613ad )
    ROM_LOAD( "15606-01.u90",  0x14000, 0x4000, 0x3c2e8e76 )
    ROM_LOAD( "15601-01.u67",  0x18000, 0x4000, 0xdc7006cd )
    ROM_LOAD( "15605-01.u89",  0x1c000, 0x4000, 0x4aa9c788 )
ROM_END

struct GameDriver viper_driver =
{
	__FILE__,
	0,
	"viper",
	"Viper",
	"1988",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&viper_machine,
	0,

	viper_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_viper,

	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	leland_hiload,leland_hisave
};



INPUT_PORTS_START( input_ports_aafb )
	PORT_START      /* GIN1 (0x41) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* GIN0 (0x40) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1  ) /* 2U */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2  ) /* 2U */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* GIN1 (0x50) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2  )

	PORT_START      /* GIN3 (0x7c) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct IOReadPort aafb_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* offroad video RAM ports */

	{ 0x40, 0x40, input_port_1_r },
	{ 0x41, 0x41, leland_slavebit0_r },
	{ 0x43, 0x43, AY8910_read_port_0_r },
	{ 0x50, 0x50, input_port_3_r },
	{ 0x51, 0x51, input_port_2_r },
	{ 0x7c, 0x7c, input_port_4_r },

	{ 0xf2, 0xf2, leland_sound_response_r },
	{ -1 }  /* end of table */
};


static struct IOWritePort aafb_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* track pack Video RAM ports */

	{ 0x40, 0x46, leland_mvram_port_w }, /* (stray) video RAM ports */
	{ 0x49, 0x49, leland_slave_cmd_w },
	{ 0x4a, 0x4f, leland_gfx_port_w },   /* Video ports */

	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },

	{ 0xf0, 0xf0, viper_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER(aafb_machine, aafb_readport, aafb_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	pigout_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( aafb_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
    ROM_LOAD("24014-02.u58",   0x00000, 0x10000, 0x5db4a3d0 ) /* SUSPECT */
    ROM_LOAD("24015-02.u59",   0x10000, 0x10000, 0x00000000 ) /* SUSPECT */

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD("24011-02.u93", 0x00000, 0x04000, 0x011c0235 )  /* SUSPECT */
    ROM_LOAD("24012-02.u94", 0x08000, 0x04000, 0x376199a2 )  /* SUSPECT */
    ROM_LOAD("24013-02.u95", 0x10000, 0x04000, 0x0a604e0d )  /* SUSPECT */

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("24000-02.u3",   0x00000, 0x02000, 0x52df0354 ) /* SUSPECT */
    ROM_LOAD("24001-02.u2t",  0x10000, 0x10000, 0x9b20697d ) /* SUSPECT */
    ROM_LOAD("24002-02.u3t",  0x20000, 0x10000, 0xbbb92184 )
    ROM_LOAD("15603-01.u4t",  0x30000, 0x10000, 0xcdc9c09d )
    ROM_LOAD("15604-01.u5t",  0x40000, 0x10000, 0x3c03e92e )
    ROM_LOAD("15605-01.u6t",  0x50000, 0x10000, 0xcdf7d19c )
    ROM_LOAD("15606-01.u7t",  0x60000, 0x10000, 0x8eeb007c )
    ROM_LOAD("24002-02.u8t",  0x70000, 0x10000, 0x3d9747c9 )

	ROM_REGION(0x100000)     /* 80186 CPU */
    ROM_LOAD_EVEN("24019-01.u25", 0x040000, 0x10000, 0x9e344768 )
    ROM_LOAD_ODD ("24016-01.u13", 0x040000, 0x10000, 0x6997025f )
    ROM_LOAD_EVEN("24020-01.u26", 0x060000, 0x10000, 0x0788f2a5 )
    ROM_LOAD_ODD ("24017-01.u14", 0x060000, 0x10000, 0xa48bd721 )
    ROM_LOAD_EVEN("24021-01.u27", 0x0e0000, 0x10000, 0x94081899 )
    ROM_LOAD_ODD ("24018-01.u15", 0x0e0000, 0x10000, 0x76eb6077 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "24007-01.u70",  0x00000, 0x4000, 0x40e46aa4 )
    ROM_LOAD( "24010-01.u92",  0x04000, 0x4000, 0x78705f42 )
    ROM_LOAD( "24006-01.u69",  0x08000, 0x4000, 0x6a576aa9 )
    ROM_LOAD( "24009-02.u91",  0x0c000, 0x4000, 0xb857a1ad )
    ROM_LOAD( "24005-02.u68",  0x10000, 0x4000, 0x8ea75319 )
    ROM_LOAD( "24008-01.u90",  0x14000, 0x4000, 0x4538bc58 )
    ROM_LOAD( "24004-02.u67",  0x18000, 0x4000, 0xcd7a3338 )
	/* 89 = empty */
ROM_END

struct GameDriver aafb_driver =
{
	__FILE__,
	0,
	"aafb",
	"All American Football",
	"????",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&aafb_machine,
	0,

	aafb_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_aafb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};


ROM_START( aafb2p_rom )
	ROM_REGION(0x020000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("aafb2p.58t",   0x00000, 0x10000, 0x79fd14cd )
	ROM_LOAD("aafb2p.59t",   0x10000, 0x10000, 0x3b0382f0 )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */

    ROM_LOAD("aafb2p.u93",   0x00000, 0x08000, 0x00000000 )
    ROM_LOAD("aafb2p.u94",   0x08000, 0x08000, 0x00000000 )
    ROM_LOAD("aafb2p.u95",   0x10000, 0x08000, 0x00000000 )

	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("aafb2p.u3",   0x00000, 0x04000, 0x7a8259c4 )
    ROM_LOAD("aafb2p.u2t",  0x10000, 0x10000, 0xf118b9b4 )
    ROM_LOAD("aafb2p.u3t",  0x20000, 0x10000, 0xbbb92184 )
    ROM_LOAD("aafb2p.u4t",  0x30000, 0x10000, 0xcdc9c09d )
    ROM_LOAD("aafb2p.u5t",  0x40000, 0x10000, 0x3c03e92e )
    ROM_LOAD("aafb2p.u6t",  0x50000, 0x10000, 0xcdf7d19c )
    ROM_LOAD("aafb2p.u7t",  0x60000, 0x10000, 0x8eeb007c )
    ROM_LOAD("aafb2p.u8t",  0x70000, 0x10000, 0x3d9747c9 )

	ROM_REGION(0x100000)     /* 80186 CPU */
    ROM_LOAD_EVEN("aafb2p.25t", 0x040000, 0x10000, 0x9e344768 )
    ROM_LOAD_ODD ("aafb2p.13t", 0x040000, 0x10000, 0x6997025f )
    ROM_LOAD_EVEN("aafb2p.26t", 0x060000, 0x10000, 0x0788f2a5 )
    ROM_LOAD_ODD ("aafb2p.14t", 0x060000, 0x10000, 0xa48bd721 )
	ROM_LOAD_EVEN("aafb2p.27t", 0x0e0000, 0x10000, 0x94081899 )
	ROM_LOAD_ODD ("aafb2p.15t", 0x0e0000, 0x10000, 0x76eb6077 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "aafb2p.u70",  0x00000, 0x4000, 0x40e46aa4 )
    ROM_LOAD( "aafb2p.u92",  0x04000, 0x4000, 0x78705f42 )
    ROM_LOAD( "aafb2p.u69",  0x08000, 0x4000, 0x6a576aa9 )
    ROM_LOAD( "aafb2p.u91",  0x0c000, 0x4000, 0xb857a1ad )
    ROM_LOAD( "aafb2p.u68",  0x10000, 0x4000, 0x8ea75319 )
    ROM_LOAD( "aafb2p.u90",  0x14000, 0x4000, 0x4538bc58 )
    ROM_LOAD( "aafb2p.u67",  0x18000, 0x4000, 0xcd7a3338 )
	/* 89 = empty */
ROM_END

struct GameDriver aafb2p_driver =
{
	__FILE__,
	0,
	"aafb2p",
    "All American Football (2 player)",
	"????",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&aafb_machine,
	0,

	aafb2p_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_aafb,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};


INPUT_PORTS_START( input_ports_aafbu )
	PORT_START      /* GIN1 (0x41) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* GIN0 (0x40) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* (0x51) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* GIN1 (0x50) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2  )

	PORT_START      /* GIN3 (0x7c) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct IOReadPort aafbu_readport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_r }, /* Video RAM ports */

	{ 0xc0, 0xc0, input_port_1_r },
	{ 0xc1, 0xc1, leland_slavebit0_r },
	{ 0xc3, 0xc3, AY8910_read_port_0_r },
	{ 0xd0, 0xd0, input_port_3_r },
	{ 0xd1, 0xd1, input_port_2_r },
	{ 0xdc, 0xdc, input_port_4_r },
	{ 0xf2, 0xf2, leland_sound_response_r },
	{ 0xfd, 0xfd, leland_analog_r },
	{ 0xfe, 0xfe, leland_eeprom_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort aafbu_writeport[] =
{
	{ 0x00, 0x1f, leland_mvram_port_w }, /* Video RAM ports */

	{ 0xc9, 0xc9, leland_slave_cmd_w },
	{ 0x8a, 0x8a, AY8910_control_port_1_w },
	{ 0x8b, 0x8b, AY8910_write_port_1_w },
	{ 0xca, 0xcf, leland_gfx_port_w },   /* Video ports */
	{ 0xf0, 0xf0, viper_banksw_w },
	{ 0xf2, 0xf2, leland_sound_cmd_low_w },
	{ 0xf4, 0xf4, leland_sound_cmd_high_w },
	{ 0xfd, 0xfd, MWA_NOP },            /* Reset analog ? */
	{ 0xfe, 0xfe, leland_analog_w },
	{ -1 }  /* end of table */
};

MACHINE_DRIVER(aafbu_machine, aafbu_readport, aafbu_writeport,
	master_readmem, master_writemem, pigout_init_machine,gfxdecodeinfo,
	leland_vh_screenrefresh, slave_readmem2,slave_writemem2);

ROM_START( aafbu_rom )
	ROM_REGION(0x048000)     /* 64k for code + banked ROMs images */
	ROM_LOAD("aafbu58t.bin",   0x00000, 0x10000, 0xfa75a4a0 )
	ROM_LOAD("aafbu59t.bin",   0x10000, 0x10000, 0xab6a606f )

	ROM_REGION_DISPOSE(0x18000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("aafbu93.bin",   0x00000, 0x04000, 0x68f8addc )
	ROM_LOAD("aafbu94.bin",   0x08000, 0x04000, 0x669791ac )
	ROM_LOAD("aafbu95.bin",   0x10000, 0x04000, 0xbd62aa8a )

	/* Everything from here down may be from the wrong version */
	ROM_REGION(0x80000)     /* Z80 slave CPU */
    ROM_LOAD("24000-02.u3",   0x00000, 0x02000, 0x52df0354 )
    ROM_LOAD("24001-02.u2t",  0x10000, 0x10000, 0x9b20697d )
    ROM_LOAD("24002-02.u3t",  0x20000, 0x10000, 0xbbb92184 )
    ROM_LOAD("15603-01.u4t",  0x30000, 0x10000, 0xcdc9c09d )
    ROM_LOAD("15604-01.u5t",  0x40000, 0x10000, 0x3c03e92e )
    ROM_LOAD("15605-01.u6t",  0x50000, 0x10000, 0xcdf7d19c )
    ROM_LOAD("15606-01.u7t",  0x60000, 0x10000, 0x8eeb007c )
    ROM_LOAD("24002-02.u8t",  0x70000, 0x10000, 0x3d9747c9 )

	ROM_REGION(0x100000)     /* 80186 CPU */
    ROM_LOAD_EVEN("24019-01.u25", 0x040000, 0x10000, 0x9e344768 )
    ROM_LOAD_ODD ("24016-01.u13", 0x040000, 0x10000, 0x6997025f )
    ROM_LOAD_EVEN("24020-01.u26", 0x060000, 0x10000, 0x0788f2a5 )
    ROM_LOAD_ODD ("24017-01.u14", 0x060000, 0x10000, 0xa48bd721 )
    ROM_LOAD_EVEN("24021-01.u27", 0x0e0000, 0x10000, 0x94081899 )
    ROM_LOAD_ODD ("24018-01.u15", 0x0e0000, 0x10000, 0x76eb6077 )

	ROM_REGION(0x20000)     /* Background PROMS */
	/* 70, 92, 69, 91, 68, 90, 67, 89 */
    ROM_LOAD( "24007-01.u70",  0x00000, 0x4000, 0x40e46aa4 )
    ROM_LOAD( "24010-01.u92",  0x04000, 0x4000, 0x78705f42 )
    ROM_LOAD( "24006-01.u69",  0x08000, 0x4000, 0x6a576aa9 )
    ROM_LOAD( "24009-02.u91",  0x0c000, 0x4000, 0xb857a1ad )
    ROM_LOAD( "24005-02.u68",  0x10000, 0x4000, 0x8ea75319 )
    ROM_LOAD( "24008-01.u90",  0x14000, 0x4000, 0x4538bc58 )
    ROM_LOAD( "24004-02.u67",  0x18000, 0x4000, 0xcd7a3338 )
	/* 89 = empty */
ROM_END

struct GameDriver aafbu_driver =
{
	__FILE__,
	0,
	"aafbu",
	"All American Football (US Version?)",
	"????",
	LELAND,
	"Paul Leaman",
	GAME_NOT_WORKING,
	&aafbu_machine,
	0,

	aafbu_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports_aafbu,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	leland_hiload,leland_hisave
};

/***************************************************************************

  Ataxx

***************************************************************************/

extern int ataxx_vh_start(void);
extern void ataxx_vh_stop(void);
extern void ataxx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void leland_graphics_ram_w(int offset, int data);
extern unsigned char *ataxx_bk_ram;
extern unsigned char *ataxx_tram;
extern int ataxx_tram_size;
extern unsigned char *ataxx_qram1;
extern unsigned char *ataxx_qram2;

static int ataxx_palette_bank=0;    /* Palette / video RAM register bank */

INPUT_PORTS_START( input_ports_ataxx )
	PORT_START /* (0xf7) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START  /* (0xf6) */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 10, 0, 0, 255 ) /* Sensitivity, clip, min, max */

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 10, 0, 0, 255 )

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 10, 0, 0, 255 ) /* Sensitivity, clip, min, max */

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 10, 0, 0, 255 )
INPUT_PORTS_END


static struct GfxLayout ataxx_tilelayout =
{
  8,8,  /* 8 wide by 8 high */
  16*1024, /* 128k/8 characters */
  6,    /* 6 bits per pixel, each ROM holds one bit */
  { 8*0xa0000, 8*0x80000, 8*0x60000, 8*0x40000, 8*0x20000, 8*0x00000 }, /* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo ataxx_gfxdecodeinfo[] =
{
  { 1, 0x00000, &ataxx_tilelayout, 0, 64 },
  { -1 } /* end of array */
};

void ataxx_slave_cmd_w(int offset, int data)
{
	cpu_set_irq_line(1, 0, data&0x01 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_nmi_line(1,    data&0x04 ? CLEAR_LINE : HOLD_LINE);
	cpu_set_reset_line(1,  data&0x10 ? CLEAR_LINE : HOLD_LINE);
	leland_slave_cmd=data;
}

void ataxx_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    int bank=data & 0x0f;
	/* Program ROM bank */
	if (!bank)
	{
		cpu_setbank(1, &RAM[0x2000]);   /* First bank */
	}
	else
	{
		cpu_setbank(1, &RAM[0x8000*bank]);
	}

	/* Battery / QRAM bank */
	cpu_setbank(2, &RAM[0xa000]);
	if (data & 0x10)
	{
		/* Battery RAM */
		cpu_setbank(2, &leland_battery_ram[0]);
	}
	else
	{
		if (data & 0x20)
		{
			/* QRAM 1 */
			cpu_setbank(2, &ataxx_qram1[0]);
		}
		if (data & 0x40)
		{
			/* QRAM 2 */
			cpu_setbank(2, &ataxx_qram2[0]);
		}
	}

	if ((data & 0x30) == 0x30)
	{
		ataxx_palette_bank=1;   /* Palette ram write */
	}
	else
	{
		ataxx_palette_bank=0;
	}

}

void ataxx_master_video_addr_w(int offset, int data)
{
	if (ataxx_palette_bank)
	{
		/* Writing to the palette */
		paletteram_xxxxRRRRGGGGBBBB_w(offset+0x7f8, data);
	}
	else
	{
		/* Writing to video ram registers */
		leland_master_video_addr_w(offset, data);
	}
}

static struct MemoryReadAddress ataxx_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xdfff, MRA_BANK2 },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, paletteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ataxx_writemem[] =
{
	{ 0xa000, 0xdfff, MWA_BANK2 },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf7ff, MWA_RAM, &ataxx_tram, &ataxx_tram_size },
	{ 0xfff8, 0xfff9, ataxx_master_video_addr_w },
	{ 0xf800, 0xffff, paletteram_xxxxRRRRGGGGBBBB_w, &paletteram },
	{ -1 }  /* end of table */
};

void ataxx_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	leland_init_machine();

	leland_rearrange_bank_swap(0, 0x10000);
	leland_rearrange_bank_swap(1, 0x10000);

	/* Hack!!!! Patch the code to get the game to start */
	RAM[0x3593]=0;
	RAM[0x3594]=0;
}


void ataxx_sound_control_w(int offset, int data)
{
	/*
		0x01=Reset
		0x02=NMI
		0x04=Int0
		0x08=Int1
		0x10=Test
	*/

	int intnum;

	cpu_set_reset_line(2, data&0x01  ? CLEAR_LINE : HOLD_LINE);
    cpu_set_nmi_line(2,   data&0x02  ? CLEAR_LINE : HOLD_LINE);
    cpu_set_test_line(2,  data&0x10  ? CLEAR_LINE : HOLD_LINE);

	/* No idea about the int 0 and int 1 pins (do they give int number?) */
	intnum =(data&0x04)>>2;  /* Int 0 */
	intnum|=(data&0x08)>>2;  /* Int 1 */

#ifdef NOISY_SOUND_CPU
	if (errorlog)
	{
		 fprintf(errorlog, "PC=%04x Sound CPU intnum=%02x\n",
			cpu_get_pc(), intnum);
	}
#endif
}

static struct IOReadPort ataxx_readport[] =
{
	{ 0x00, 0x00, input_port_2_r },         /* Player 1 Track X */
	{ 0x00, 0x01, input_port_3_r },         /* Player 1 Track Y */
	{ 0x00, 0x02, input_port_4_r },         /* Player 2 Track X */
	{ 0x00, 0x03, input_port_5_r },         /* Player 2 Track Y */
	{ 0x04, 0x04, leland_sound_response_r },    /* Sound comms read port */
	{ 0x20, 0x20, leland_eeprom_r },         /* EEPROM */
	{ 0xd0, 0xe7, ataxx_mvram_port_r },     /* Master video RAM ports */
	{ 0xf6, 0xf6, input_port_1_r },         /* Buttons */
	{ 0xf7, 0xf7, leland_slavebit0_r },     /* Slave block (slvblk) */
	{ -1 }  /* end of table */
};

static struct IOWritePort ataxx_writeport[] =
{
	{ 0x05, 0x05, leland_sound_cmd_high_w },
	{ 0x06, 0x06, leland_sound_cmd_low_w },
	{ 0x0c, 0x0c, ataxx_sound_control_w },  /* Sound control register */
	{ 0x20, 0x20, leland_eeprom_w },         /* EEPROM */
	{ 0xd0, 0xe7, ataxx_mvram_port_w },     /* Master video RAM */
	{ 0xf0, 0xf0, leland_bk_xlow_w },       /* Probably ... always zero */
	{ 0xf1, 0xf1, leland_bk_xhigh_w },
	{ 0xf2, 0xf2, leland_bk_ylow_w },
	{ 0xf3, 0xf3, leland_bk_yhigh_w },
	{ 0xf4, 0xf4, ataxx_banksw_w },         /* Bank switch */
	{ 0xf5, 0xf5, ataxx_slave_cmd_w },      /* Slave output (slvo) */
	{ 0xf8, 0xf8, MWA_NOP },                /* Unknown */
	{ -1 }  /* end of table */
};

void ataxx_slave_banksw_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
    int bankaddress=0x10000*(data&0x0f);
	if (data&0x10)
	{
		bankaddress+=0x8000;
	}
	cpu_setbank(3, &RAM[bankaddress]);
}

static struct MemoryReadAddress ataxx_slave_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },        /* Resident program ROM */
	{ 0x2000, 0x9fff, MRA_BANK3 },      /* Paged graphics ROM */
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xfffe, 0xfffe, leland_unknown_slave_video_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ataxx_slave_writemem[] =
{
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xfffc, 0xfffd, leland_slave_video_addr_w },
	{ 0xffff, 0xffff, ataxx_slave_banksw_w },
	{ -1 }  /* end of table */
};


#if 0
	/* Ataxx uses a slightly different audio CPU (fewer DACs etc) */
				{                                                  \
						CPU_I86,         /* Sound processor */     \
						16000000,        /* 16 Mhz  */             \
						3,                                         \
						leland_i86_readmem,leland_i86_writemem,\
						leland_i86_readport,leland_i86_writeport, \
						leland_i86_interrupt,1,                    \
						0,0,&leland_addrmask                    \
				},    \

#endif

#define ATAXX_MACHINE_DRIVER(DRV, MRP, MWP, MR, MW, INITMAC, GFXD, VRF, SLR, SLW)\
static struct MachineDriver DRV =    \
{                                                      \
		{                                           \
				{                                    \
						CPU_Z80,        /* Master game processor */  \
						6000000,        /* 6.000 Mhz  */             \
						0,                                          \
						MR,MW,            \
						MRP,MWP,                                   \
						leland_master_interrupt,16                 \
				},                                                 \
				{                                                  \
						CPU_Z80, /* Slave graphics processor*/     \
						6000000, /* 6.000 Mhz */                   \
						2,       /* memory region #2 */            \
						SLR,SLW,              \
						slave_readport,slave_writeport,            \
						leland_slave_interrupt,1                   \
				},                                                 \
		},                                                         \
		60, 2000,2,                                                \
		INITMAC,                                       \
		0x28*8, 0x20*8, { 0*8, 0x28*8-1, 0*8, 0x1e*8-1 },              \
		GFXD,                                             \
		1024,1024,                                                 \
		0,                                                         \
		VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,                \
		0,                                                         \
		ataxx_vh_start,ataxx_vh_stop,VRF,    \
		0,0,0,0,\
		{   \
			{SOUND_AY8910, &ay8910_interface }, \
			{SOUND_DAC,    &dac_interface }     \
		}                 \
}

ATAXX_MACHINE_DRIVER(ataxx_machine,ataxx_readport, ataxx_writeport,
	ataxx_readmem, ataxx_writemem, ataxx_init_machine, ataxx_gfxdecodeinfo,
	ataxx_vh_screenrefresh, ataxx_slave_readmem, ataxx_slave_writemem);

ROM_START( ataxx_rom )
	ROM_REGION(0x20000)
	ROM_LOAD( "ataxx.038",   0x00000, 0x20000, 0x0e1cf6236)

	ROM_REGION_DISPOSE(0xC0000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ataxx.098",  0x00000, 0x20000, 0x059d0f2ae )
	ROM_LOAD( "ataxx.099",  0x20000, 0x20000, 0x06ab7db25 )
	ROM_LOAD( "ataxx.100",  0x40000, 0x20000, 0x02352849e )
	ROM_LOAD( "ataxx.101",  0x60000, 0x20000, 0x04c31e02b )
	ROM_LOAD( "ataxx.102",  0x80000, 0x20000, 0x0a951228c )
	ROM_LOAD( "ataxx.103",  0xa0000, 0x20000, 0x0ed326164 )

    ROM_REGION(0x100000) /* 1M for secondary cpu */
	ROM_LOAD( "ataxx.111",  0x00000, 0x20000, 0x09a3297cc )
	ROM_LOAD( "ataxx.112",  0x20000, 0x20000, 0x07e7c3e2f )
	ROM_LOAD( "ataxx.113",  0x40000, 0x20000, 0x08cf3e101 )

	ROM_REGION(0x100000) /* 1M for sound cpu */
	ROM_LOAD_EVEN( "ataxx.015",  0x80000, 0x20000, 0x08bb3233b )
	ROM_LOAD_ODD ( "ataxx.001",  0x80000, 0x20000, 0x0728d75f2 )
	ROM_LOAD_EVEN( "ataxx.016",  0xC0000, 0x20000, 0x0f2bdff48 )
	ROM_LOAD_ODD ( "ataxx.002",  0xC0000, 0x20000, 0x0ca06a394 )
ROM_END

struct GameDriver ataxx_driver =
{
	__FILE__,
	0,
	"ataxx",
	"Ataxx",
	"1990",
	LELAND,
	"Paul Leaman\nScott Kelley",
	GAME_NOT_WORKING,
	&ataxx_machine,
	0,
	ataxx_rom,
	0, 0,
	0,
	0,

	input_ports_ataxx,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	leland_hiload, leland_hisave
};

INPUT_PORTS_START( input_ports_indyheat )
	PORT_START /* (0xf7) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SLAVEHALT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START  /* (0xf6) */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 10, 0, 0, 255 ) /* Sensitivity, clip, min, max */

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 10, 0, 0, 255 )

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 10, 0, 0, 255 ) /* Sensitivity, clip, min, max */

	PORT_START
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 10, 0, 0, 255 )

    PORT_START /* (0xff) */
    PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_START /* (0xff) */
    PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_START /* (0xff) */
    PORT_BITX(0xf0, IP_ACTIVE_LOW, IPT_SERVICE, "Service", KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

INPUT_PORTS_END


void indyheat_init_machine(void)
{
	leland_init_machine();

	leland_rearrange_bank_swap(0, 0x10000);
	leland_rearrange_bank_swap(1, 0x10000);
}

static struct IOReadPort indyheat_readport[] =
{
	{ 0x00, 0x00, input_port_2_r },         /* Player 1 Track X */
	{ 0x00, 0x01, input_port_3_r },         /* Player 1 Track Y */
	{ 0x00, 0x02, input_port_4_r },         /* Player 2 Track X */
	{ 0x00, 0x03, input_port_5_r },         /* Player 2 Track Y */
	{ 0x04, 0x04, leland_sound_response_r },    /* Sound comms read port */
    { 0x0d, 0x0d, input_port_6_r },
    { 0x0e, 0x0e, input_port_7_r },
    { 0x0f, 0x0f, input_port_8_r },

	{ 0x20, 0x20, leland_eeprom_r },         /* EEPROM */
	{ 0xd0, 0xe7, ataxx_mvram_port_r },     /* Master video RAM ports */
	{ 0xf6, 0xf6, input_port_1_r },         /* Buttons */
	{ 0xf7, 0xf7, leland_slavebit0_r },     /* Slave block (slvblk) */
	{ -1 }  /* end of table */
};

static struct IOWritePort indyheat_writeport[] =
{
	{ 0x05, 0x05, leland_sound_cmd_high_w },
	{ 0x06, 0x06, leland_sound_cmd_low_w },
	{ 0x0c, 0x0c, ataxx_sound_control_w },  /* Sound control register */
	{ 0x20, 0x20, leland_eeprom_w },         /* EEPROM */
	{ 0xd0, 0xe7, ataxx_mvram_port_w },     /* Master video RAM */
	{ 0xf0, 0xf0, leland_bk_xlow_w },       /* Probably ... always zero */
	{ 0xf1, 0xf1, leland_bk_xhigh_w },
	{ 0xf2, 0xf2, leland_bk_ylow_w },
	{ 0xf3, 0xf3, leland_bk_yhigh_w },
	{ 0xf4, 0xf4, ataxx_banksw_w },         /* Bank switch */
	{ 0xf5, 0xf5, ataxx_slave_cmd_w },      /* Slave output (slvo) */
	{ 0xf8, 0xf8, MWA_NOP },                /* Unknown */
	{ -1 }  /* end of table */
};

ATAXX_MACHINE_DRIVER(indyheat_machine,indyheat_readport, indyheat_writeport,
    ataxx_readmem, ataxx_writemem, indyheat_init_machine, ataxx_gfxdecodeinfo,
	ataxx_vh_screenrefresh, ataxx_slave_readmem, ataxx_slave_writemem);

ROM_START( indyheat_rom )
    ROM_REGION(0x100000)
    ROM_LOAD( "u64_27c.010",   0x00000, 0x20000, 0x2b97a347)
    ROM_LOAD( "u65_27c.010",   0x20000, 0x20000, 0x71301d74)
    ROM_LOAD( "u66_27c.010",   0x40000, 0x20000, 0xc9612072)
    ROM_LOAD( "u67_27c.010",   0x60000, 0x20000, 0x4c4b25e0)
    ROM_LOAD( "u68_27c.010",   0x80000, 0x20000, 0x9e88efb3)
    ROM_LOAD( "u69_27c.010",   0xa0000, 0x20000, 0xaa39fcb3)

	ROM_REGION_DISPOSE(0xC0000)  /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "u145_27c.010",  0x00000, 0x20000, 0x612d4bf8 )
    ROM_LOAD( "u146_27c.010",  0x20000, 0x20000, 0x77a725f6 )
    ROM_LOAD( "u147_27c.010",  0x40000, 0x20000, 0xd6aac372 )
    ROM_LOAD( "u148_27c.010",  0x60000, 0x20000, 0x5d19723e )
    ROM_LOAD( "u149_27c.010",  0x80000, 0x20000, 0x29056791 )
    ROM_LOAD( "u150_27c.010",  0xa0000, 0x20000, 0xcb73dd6a )

    ROM_REGION(0x100000) /* 1M for secondary cpu */
    ROM_LOAD( "u151_27c.010",  0x00000, 0x20000, 0x2622dfa4 )
    ROM_LOAD( "u152_27c.010",  0x20000, 0x20000, 0xd2034826 )
    ROM_LOAD( "u153_27c.010",  0x40000, 0x20000, 0x36ce207a )
    ROM_LOAD( "u154_27c.010",  0x60000, 0x20000, 0x76d3c235 )
    ROM_LOAD( "u155_27c.010",  0x80000, 0x20000, 0xd5d866b3 )
    ROM_LOAD( "u156_27c.010",  0xa0000, 0x20000, 0x7fe71842 )
    ROM_LOAD( "u157_27c.010",  0xc0000, 0x20000, 0xa6462adc )
    ROM_LOAD( "u158_27c.010",  0xe0000, 0x20000, 0xd6ef27a3 )

    ROM_REGION(0x100000) /* 1M for sound cpu */
    ROM_LOAD_EVEN( "u6_27c.010",  0x40000, 0x20000, 0x15a89962 )
    ROM_LOAD_ODD ( "u3_27c.010",  0x40000, 0x20000, 0x97413818 )
    ROM_LOAD_WIDE( "u8_27c.010",  0x80000, 0x20000, 0x9f16e5b6 )
    ROM_LOAD_WIDE( "u9_27c.010",  0xa0000, 0x20000, 0x0dc8f488 )
    ROM_LOAD_EVEN( "u4_27c.010",  0xC0000, 0x20000, 0xfa7bfa04 )
    ROM_LOAD_ODD ( "u5_27c.010",  0xC0000, 0x20000, 0x198285d4 )
ROM_END

struct GameDriver indyheat_driver =
{
	__FILE__,
	0,
    "indyheat",
    "Indy Heat",
	"1990",
	LELAND,
    "Paul Leaman",
	GAME_NOT_WORKING,
    &indyheat_machine,
	0,
    indyheat_rom,
	0, 0,
	0,
	0,

    input_ports_indyheat,
    0, 0, 0,
	ORIENTATION_DEFAULT,
	leland_hiload, leland_hisave
};

ATAXX_MACHINE_DRIVER(wsf_machine,ataxx_readport, ataxx_writeport,
    ataxx_readmem, ataxx_writemem, indyheat_init_machine, ataxx_gfxdecodeinfo,
	ataxx_vh_screenrefresh, ataxx_slave_readmem, ataxx_slave_writemem);


ROM_START( wsf_rom )
    ROM_REGION(0x100000)
    ROM_LOAD( "30022-03.u64",   0x00000, 0x20000, 0x2e7faa96)
    ROM_LOAD( "30023-03.u65",   0x20000, 0x20000, 0x7146328f)
    /* Unknown program ROM loading */
    ROM_LOAD( "30009-01.u68",   0x80000, 0x10000, 0xf2fbfc15)
    ROM_LOAD( "30010-01.u69",   0xa0000, 0x10000, 0xb4ed2d3b)

    ROM_REGION_DISPOSE(0x100000)  /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "30011-02.145",  0x00000, 0x10000, 0x6153569b )
    ROM_LOAD( "30012-02.146",  0x20000, 0x10000, 0x52d65e21 )
    ROM_LOAD( "30013-02.147",  0x40000, 0x10000, 0xb3afda12 )
    ROM_LOAD( "30014-02.148",  0x60000, 0x10000, 0x624e6c64 )
    ROM_LOAD( "30015-01.149",  0x80000, 0x10000, 0x5d9064f2 )
    ROM_LOAD( "30016-01.150",  0xa0000, 0x10000, 0xd76389cd )

    ROM_REGION(0x100000) /* 1M for secondary cpu */
    ROM_LOAD( "30001-01.151",  0x00000, 0x20000, 0x31c63af5 )
    ROM_LOAD( "30002-01.152",  0x20000, 0x20000, 0xa53e88a6 )
    ROM_LOAD( "30003-01.153",  0x40000, 0x20000, 0x12afad1d )
    ROM_LOAD( "30004-01.154",  0x60000, 0x20000, 0xb8b3d59c )
    ROM_LOAD( "30005-01.155",  0x80000, 0x20000, 0x505724b9 )
    ROM_LOAD( "30006-01.156",  0xa0000, 0x20000, 0xc86b5c4d )
    ROM_LOAD( "30007-01.157",  0xc0000, 0x20000, 0x451321ae )
    ROM_LOAD( "30008-01.158",  0xe0000, 0x20000, 0x4d23836f )

    ROM_REGION(0x100000) /* 1M for sound cpu */
    ROM_LOAD_EVEN( "30020-01.u6",  0x40000, 0x20000, 0x031a06d7 )
    ROM_LOAD_ODD ( "30017-01.u3",  0x40000, 0x20000, 0x39ec13c1 )
    ROM_LOAD_WIDE( "30021-01.u8",  0x80000, 0x20000, 0xbb91dc10 )
    /* U9 = empty ? */
    ROM_LOAD_EVEN( "30018-01.u4",  0xC0000, 0x20000, 0x1ec16735 )
    ROM_LOAD_ODD ( "30019-01.u5",  0xC0000, 0x20000, 0x2881f73b )
ROM_END

struct GameDriver wsf_driver =
{
	__FILE__,
	0,
    "wsf",
    "World Soccer Finals",
	"1990",
	LELAND,
    "Paul Leaman",
	GAME_NOT_WORKING,
    &wsf_machine,
	0,
    wsf_rom,
	0, 0,
	0,
	0,

    input_ports_indyheat,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	leland_hiload, leland_hisave
};

