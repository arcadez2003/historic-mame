/***************************************************************************

Super Basketball memory map (preliminary)
(Hold down 1 & 2 keys to enter test mode on start up)

MAIN BOARD:
0000-2fff RAM
2500-2900 Sprite RAM
3000-33ff Color RAM
3400-37ff Video RAM
6000-ffff ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "M6809.h"


extern unsigned char *sbasketb_scroll;
void sbasketb_vh_screenrefresh(struct osd_bitmap *bitmap);



void sbasketb_init_machine(void)
{
        /* Set optimization flags for M6809 */
        m6809_Flags = M6809_FAST_OP | M6809_FAST_S | M6809_FAST_U;
}



static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x3bff, MRA_RAM },    /* RAM and Video RAM */
        { 0x3c10, 0x3c10, MRA_NOP },    /* ???? */
        { 0x3e00, 0x3e00, input_port_0_r },
        { 0x3e01, 0x3e01, input_port_1_r },
        { 0x3e02, 0x3e02, input_port_2_r },
        { 0x3e03, 0x3e03, MRA_NOP },    /* ???? */
        { 0x3e80, 0x3e80, input_port_3_r },
        { 0x3f00, 0x3f00, input_port_4_r },
        { 0x6000, 0xffff, MRA_ROM },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x0000, 0x2fff, MWA_RAM },
        { 0x2500, 0x28ff, MWA_RAM, &spriteram, &spriteram_size },
        { 0x3000, 0x33ff, colorram_w, &colorram },
        { 0x3400, 0x37ff, videoram_w, &videoram, &videoram_size },
        { 0x3800, 0x3bff, MWA_RAM },
        { 0x3c00, 0x3c00, MWA_NOP },  /* Gets written to it all the time. Dunno what it does */
        { 0x3c20, 0x3c20, MWA_NOP },  /* ??? */
        { 0x3c80, 0x3c80, MWA_NOP },  /* ??? */
        { 0x3c81, 0x3c81, interrupt_enable_w },  /* Interrupt enable */
        { 0x3c83, 0x3c83, MWA_NOP },  /* Coin light 1 */
        { 0x3c84, 0x3c84, MWA_NOP },  /* Coin light 2 */
        { 0x3c85, 0x3c85, MWA_NOP },  /* Counter gets incremented on every interrupt */
        { 0x3d00, 0x3d00, sound_command_w },  /* Sound Command */
        { 0x3d80, 0x3d80, sound_command_w },  /* Sound Command (Same as 3d00 ???) */
        { 0x3f80, 0x3f80, MWA_RAM, &sbasketb_scroll },  /* Scroll amount */
        { 0x6000, 0xffff, MWA_ROM },
        { -1 }  /* end of table */
};

#if 0
static struct MemoryReadAddress sound_readmem[] =
{
        { 0x4000, 0x43ff, MRA_RAM },
        { 0x8000, 0x8000, MRA_RAM },  /* ??? */
        { 0x6000, 0x6000, sound_command_r },
        { 0x0000, 0x3fff, MRA_ROM },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
        { 0x4000, 0x43ff, MWA_RAM },
        { 0x0000, 0x3fff, MWA_ROM },
        { -1 }  /* end of table */
};
#endif



