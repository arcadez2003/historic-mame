/***************************************************************************

Bomb Jack memory map (preliminary)
MAIN BOARD:

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-83ff RAM 0
8400-87ff RAM 1
8800-8bff RAM 2
8c00-8fff RAM 3
9000-93ff Video RAM (RAM 4)
9400-97ff Color RAM (RAM 4)
9c00-9cff Palette RAM
c000-dfff ROM 4

memory mapped ports:
read:
b000      IN0
b001      IN1
b002      IN2
b003      watchdog reset?
b004      DSW1
b005      DSW2

write:
9820-987f sprites
9a00      ? number of small sprites for video controller
9e00      background image selector
b000      interrupt enable
b004      flip screen
b800      command to soundboard & trigger NMI on sound board



SOUND BOARD:
0x0000 0x1fff ROM
0x2000 0x43ff RAM

memory mapped ports:
read:
0x6000 command from soundboard
write :
none

IO ports:
write:
0x00 AY#1 control
0x01 AY#1 write
0x10 AY#2 control
0x11 AY#2 write
0x80 AY#3 control
0x81 AY#3 write

interrupts:
NMI triggered by the commands sent by MAIN BOARD (?)
NMI interrupts for music timing

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void bombjack_background_w(int offset,int data);
void bombjack_flipscreen_w(int offset,int data);
void bombjack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static int latch;

static void soundlatch_callback(int param)
{
	latch = param;
}

void bombjack_soundlatch_w(int offset,int data)
{
	/* make all the CPUs synchronize, and only AFTER that write the new command to the latch */
	timer_set(TIME_NOW,data,soundlatch_callback);
}

int bombjack_soundlatch_r(int offset)
{
	int res;


	res = latch;
	latch = 0;
	return res;
}

unsigned char *bombjack_sh_intflag;

int bombjack_sh_intflag_r(int offset)
{
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for an interrupt, and force it in that case */
	if (cpu_getpc() == 0x0099 && (*bombjack_sh_intflag & 1) == 0)
		cpu_spinuntil_int();

	return *bombjack_sh_intflag;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },	/* including video and color RAM */
	{ 0xb000, 0xb000, input_port_0_r },	/* player 1 input */
	{ 0xb001, 0xb001, input_port_1_r },	/* player 2 input */
	{ 0xb002, 0xb002, input_port_2_r },	/* coin */
	{ 0xb003, 0xb003, MRA_NOP },	/* watchdog reset? */
	{ 0xb004, 0xb004, input_port_3_r },	/* DSW1 */
	{ 0xb005, 0xb005, input_port_4_r },	/* DSW2 */
	{ 0xc000, 0xdfff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x9820, 0x987f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9a00, 0x9a00, MWA_NOP },
	{ 0x9c00, 0x9cff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0x9e00, 0x9e00, bombjack_background_w },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb004, 0xb004, bombjack_flipscreen_w },
	{ 0xb800, 0xb800, bombjack_soundlatch_w },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bombjack_sound_readmem[] =
{
	{ 0x4390, 0x4390, bombjack_sh_intflag_r, &bombjack_sh_intflag },	/* kludge to speed up the emulation */
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, bombjack_soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bombjack_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }  /* end of table */
};


