/*
Double Dragon, Double Dragon (bootleg) & Double Dragon II

By Carlos A. Lozano & Rob Rosenbrock
Help to do the original drivers from Chris Moore
Sprite CPU support and additional code by Phil Stroffolino
Sprite CPU emulation, vblank support, and partial sound code by Ernesto Corvi.
Dipswitch to dd2 by Marco Cassili.

TODO:
- Add High Score support
- Find the original MCU code so original Double Dragon ROMs can be supported

NOTES:
The OKI M5205 chip 0 sampling rate is 8000hz (8khz).
The OKI M5205 chip 1 sampling rate is 4000hz (4khz).
Until the ADPCM interface is updated to be able to use
multiple sampling rates, all samples currently play at 8khz.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6809/m6809.h"

/* from vidhrdw */
extern unsigned char *dd_videoram;
extern unsigned char *dd_palette_ram;
extern int dd_scrollx_hi, dd_scrolly_hi;
extern unsigned char *dd_scrollx_lo;
extern unsigned char *dd_scrolly_lo;
extern int dd_vh_start(void);
extern void dd_vh_stop(void);
extern void dd_vh_screenrefresh(struct osd_bitmap *bitmap);
extern void dd_background_w( int offset, int val );
extern void dd_palette_w( int offset, int val );
extern unsigned char *dd_spriteram;
extern int dd2_video;
/* end of extern code & data */

/* private globals */
static int dd_sub_cpu_busy;
static int sprite_irq, sound_irq, ym_irq;
/* end of private globals */

static void dd_init_machine( void ) {
	sprite_irq = M6809_INT_NMI;
	sound_irq = M6809_INT_IRQ;
	ym_irq = M6809_INT_FIRQ;
	dd2_video = 0;
	dd_sub_cpu_busy = 0x10;
}

static void dd2_init_machine( void ) {
	sprite_irq = Z80_NMI_INT;
	sound_irq = Z80_NMI_INT;
	ym_irq = -1000;
	dd2_video = 1;
	dd_sub_cpu_busy = 0x10;
}

static void dd_bankswitch_w( int offset, int data ) {

	dd_scrolly_hi = ( ( data & 0x02 ) << 7 );
	dd_scrollx_hi = ( ( data & 0x01 ) << 8 );

	if ( ( data & 0x10 ) == 0x10 ) {
		dd_sub_cpu_busy = 0x00;
	} else if ( dd_sub_cpu_busy == 0x00 )
			cpu_cause_interrupt( 1, sprite_irq );

	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data >> 5 ) & 7 ) ) ] );
}

static void dd_forcedIRQ_w( int offset, int data ) {
	cpu_cause_interrupt( 0, M6809_INT_IRQ );
}

static int port4_r( int offset ) {
	int port = readinputport( 4 );

	return port | dd_sub_cpu_busy;
}

static int dd_spriteram_r( int offset ){
	return dd_spriteram[offset];
}

static void dd_spriteram_w( int offset, int data ) {

	if ( cpu_getactivecpu() == 1 && offset == 0 )
		dd_sub_cpu_busy = 0x10;

	dd_spriteram[offset] = data;
}

static void cpu_sound_command_w( int offset, int data ) {
	soundlatch_w( offset, data );
	cpu_cause_interrupt( 2, sound_irq );
}

static void dd_adpcm_w( int offset, int data ) {
	static int cur_sample[2];
	int chip = offset & 1;

	offset >>= 1;

	if ( offset == 1 )
		cur_sample[chip] = data;

	if ( offset == 0 )
		ADPCM_trigger( chip, ( cur_sample[chip] ) + ( chip << 8 ) );
}