static struct InputPort input_ports[] =
{
  {       /* IN0 */
          0xff,
          { OSD_KEY_3, 0, OSD_KEY_F1, OSD_KEY_1, OSD_KEY_2, 0, 0, 0},
          { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {       /* IN1 */
          0xff,
          { OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_UP, OSD_KEY_DOWN,
            OSD_KEY_CONTROL, OSD_KEY_ALT, OSD_KEY_SPACE, 0},
          { OSD_JOY_LEFT, OSD_JOY_RIGHT, OSD_JOY_UP, OSD_JOY_DOWN,
            OSD_JOY_FIRE1, OSD_JOY_FIRE2, OSD_JOY_FIRE3, 0 },
  },
  {       /* IN2 */
          0xff,                         /* Player 2 Controls (Table only) */
          { 0, 0, 0, 0, 0, 0, 0, 0 },
          { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {       /* DSW1 */
          0xf8,
          { 0, 0, 0, 0, 0, 0, 0, 0 },
          { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
  {       /* DSW2 */
          0xff,
          { 0, 0, 0, 0, 0, 0, 0, 0 },
          { 0, 0, 0, 0, 0, 0, 0, 0 },
  },
        { -1 }  /* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
  { 1, 0, "MOVE LEFT" },
  { 1, 1, "MOVE RIGHT"  },
  { 1, 2, "MOVE UP" },
  { 1, 3, "MOVE DOWN" },
  { 1, 4, "DRIBBLE" },
  { 1, 5, "SHOOT" },
  { 1, 6, "PASS" },
  { -1 }
};


static struct DSW sbasketb_dsw[] =
{
  { 3, 0x08, "STARTING SCORE", {"115 - 100", " 78 -  70"}, 1},
  { 4, 0x0f, "COINS/PLAY",   { "FREE PLAY", "4/3",
                               "4/1", "3/4",
                               "3/2", "3/1",
                               "2/5", "2/3",
                               "2/1", "1/7",
                               "1/6", "1/5",
                               "1/4", "1/3",
                               "1/2", "1/1"}, 1 },
  { 3, 0x03, "GAME TIME", {"60 SECS", "40 SECS", "50 SECS", "30 SECS"}, 1},
  { 3, 0x80, "ATTRACT SOUND", {"ON", "OFF"}, 1},
  { 3, 0x10, "BUCK UP", {"ON", "OFF"}, 1},
          { -1 }
};

static struct GfxLayout charlayout =
{
        8,8,    /* 8*8 characters */
        256,    /* 256 characters */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 }, /* the bitplanes are packed */
        { 7*4*8, 6*4*8, 5*4*8, 4*4*8, 3*4*8, 2*4*8, 1*4*8, 0*4*8 },
        { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
        8*4*8     /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
        16,16,  /* 16*16 sprites */
        128 * 3,/* 384 sprites */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 },        /* the bitplanes are packed */
        { 15*4*16, 14*4*16, 13*4*16, 12*4*16, 11*4*16, 10*4*16, 9*4*16, 8*4*16,
           7*4*16, 6*4*16, 5*4*16, 4*4*16, 3*4*16, 2*4*16, 1*4*16, 0*4*16 },
        { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
                        8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
        32*4*8    /* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 1, 0x0000, &charlayout,     0, 1 },
        { 1, 0x2000, &charlayout,     0, 1 },
        { 1, 0x4000, &spritelayout,   0, 1 },
        { -1 } /* end of array */
};



/* These colors are obviously wrong */
static unsigned char palette[] =
{
        0   , 0   , 0,
        0x10, 0x10, 0x10,
        0x20, 0x20, 0x20,
        0x30, 0x30, 0x30,
        0x40, 0x40, 0x40,
        0x50, 0x50, 0x50,
        0x60, 0x60, 0x60,
        0x70, 0x70, 0x70,
        0x80, 0x80, 0x80,
        0x90, 0x90, 0x90,
        0xa0, 0xa0, 0xa0,
        0xb0, 0xb0, 0xb0,
        0xc0, 0xc0, 0xc0,
        0xd0, 0xd0, 0xd0,
        0xe0, 0xe0, 0xe0,
        0xf0, 0xf0, 0xf0
};


static unsigned char colortable[] =
{
        /* characters and sprites */
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};


static struct MachineDriver sbasketb_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_M6809,
                        1400000,        /* 1.400 Mhz ??? */
                        0,
                        readmem,writemem,0,0,
                        interrupt,1
                },
#if 0
                {
                        CPU_Z80 | CPU_AUDIO_CPU,
                        3072000,        /* 1.400 Mhz ??? */
                        2,
                        sound_readmem,sound_writemem,0,0,
                        interrupt,1
                }
#endif
        },
        60,
        sbasketb_init_machine,

        /* video hardware */
        32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
        gfxdecodeinfo,
        sizeof(palette)/3,sizeof(colortable),
        0,

	VIDEO_TYPE_RASTER,
        0,
        generic_vh_start,
        generic_vh_stop,
        sbasketb_vh_screenrefresh,

        /* sound hardware */
        0,
        0,
        0,
        0,
        0
#if 0
        AY8910_sh_stop,                 /* sh_stop routine */
        AY8910_sh_update                /* sh_update routine */
#endif
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sbasketb_rom )
        ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "sbb_j13.bin", 0x6000, 0x2000, 0x31c615be )
        ROM_LOAD( "sbb_j11.bin", 0x8000, 0x4000, 0x0f580fca )
        ROM_LOAD( "sbb_j09.bin", 0xc000, 0x4000, 0xb5e9f53d )

        ROM_REGION(0x10000)    /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "sbb_e12.bin", 0x0000, 0x4000, 0xc34f6429 )
        ROM_LOAD( "sbb_h06.bin", 0x4000, 0x4000, 0x613e1e6c )
        ROM_LOAD( "sbb_h08.bin", 0x8000, 0x4000, 0x5eb646ec )
        ROM_LOAD( "sbb_h10.bin", 0xc000, 0x4000, 0xd02d8a3d )

        ROM_REGION(0x10000)     /* 64k for audio cpu */
        ROM_LOAD( "sbb_e13.bin", 0x0000, 0x2000, 0x00000000 )
        ROM_LOAD( "sbb_e15.bin", 0x2000, 0x2000, 0x00000000 )
ROM_END


static int hiload(const char *name)
{
        /* get RAM pointer (this game is multiCPU, we can't assume the global */
        /* RAM pointer is pointing to the right place) */
        unsigned char *RAM = Machine->memory_region[0];


        /* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x2bb7],"\x15\x1e\x14",3) == 0)
        {
                FILE *f;


                if ((f = fopen(name,"rb")) != 0)
                {
                    fread(&RAM[0x2b24],1,0x96,f);
                    fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
        FILE *f;
        /* get RAM pointer (this game is multiCPU, we can't assume the global */
        /* RAM pointer is pointing to the right place) */
        unsigned char *RAM = Machine->memory_region[0];


        if ((f = fopen(name,"wb")) != 0)
        {
                fwrite(&RAM[0x2b24],1,0x96,f);
                fclose(f);
        }
}



struct GameDriver sbasketb_driver =
{
        "Super Basketball",
        "sbasketb",
        "ZSOLT VASVARI",
        &sbasketb_machine_driver,

        sbasketb_rom,
        0, 0,
        0,

        input_ports, 0, trak_ports, sbasketb_dsw, keys,

        0, palette, colortable,
        ORIENTATION_DEFAULT,

        hiload, hisave
};