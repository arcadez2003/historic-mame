/***************************************************************************

Commando memory map (preliminary)

MAIN CPU
0000-bfff ROM
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff background video RAM
dc00-dfff background color RAM
e000-ffff RAM
fe00-ff7f Sprites

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW1
c004      DSW2

write:
c808-c809 background scroll y position
c80a-c80b background scroll x position

SOUND CPU
0000-3fff ROM
4000-47ff RAM

write:
8000      YM2203 #1 control
8001      YM2203 #1 write
8002      YM2203 #2 control
8003      YM2203 #2 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



int commando_interrupt(void);

extern unsigned char *commando_bgvideoram,*commando_bgcolorram;
extern int commando_bgvideoram_size;
extern unsigned char *commando_scrollx,*commando_scrolly;
void commando_bgvideoram_w(int offset,int data);
void commando_bgcolorram_w(int offset,int data);
int commando_vh_start(void);
void commando_vh_stop(void);
void commando_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void commando_vh_screenrefresh(struct osd_bitmap *bitmap);

int capcom_sh_start(void);
int capcom_sh_interrupt(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xe000, 0xff7f, MRA_RAM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc808, 0xc809, MWA_RAM, &commando_scrolly },
	{ 0xc80a, 0xc80b, MWA_RAM, &commando_scrollx },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xdbff, commando_bgvideoram_w, &commando_bgvideoram, &commando_bgvideoram_size },
	{ 0xdc00, 0xdfff, commando_bgcolorram_w, &commando_bgcolorram },
	{ 0xe000, 0xfdff, MWA_RAM },
	{ 0xfe00, 0xff7f, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, AY8910_control_port_1_w },
	{ 0x8003, 0x8003, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_LCONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
	{ 1, 3, "MOVE UP" },
	{ 1, 1, "MOVE LEFT"  },
	{ 1, 0, "MOVE RIGHT" },
	{ 1, 2, "MOVE DOWN" },
	{ 1, 4, "FIRE" },
	{ 1, 5, "GRENADE" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x0c, "LIVES", { "5", "2", "4", "3" }, 1 },
	{ 4, 0x07, "BONUS", { "NONE", "20000 700000", "30000 800000", "10000 600000", "40000 1000000", "20000 600000", "30000 700000", "10000 500000" }, 1 },
	{ 4, 0x10, "DIFFICULTY", { "DIFFICULT", "NORMAL" }, 1 },
	{ 3, 0x03, "STARTING STAGE", { "7", "3", "5", "1" }, 1 },
	{ 4, 0x08, "DEMO SOUNDS", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	1024,	/* 1024 tiles */
	3,	/* 3 bits per pixel */
	{ 0, 1024*32*8, 2*1024*32*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	768,	/* 768 sprites */
	4,	/* 4 bits per pixel */
	{ 768*64*8+4, 768*64*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,           0, 16 },
	{ 1, 0x04000, &tilelayout,   16*4+4*16, 16 },
	{ 1, 0x1c000, &spritelayout,      16*4, 4 },
	{ -1 } /* end of array */
};