static int dd_adpcm_status_r( int offset ) {
	return ( ADPCM_playing( 0 ) + ( ADPCM_playing( 1 ) << 1 ) );
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, dd_spriteram_r, &dd_spriteram },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x3805, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, dd_palette_w, &dd_palette_ram },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, MWA_RAM, &videoram },
	{ 0x2000, 0x2fff, dd_spriteram_w },
	{ 0x3000, 0x37ff, dd_background_w, &dd_videoram },
	{ 0x3800, 0x3807, MWA_RAM },
	{ 0x3808, 0x3808, dd_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &dd_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &dd_scrolly_lo },
	{ 0x380b, 0x380b, MWA_RAM },
	{ 0x380c, 0x380d, MWA_RAM },
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, dd_forcedIRQ_w },
	{ 0x3810, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, MWA_RAM, &videoram },
	{ 0x2000, 0x2fff, dd_spriteram_w },
	{ 0x3000, 0x37ff, dd_background_w, &dd_videoram },
	{ 0x3800, 0x3807, MWA_RAM },
	{ 0x3808, 0x3808, dd_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &dd_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &dd_scrolly_lo },
	{ 0x380b, 0x380b, MWA_RAM },
	{ 0x380c, 0x380d, MWA_RAM },
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, dd_forcedIRQ_w },
	{ 0x3810, 0x3bff, MWA_RAM },
	{ 0x3c00, 0x3fff, dd_palette_w, &dd_palette_ram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sub_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x8000, 0x8fff, dd_spriteram_r },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sub_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x8000, 0x8fff, dd_spriteram_w },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x1800, 0x1800, dd_adpcm_status_r },
	{ 0x2800, 0x2801, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM2151_register_port_0_w },
	{ 0x2801, 0x2801, YM2151_data_port_0_w },
	{ 0x3800, 0x3807, dd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dd2_sub_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, dd_spriteram_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_sub_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, dd_spriteram_w },
	{ 0xd000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dd2_sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_r },
	{ 0xA000, 0xA000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_w },
	{ -1 }	/* end of table */
};