static struct IOWritePort bombjack_sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x11, 0x11, AY8910_write_port_1_w },
	{ 0x80, 0x80, AY8910_control_port_2_w },
	{ 0x81, 0x81, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, "Initial High Score?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x01, "100000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x04, "100000" )
	PORT_DIPSETTING(    0x05, "50000" )
	PORT_DIPSETTING(    0x06, "100000" )
	PORT_DIPSETTING(    0x07, "50000" )
	PORT_DIPNAME( 0x18, 0x00, "Bird Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x18, "Hardest" )
	PORT_DIPNAME( 0x60, 0x00, "Enemies Number & Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x60, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Special Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x80, "Hard" )
INPUT_PORTS_END



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 2*512*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	32,	/* 32 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			80*8, 81*8, 82*8, 83*8, 84*8, 85*8, 86*8, 87*8 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,      0, 16 },	/* characters */
	{ 1, 0x3000, &charlayout2,      0, 16 },	/* background tiles */
	{ 1, 0x9000, &spritelayout1,    0, 16 },	/* normal sprites */
	{ 1, 0xa000, &spritelayout2,    0, 16 },	/* large sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 40, 40, 40 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz????? */
			3,	/* memory region #3 */
			bombjack_sound_readmem,bombjack_sound_writemem,0,bombjack_sound_writeport,
			nmi_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	128, 128,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	bombjack_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bombjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "09_j01b.bin",  0x0000, 0x2000, 0xc668dc30 )
	ROM_LOAD( "10_l01b.bin",  0x2000, 0x2000, 0x52a1e5fb )
	ROM_LOAD( "11_m01b.bin",  0x4000, 0x2000, 0xb68a062a )
	ROM_LOAD( "12_n01b.bin",  0x6000, 0x2000, 0x1d3ecee5 )
	ROM_LOAD( "13_r01b.bin",  0xc000, 0x2000, 0xbcafdd29 )

	ROM_REGION_DISPOSE(0xf000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "03_e08t.bin",  0x0000, 0x1000, 0x9f0470d5 )	/* chars */
	ROM_LOAD( "04_h08t.bin",  0x1000, 0x1000, 0x81ec12e6 )
	ROM_LOAD( "05_k08t.bin",  0x2000, 0x1000, 0xe87ec8b1 )
	ROM_LOAD( "06_l08t.bin",  0x3000, 0x2000, 0x51eebd89 )	/* background tiles */
	ROM_LOAD( "07_n08t.bin",  0x5000, 0x2000, 0x9dd98e9d )
	ROM_LOAD( "08_r08t.bin",  0x7000, 0x2000, 0x3155ee7d )
	ROM_LOAD( "16_m07b.bin",  0x9000, 0x2000, 0x94694097 )	/* sprites */
	ROM_LOAD( "15_l07b.bin",  0xb000, 0x2000, 0x013f58f2 )
	ROM_LOAD( "14_j07b.bin",  0xd000, 0x2000, 0x101c858d )

	ROM_REGION(0x1000)	/* background graphics */
	ROM_LOAD( "02_p04t.bin",  0x0000, 0x1000, 0x398d4a02 )

	ROM_REGION(0x10000)	/* 64k for sound board */
	ROM_LOAD( "01_h03t.bin",  0x0000, 0x2000, 0x8407917d )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8100],&RAM[0x8124],4) == 0 &&
			memcmp(&RAM[0x8100],&RAM[0x80e2],4) == 0 &&	/* high score */
			(memcmp(&RAM[0x8100],"\x00\x00\x01\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x03\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x05\x00",4) == 0 ||
			memcmp(&RAM[0x8100],"\x00\x00\x10\x00",4) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			char buf[10];
			int hi;


			osd_fread(f,&RAM[0x8100],15*10);
			RAM[0x80e2] = RAM[0x8100];
			RAM[0x80e3] = RAM[0x8101];
			RAM[0x80e4] = RAM[0x8102];
			RAM[0x80e5] = RAM[0x8103];
			/* also copy the high score to the screen, otherwise it won't be */
			/* updated until a new game is started */
			hi = (RAM[0x8100] & 0x0f) +
					(RAM[0x8100] >> 4) * 10 +
					(RAM[0x8101] & 0x0f) * 100 +
					(RAM[0x8101] >> 4) * 1000 +
					(RAM[0x8102] & 0x0f) * 10000 +
					(RAM[0x8102] >> 4) * 100000 +
					(RAM[0x8103] & 0x0f) * 1000000 +
					(RAM[0x8103] >> 4) * 10000000;
			sprintf(buf,"%8d",hi);
			videoram_w(0x013f,buf[0]);
			videoram_w(0x011f,buf[1]);
			videoram_w(0x00ff,buf[2]);
			videoram_w(0x00df,buf[3]);
			videoram_w(0x00bf,buf[4]);
			videoram_w(0x009f,buf[5]);
			videoram_w(0x007f,buf[6]);
			videoram_w(0x005f,buf[7]);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8100],15*10);
		osd_fclose(f);
	}
}



struct GameDriver bombjack_driver =
{
	__FILE__,
	0,
	"bombjack",
	"Bomb Jack",
	"1984",
	"Tehkan",
	"Brad Thomas (hardware info)\nJakob Frendsen (hardware info)\nConny Melin (hardware info)\nMirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nJarek Burczynski (sound)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	bombjack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