/* these are NOT the original color PROMs */
static unsigned char color_prom[] =
{
	/* 1D: palette red component */
	0x00,0x08,0x00,0x09,0x06,0x00,0x07,0x03,0x04,0x08,0x0A,0x07,0x05,0x03,0x07,0x05,
	0x03,0x08,0x00,0x06,0x04,0x00,0x07,0x03,0x0A,0x08,0x09,0x06,0x04,0x03,0x06,0x04,
	0x0A,0x08,0x05,0x05,0x04,0x09,0x05,0x07,0x00,0x00,0x00,0x00,0x06,0x04,0x07,0x0B,
	0x00,0x09,0x0A,0x08,0x0B,0x07,0x04,0x06,0x03,0x08,0x07,0x0A,0x08,0x06,0x04,0x08,
	0x05,0x08,0x00,0x09,0x06,0x00,0x07,0x03,0x08,0x03,0x0A,0x09,0x07,0x07,0x05,0x04,
	0x09,0x08,0x0A,0x07,0x06,0x05,0x04,0x07,0x00,0x08,0x06,0x05,0x04,0x09,0x07,0x0B,
	0x00,0x08,0x06,0x05,0x04,0x09,0x07,0x08,0x00,0x08,0x08,0x0A,0x08,0x06,0x00,0x0C,
	0x00,0x09,0x07,0x06,0x04,0x0A,0x06,0x04,0x00,0x08,0x0A,0x07,0x06,0x05,0x04,0x07,
	0x01,0x00,0x00,0x00,0x0C,0x0A,0x08,0x0C,0x07,0x0C,0x0A,0x0A,0x08,0x06,0x04,0x00,
	0x01,0x0A,0x08,0x06,0x06,0x04,0x02,0x0C,0x07,0x06,0x0B,0x09,0x07,0x05,0x04,0x00,
	0x01,0x0A,0x07,0x05,0x09,0x09,0x05,0x0B,0x05,0x03,0x07,0x08,0x07,0x06,0x04,0x00,
	0x01,0x07,0x05,0x03,0x08,0x04,0x06,0x0C,0x05,0x0F,0x09,0x0A,0x09,0x07,0x05,0x00,
	0x0A,0x0A,0x00,0x00,0x0C,0x0B,0x00,0x00,0x0C,0x0B,0x00,0x00,0x0C,0x00,0x00,0x00,
	0x0C,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x0C,0x0A,0x00,0x00,0x0A,0x00,0x04,0x00,
	0x0B,0x03,0x06,0x00,0x05,0x0B,0x0A,0x00,0x05,0x0B,0x00,0x00,0x0B,0x05,0x08,0x00,
	0x03,0x07,0x05,0x00,0x04,0x0A,0x00,0x00,0x0C,0x00,0x03,0x00,0x08,0x04,0x06,0x00,
	/* 2D: palette green component */
	0x00,0x05,0x05,0x06,0x03,0x07,0x04,0x04,0x03,0x06,0x08,0x08,0x06,0x04,0x05,0x04,
	0x04,0x06,0x05,0x08,0x06,0x07,0x05,0x04,0x08,0x06,0x0B,0x08,0x06,0x04,0x04,0x03,
	0x08,0x06,0x05,0x06,0x04,0x07,0x04,0x05,0x00,0x09,0x06,0x04,0x06,0x04,0x05,0x09,
	0x00,0x07,0x08,0x06,0x09,0x05,0x03,0x04,0x03,0x06,0x07,0x0A,0x08,0x06,0x04,0x05,
	0x04,0x06,0x05,0x07,0x04,0x07,0x05,0x04,0x05,0x04,0x09,0x08,0x06,0x04,0x04,0x03,
	0x07,0x06,0x0A,0x07,0x06,0x05,0x04,0x05,0x00,0x05,0x03,0x02,0x00,0x06,0x04,0x09,
	0x00,0x05,0x03,0x02,0x00,0x06,0x04,0x05,0x00,0x06,0x05,0x09,0x07,0x05,0x00,0x0A,
	0x00,0x07,0x05,0x04,0x03,0x08,0x06,0x04,0x00,0x05,0x0A,0x07,0x06,0x05,0x04,0x04,
	0x01,0x09,0x06,0x04,0x0A,0x08,0x06,0x0A,0x05,0x0C,0x00,0x0A,0x08,0x06,0x04,0x00,
	0x01,0x09,0x07,0x05,0x08,0x06,0x04,0x0A,0x05,0x06,0x0A,0x09,0x07,0x05,0x04,0x00,
	0x01,0x0A,0x07,0x05,0x08,0x06,0x02,0x09,0x05,0x04,0x06,0x05,0x04,0x03,0x00,0x00,
	0x01,0x09,0x06,0x04,0x06,0x03,0x04,0x0A,0x05,0x0E,0x0C,0x08,0x07,0x05,0x04,0x00,
	0x00,0x0A,0x00,0x00,0x0C,0x0A,0x00,0x00,0x0C,0x06,0x00,0x00,0x0C,0x00,0x00,0x00,
	0x0C,0x00,0x00,0x00,0x0C,0x08,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x06,0x04,0x00,
	0x09,0x02,0x03,0x00,0x04,0x0A,0x00,0x00,0x04,0x0A,0x07,0x00,0x0A,0x04,0x06,0x00,
	0x05,0x09,0x07,0x00,0x04,0x09,0x06,0x00,0x0C,0x0B,0x02,0x00,0x08,0x04,0x06,0x00,
	/* 3D: palette blue component */
	0x00,0x03,0x07,0x04,0x00,0x08,0x00,0x05,0x00,0x04,0x06,0x05,0x02,0x00,0x03,0x03,
	0x00,0x04,0x07,0x04,0x00,0x08,0x03,0x05,0x06,0x04,0x06,0x04,0x00,0x00,0x02,0x00,
	0x06,0x04,0x03,0x03,0x00,0x05,0x03,0x03,0x00,0x0A,0x08,0x05,0x05,0x03,0x03,0x07,
	0x00,0x05,0x06,0x04,0x07,0x03,0x00,0x00,0x02,0x04,0x06,0x09,0x07,0x05,0x03,0x03,
	0x03,0x04,0x07,0x05,0x00,0x08,0x03,0x05,0x03,0x05,0x06,0x05,0x00,0x00,0x00,0x00,
	0x05,0x04,0x09,0x06,0x05,0x04,0x03,0x03,0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x07,
	0x00,0x03,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x04,0x03,0x07,0x05,0x03,0x00,0x08,
	0x00,0x05,0x03,0x00,0x00,0x06,0x05,0x03,0x00,0x03,0x09,0x06,0x05,0x04,0x03,0x00,
	0x01,0x0A,0x08,0x05,0x00,0x00,0x00,0x08,0x03,0x0F,0x00,0x09,0x07,0x05,0x03,0x00,
	0x01,0x07,0x05,0x03,0x08,0x05,0x03,0x08,0x03,0x05,0x00,0x08,0x06,0x04,0x03,0x00,
	0x01,0x0A,0x07,0x05,0x05,0x04,0x00,0x07,0x04,0x05,0x00,0x03,0x00,0x00,0x00,0x00,
	0x01,0x04,0x00,0x00,0x04,0x00,0x00,0x08,0x04,0x0B,0x04,0x06,0x05,0x03,0x03,0x00,
	0x00,0x0A,0x00,0x00,0x0C,0x00,0x00,0x00,0x0C,0x0A,0x00,0x00,0x0C,0x0B,0x00,0x00,
	0x0C,0x0C,0x00,0x00,0x0C,0x0B,0x00,0x00,0x0C,0x00,0x00,0x00,0x00,0x0C,0x07,0x00,
	0x07,0x00,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x09,0x06,0x00,0x03,0x07,0x09,0x00,0x0C,0x07,0x06,0x00,0x07,0x03,0x05,0x00
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			commando_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			capcom_sh_interrupt,12
		}
	},
	60,
	10,	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,16*4+4*16+16*8,
	commando_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	commando_vh_start,
	commando_vh_stop,
	commando_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	capcom_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( commando_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "m09_cm04.bin", 0x0000, 0x8000, 0xf44b9f43 )
	ROM_LOAD( "m08_cm03.bin", 0x8000, 0x4000, 0x6e158a17 )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000, 0x9c3344b3 )	/* characters */
	ROM_LOAD( "a05_vt11.bin", 0x04000, 0x4000, 0x0babe1d9 )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x08000, 0x4000, 0x0ef15ee7 )
	ROM_LOAD( "a07_vt13.bin", 0x0c000, 0x4000, 0x8244ea38 )
	ROM_LOAD( "a08_vt14.bin", 0x10000, 0x4000, 0x91390ad1 )
	ROM_LOAD( "a09_vt15.bin", 0x14000, 0x4000, 0x755876be )
	ROM_LOAD( "a10_vt16.bin", 0x18000, 0x4000, 0x8c6d8225 )
	ROM_LOAD( "e07_vt05.bin", 0x1c000, 0x4000, 0x4eda8b78 )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x20000, 0x4000, 0x280b34f9 )
	ROM_LOAD( "e09_vt07.bin", 0x24000, 0x4000, 0x2ab5880f )
	ROM_LOAD( "h07_vt08.bin", 0x28000, 0x4000, 0x48696aa7 )
	ROM_LOAD( "h08_vt09.bin", 0x2c000, 0x4000, 0xab521082 )
	ROM_LOAD( "h09_vt10.bin", 0x30000, 0x4000, 0x998c53a6 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "f09_cm02.bin", 0x0000, 0x4000, 0x07fced60 )