/* bit 0x10 is sprite CPU busy signal */
#define COMMON_PORT4	PORT_START \
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) \
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK ) \
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define COMMON_INPUT_PORTS PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_START \
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x01, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x02, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x07, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(	0x06, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(	0x05, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(	0x04, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(	0x03, "1 Coin/5 Credits" ) \
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x08, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x10, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x38, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(	0x30, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(	0x28, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(	0x20, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(	0x18, "1 Coin/5 Credits" ) \
	PORT_DIPNAME( 0x40, 0x40, "Screen Orientation", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "On" ) \
	PORT_DIPSETTING(	0x40, "Off") \
	PORT_DIPNAME( 0x80, 0x80, "Screen Reverse", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "On" ) \
	PORT_DIPSETTING(	0x80, "Off")

INPUT_PORTS_START( dd1_input_ports )
	COMMON_INPUT_PORTS

    PORT_START      /* DSW1 */
    PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Easy")
    PORT_DIPSETTING(    0x03, "Normal")
    PORT_DIPSETTING(    0x01, "Hard")
    PORT_DIPSETTING(    0x00, "Very Hard")
    PORT_DIPNAME( 0x04, 0x04, "Attract Mode Sound", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x04, "On")
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "20K")
    PORT_DIPSETTING(    0x00, "40K" )
    PORT_DIPSETTING(    0x30, "30K and every 60K")
    PORT_DIPSETTING(    0x20, "40K and every 80K" )
    PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "2")
    PORT_DIPSETTING(    0x80, "3" )
    PORT_DIPSETTING(    0x40, "4")
    PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )

    COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( dd2_input_ports )
	COMMON_INPUT_PORTS

  PORT_START      /* DSW1 */
    PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Easy")
    PORT_DIPSETTING(    0x03, "Normal")
    PORT_DIPSETTING(    0x01, "Medium")
    PORT_DIPSETTING(    0x00, "Hard")
    PORT_DIPNAME( 0x04, 0x04, "Attract Mode Sound", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x04, "On")
    PORT_DIPNAME( 0x08, 0x08, "Hurricane Kick", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Easy" )
    PORT_DIPSETTING(    0x08, "Normal")
    PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
    PORT_DIPSETTING(    0x20, "80" )
    PORT_DIPSETTING(    0x30, "70")
    PORT_DIPSETTING(    0x10, "65")
    PORT_DIPSETTING(    0x00, "60" )
    PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "1")
    PORT_DIPSETTING(    0x80, "2" )
    PORT_DIPSETTING(    0x40, "3")
    PORT_DIPSETTING(    0x00, "4")

	COMMON_PORT4
INPUT_PORTS_END

#undef COMMON_INPUT_PORTS
#undef COMMON_PORT4

#define CHAR_LAYOUT( name, num ) \
	static struct GfxLayout name = \
	{ \
		8,8, /* 8*8 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ 0, 2, 4, 6 }, /* plane offset */ \
		{ 1, 0, 65, 64, 129, 128, 193, 192 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },	\
		32*8 /* every char takes 32 consecutive bytes */ \
	};

#define TILE_LAYOUT( name, num, planeoffset ) \
	static struct GfxLayout name = \
	{ \
		16,16, /* 16x16 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ planeoffset*8+0, planeoffset*8+4, 0,4 }, /* plane offset */ \
		{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0, \
	          32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, \
	          8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 }, \
		64*8 /* every char takes 64 consecutive bytes */ \
	};

CHAR_LAYOUT( char_layout, 1024 ) /* foreground chars */
TILE_LAYOUT( tile_layout, 2048, 0x20000 ) /* background tiles */
TILE_LAYOUT( sprite_layout, 2048*2, 0x40000 ) /* sprites */

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0xc0000, &char_layout,	256, 8 },	/* 8x8 text */
	{ 1, 0x40000, &sprite_layout,	128, 8 },	/* 16x16 sprites */
	{ 1, 0x00000, &tile_layout,	0, 8 },		/* 16x16 background tiles */
	{ -1 }
};

CHAR_LAYOUT( dd2_char_layout, 2048 ) /* foreground chars */
TILE_LAYOUT( dd2_sprite_layout, 2048*3, 0x60000 ) /* sprites */

/* background tiles encoding for dd2 is the same as dd1 */

static struct GfxDecodeInfo dd2_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &dd2_char_layout,	256, 8 },	/* 8x8 chars */
	{ 1, 0x50000, &dd2_sprite_layout,128, 8 },	/* 16x16 sprites */
	{ 1, 0x10000, &tile_layout,	0, 8 },         /* 16x16 background tiles */
	{ -1 } // end of array
};

static void dd_irq_handler(void) {
	cpu_cause_interrupt( 2, ym_irq );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3582071,	/* seems to be the standard */
	{ 255 },
	{ dd_irq_handler }
};

static struct ADPCMinterface adpcm_interface =
{
	2,			/* 2 channels */
	8000,       /* 8000Hz playback */
	4,			/* memory region 4 */
	0,			/* init function */
	{ 255, 255 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* frequency (Hz) */
	4,              /* memory region */
	{ 128 }
};

static int dd_interrupt(void)
{
	return ( M6809_INT_FIRQ | M6809_INT_NMI );
}

static struct MachineDriver ddragonb_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,writemem,0,0,
			dd_interrupt,1
		},
		{
 			CPU_M6809,
			12000000 / 3, /* 4 Mhz */
			2,
			sub_readmem,sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0 /* irq on command */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 3*8*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver ddragon_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,writemem,0,0,
			dd_interrupt,1
		},
		{
 			CPU_HD63701, /* we're missing the code for this one */
			12000000 / 3, /* 4 Mhz */
			2,
			sub_readmem,sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0 /* irq on command */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 3*8*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver ddragon2_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,dd2_writemem,0,0,
			dd_interrupt,1
		},
		{
			CPU_Z80,
			12000000 / 3, /* 4 Mhz */
			2,	/* memory region */
			dd2_sub_readmem,dd2_sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,	/* memory region */
			dd2_sound_readmem,dd2_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd2_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	dd2_gfxdecodeinfo,
	256,3*8*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_16BIT,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( dd_rom )
	ROM_REGION(0x28000)	/* 64k for code + bankswitched memory */
	ROM_LOAD( "IC26",  0x08000, 0x08000, 0x113d6391 )
	ROM_LOAD( "IC25",  0x10000, 0x08000, 0xd40db89b ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "IC24",  0x18000, 0x08000, 0x1a208c10 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "IC23",  0x20000, 0x08000, 0xe9e164b9 ) /* banked at 0x4000-0x8000 */

	ROM_REGION(0xc8000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC20",  0xc0000, 0x08000, 0x7146494c ) /* 0,1,2,3 */ /* text */
	ROM_LOAD( "IC80",  0x00000, 0x10000, 0x10c0e6d0 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "IC79",  0x10000, 0x10000, 0x9a4cc484 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "IC115", 0x20000, 0x10000, 0x51b52415 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "IC114", 0x30000, 0x10000, 0xf1e7d881 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "IC123", 0x40000, 0x10000, 0x172eb21c ) /* 0,1 */ /* sprites */
	ROM_LOAD( "IC122", 0x50000, 0x10000, 0xee6eb2d6 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "IC121", 0x60000, 0x10000, 0xe98e2e82 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "IC120", 0x70000, 0x10000, 0x526d686d ) /* 0,1 */ /* sprites */
	ROM_LOAD( "IC119", 0x80000, 0x10000, 0x9ae98b73 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "IC118", 0x90000, 0x10000, 0x7a6c1946 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "IC117", 0xa0000, 0x10000, 0xfd29a219 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "IC116", 0xb0000, 0x10000, 0xc5d467e4 ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* sprite cpu */
	ROM_LOAD( "IC38",  0x0c000, 0x04000, 0x7f95ec95 )

	ROM_REGION(0x10000) /* audio cpu */
	ROM_LOAD( "IC30",  0x08000, 0x08000, 0xad00a730 )

	ROM_REGION(0x20000) /* adpcm samples */
	ROM_LOAD( "IC99",  0x00000, 0x10000, 0x27a46af4 )
	ROM_LOAD( "IC98",  0x10000, 0x10000, 0x105e0bba )
ROM_END

ROM_START( ddragon_rom )
	ROM_REGION(0x28000)	/* 64k for code + bankswitched memory */
	ROM_LOAD( "A_M2_D02.BIN", 0x08000, 0x08000, 0x62e30fe9 )
	ROM_LOAD( "A_K2_D03.BIN", 0x10000, 0x08000, 0xd40db89b )
	ROM_LOAD( "A_H2_D04.BIN", 0x18000, 0x08000, 0x8aa2463c )
	ROM_LOAD( "A_G2_D05.BIN", 0x20000, 0x08000, 0x20935a8d )

	ROM_REGION(0xC8000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "A_A2_D06.BIN", 0xC0000, 0x08000 , 0x7146494c ) /* 0,1,2,3 */ /* text */
	ROM_LOAD( "B_C5_D09.BIN", 0x00000, 0x10000, 0x10c0e6d0 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "B_A5_D10.BIN", 0x10000, 0x10000, 0x9a4cc484 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "B_C7_D19.BIN", 0x20000, 0x10000, 0x51b52415 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "B_A7_D20.BIN", 0x30000, 0x10000, 0xf1e7d881 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "B_R7_D11.BIN", 0x40000, 0x10000, 0x172eb21c ) /* 0,1 */ /* sprites */
	ROM_LOAD( "B_P7_D12.BIN", 0x50000, 0x10000, 0xee6eb2d6 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "B_M7_D13.BIN", 0x60000, 0x10000, 0xe98e2e82 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "B_L7_D14.BIN", 0x70000, 0x10000, 0x526d686d ) /* 0,1 */ /* sprites */
	ROM_LOAD( "B_J7_D15.BIN", 0x80000, 0x10000, 0x9ae98b73 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "B_H7_D16.BIN", 0x90000, 0x10000, 0x7a6c1946 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "B_F7_D17.BIN", 0xA0000, 0x10000, 0xfd29a219 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "B_D7_D18.BIN", 0xB0000, 0x10000, 0xc5d467e4 ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* sprite cpu */
	/* missing mcu code */
	/* currently load the audio cpu code in this location */
	/* because otherwise mame will loop indefinately in cpu_run */
	ROM_LOAD( "A_S2_D01.BIN",  0x08000, 0x08000, 0xad00a730 )

	ROM_REGION(0x10000) /* audio cpu */
	ROM_LOAD( "A_S2_D01.BIN",  0x08000, 0x08000, 0xad00a730 )

	ROM_REGION(0x20000) /* adpcm samples */
	ROM_LOAD( "A_S6_D07.BIN",  0x00000, 0x10000, 0x27a46af4 )
	ROM_LOAD( "A_R6_D08.BIN",  0x10000, 0x10000, 0x105e0bba )
ROM_END

ROM_START( ddragon2_rom )
	ROM_REGION(0x28000)	/* region#0: 64k for code */
	ROM_LOAD( "26A9-04.BIN", 0x08000, 0x8000, 0x718a0bcc )
	ROM_LOAD( "26AA-03.BIN", 0x10000, 0x8000, 0x87c72a6f )
	ROM_LOAD(  "26AB-0.BIN", 0x18000, 0x8000, 0x40acba00 )
	ROM_LOAD( "26AC-02.BIN", 0x20000, 0x8000, 0xf4dc516e )

	ROM_REGION(0x110000) /* region#1: graphics (disposed after conversion) */
	ROM_LOAD( "26A8-0.BIN", 0x00000, 0x10000, 0x1d59c0d9 ) /* 0,1,2,3 */ /* text */
    ROM_LOAD( "26J4-0.BIN", 0x10000, 0x20000, 0x67e512c9 ) /* 0,1 */ /* tiles */
    ROM_LOAD( "26J5-0.BIN", 0x30000, 0x20000, 0x9ad37fcb ) /* 2,3 */ /* tiles */
    ROM_LOAD( "26J0-0.BIN", 0x50000, 0x20000, 0xf8e039a4 ) /* 0,1 */ /* sprites */
    ROM_LOAD( "26J1-0.BIN", 0x70000, 0x20000, 0x22b6c66c ) /* 0,1 */ /* sprites */
	ROM_LOAD( "26AF-0.BIN", 0x90000, 0x20000, 0x30092467 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "26J2-0.BIN", 0xb0000, 0x20000, 0xaa721e6a ) /* 2,3 */ /* sprites */
	ROM_LOAD( "26J3-0.BIN", 0xd0000, 0x20000, 0x66e0b442 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "26A10-0.BIN",0xf0000, 0x20000, 0x00ccaeb2 ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* region#2: sprite CPU 64kb (Upper 16kb = 0) */
    ROM_LOAD( "26AE-0.BIN", 0x00000, 0x10000, 0xe36202e8 )

	ROM_REGION(0x10000) /* region#3: music CPU, 64kb */
    ROM_LOAD("26AD-0.BIN", 0x00000, 0x8000, 0xd353aa0b )

	ROM_REGION(0x40000) /* region#4: adpcm */
    ROM_LOAD("26J6-0.BIN", 0x00000, 0x20000, 0xd95b0e0d )
    ROM_LOAD("26J7-0.BIN", 0x20000, 0x20000, 0x2e49c0f9 )
ROM_END

/*************************/
/* Double Dragon Samples */
/*************************/
ADPCM_SAMPLES_START( dd_samples )
	/* samples on ROM 1 */
	ADPCM_SAMPLE( 0x000, 0x0000, (0x0800-0x0000)*2 )
	ADPCM_SAMPLE( 0x004, 0x0800, (0x1300-0x0800)*2 )
	ADPCM_SAMPLE( 0x009, 0x1300, (0x1f00-0x1300)*2 )
	ADPCM_SAMPLE( 0x00f, 0x1f00, (0x3300-0x1f00)*2 )
	ADPCM_SAMPLE( 0x019, 0x3300, (0x4d00-0x3300)*2 )
	ADPCM_SAMPLE( 0x026, 0x4d00, (0x6900-0x4d00)*2 )
	ADPCM_SAMPLE( 0x034, 0x6900, (0x7500-0x6900)*2 )
	ADPCM_SAMPLE( 0x03a, 0x7500, (0x8600-0x7500)*2 )
	ADPCM_SAMPLE( 0x043, 0x8600, (0x9300-0x8600)*2 )
	ADPCM_SAMPLE( 0x049, 0x9300, (0x9d00-0x9300)*2 )
	ADPCM_SAMPLE( 0x04e, 0x9d00, (0xa700-0x9d00)*2 )
	ADPCM_SAMPLE( 0x053, 0xa700, (0xb200-0xa700)*2 )
	ADPCM_SAMPLE( 0x059, 0xb200, (0xe700-0xb200)*2 )
	/* samples on ROM 2 */
	ADPCM_SAMPLE( 0x100, 0x10000+0x0000, (0x1600-0x0000)*2 )
	ADPCM_SAMPLE( 0x10b, 0x10000+0x1600, (0x2500-0x1600)*2 )
	ADPCM_SAMPLE( 0x112, 0x10000+0x2500, (0x3500-0x2500)*2 )
	ADPCM_SAMPLE( 0x11a, 0x10000+0x3500, (0x4d00-0x3500)*2 )
	ADPCM_SAMPLE( 0x126, 0x10000+0x4d00, (0x7900-0x4d00)*2 )
	ADPCM_SAMPLE( 0x13c, 0x10000+0x7900, (0x8f00-0x7900)*2 )
	ADPCM_SAMPLE( 0x148, 0x10000+0x9000, (0x9900-0x9000)*2 )
	ADPCM_SAMPLE( 0x14c, 0x10000+0x9900, (0xa400-0x9900)*2 )
	ADPCM_SAMPLE( 0x152, 0x10000+0xa400, (0xaf00-0xa400)*2 )
	ADPCM_SAMPLE( 0x157, 0x10000+0xaf00, (0xb700-0xaf00)*2 )
	ADPCM_SAMPLE( 0x15a, 0x10000+0xb400, (0xce00-0xb400)*2 )
	ADPCM_SAMPLE( 0x168, 0x10000+0xd100, (0xe500-0xd100)*2 )
	ADPCM_SAMPLE( 0x172, 0x10000+0xe500, (0xf400-0xe500)*2 )
	ADPCM_SAMPLE( 0x1f4, 0x10000+0xf400, (0xfd00-0xf400)*2 )
ADPCM_SAMPLES_END

struct GameDriver ddragon_driver =
{
	"Double Dragon",
	"ddragon",
	"Carlos A. Lozano\nRob Rosenbrock\nChris Moore\nPhil Stroffolino\nErnesto Corvi\n\n\n"
	"This driver does not work and we are aware of it.\nPlay with the bootleg version in the meantime.\n",
	&ddragon_machine_driver,

	ddragon_rom,
	0, 0,
	0,
	(void *)dd_samples,

	dd1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver ddragonb_driver =
{
	"Double Dragon (bootleg)",
	"ddragonb",
	"Carlos A. Lozano\nRob Rosenbrock\nChris Moore\nPhil Stroffolino\nErnesto Corvi\n",
	&ddragonb_machine_driver,

	dd_rom,
	0, 0,
	0,
	(void *)dd_samples,

	dd1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver ddragon2_driver =
{
	"Double Dragon 2",
	"ddragon2",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\nErnesto Corvi\n",
	&ddragon2_machine_driver,

	ddragon2_rom,
	0, 0,
	0,
	0,

	dd2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