ROM_END

ROM_START( commandj_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "09m_so04.bin", 0x0000, 0x8000, 0xf5dffe09 )
	ROM_LOAD( "08m_so03.bin", 0x8000, 0x4000, 0xf8463efe )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "05d_vt01.bin", 0x00000, 0x4000, 0x9c3344b3 )	/* characters */
	ROM_LOAD( "05a_vt11.bin", 0x04000, 0x4000, 0x0babe1d9 )	/* tiles */
	ROM_LOAD( "06a_vt12.bin", 0x08000, 0x4000, 0x0ef15ee7 )
	ROM_LOAD( "07a_vt13.bin", 0x0c000, 0x4000, 0x8244ea38 )
	ROM_LOAD( "08a_vt14.bin", 0x10000, 0x4000, 0x91390ad1 )
	ROM_LOAD( "09a_vt15.bin", 0x14000, 0x4000, 0x755876be )
	ROM_LOAD( "10a_vt16.bin", 0x18000, 0x4000, 0x8c6d8225 )
	ROM_LOAD( "07e_vt05.bin", 0x1c000, 0x4000, 0x4eda8b78 )	/* sprites */
	ROM_LOAD( "08e_vt06.bin", 0x20000, 0x4000, 0x280b34f9 )
	ROM_LOAD( "09e_vt07.bin", 0x24000, 0x4000, 0x2ab5880f )
	ROM_LOAD( "07h_vt08.bin", 0x28000, 0x4000, 0x48696aa7 )
	ROM_LOAD( "08h_vt09.bin", 0x2c000, 0x4000, 0xab521082 )
	ROM_LOAD( "09h_vt10.bin", 0x30000, 0x4000, 0x998c53a6 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "09f_so02.bin", 0x0000, 0x4000, 0xfda056a2 )
ROM_END



static void commando_decode(void)
{
	static const unsigned char xortable[] =
	{
		0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,
		0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,
		0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,
		0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,
		0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,
		0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,
		0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,
		0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,
		0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,
		0x88,0x88,0xaa,0xaa,0xcc,0xcc,0xee,0xee,0x00,0x00,0x22,0x22,0x44,0x44,0x66,0x66,
		0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,
		0xaa,0xaa,0x88,0x88,0xee,0xee,0xcc,0xcc,0x22,0x22,0x00,0x00,0x66,0x66,0x44,0x44,
		0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,
		0xcc,0xcc,0xee,0xee,0x88,0x88,0xaa,0xaa,0x44,0x44,0x66,0x66,0x00,0x00,0x22,0x22,
		0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00,
		0xee,0xee,0xcc,0xcc,0xaa,0xaa,0x88,0x88,0x66,0x66,0x44,0x44,0x22,0x22,0x00,0x00
	};
	int A;


	for (A = 0;A < 0xc000;A++)
	{
		if (A > 0) ROM[A] = RAM[A] ^ xortable[RAM[A]];
	}
}



static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xee00],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0xee4e],"\x00\x08\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xee00],13*7);
			RAM[0xee97] = RAM[0xee00];
			RAM[0xee98] = RAM[0xee01];
			RAM[0xee99] = RAM[0xee02];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xee00],13*7);
		osd_fclose(f);
	}
}



struct GameDriver commando_driver =
{
	"Commando (US version)",
	"commando",
	"PAUL JOHNSON\nNICOLA SALMORIA",
	&machine_driver,

	commando_rom,
	0, commando_decode,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver commandj_driver =
{
	"Commando (Japanese version)",
	"commandj",
	"PAUL JOHNSON\nNICOLA SALMORIA",
	&machine_driver,

	commandj_rom,
	0, commando_decode,
	0,

	input_ports, 0, trak_ports, dsw, keys,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
