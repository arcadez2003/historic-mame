/*

Taito JC System

-- will need a 68040 core
-- they upload a font to ram, all games upload the same font, boring ;-)

Side By Side 2
Taito, 1997

This game runs on Taito JC System hardware

PCB Layout
----------

Top board: MOTHER PCB-C K11X0838A  M43E0325A
|---------------------------------------------------------------------------------|
|                                                                 43256           |
|54MHz                                   E23-32-1.51              43256           |
|         424210 424210                                           43256           |
|                      E23-29.39                                                  |
|E23-25-1.3 TCO870HVP             E23-31.46                                       |
|           (QFP208)   E23-30.40             E23-34.72      93C46.87              |
|E23-26.4                                                                         |
|                              MC68040RC25            CXD1178Q    TCO640FIO       |
|424260     TMS320C31          (PGA TYPE)                         (QFP120)        |
|           (QFP132)                                                              |
|           labelled                                              TEST_SW         |
|424260     "Taito E07-11"                                      MB3771   RESET_SW |
|                                  E23-33.53                                      |
|                                             CY7B991       MB8421-90             |
|4218160    43256                    CY7B991                                      |
|                                                                                 |
|           43256        TCO770CMU                          E23-35.110            |
|4218160                 (QFP208)                                                 |
|                                    10MHz    MC68EC000     LC321664AJ-80         |
|E23-27.13  TCO780FPA                                                             |
|           (QFP240)                                        ENSONIC               |
|                      D482445                TC51832       ESPR6 ES5510          |
|                                             TC51832                             |
|4218160               D482445                                                    |
|                                 TCO840GLU                 MC33274   TDA1543     |
|                      D482445    (QFP144)                                        |
|4218160                                      16MHz         MB87078               |
|           TCO780FPA  D482445           30.4761MHz                               |
|           (QFP240)              ENSONIC                                         |
|E23-28.18                        OTISR2                                          |
|                                                                                 |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.


Bottom board: JCG DAUGHTERL PCB-C K91E0677A
|---------------------------------------------------------------------------------|
|                                                                                 |
|                                                                                 |
|                    E38-19.30  E38-20.31                                         |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|  E38-01.5   E38-09.18   E23-15.32                                               |
|                                                                                 |
|  E38-02.6   E38-10.19   E38-17.33                                               |
|                                                                                 |
|  E38-03.7   E38-11.20   E38-18.34                     E17-23.65                 |
|                                                                                 |
|  E38-04.8   E38-12.21   E38-21.35                     TC5563                    |
|                                                                                 |
|  E38-05.9   E38-13.22   SBS2_P0.36                       MC68HC11M0             |
|                                                          (QFP80)                |
|  E38-06.10  E38-14.23   SBS2_P1.37                                              |
|                                                                                 |
|  E38-07.11  E38-15.24   SBS2_P2.38                                              |
|                                                          E23-37.69              |
|  E38-08.12  E38-16.25   SBS2_P3.39                             SMC_COM20020I    |
|                                                                                 |
|                                                                                 |
|                                                      E23-38.73                  |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.
      MC68HC11: Microcontroller with 64k internal ROM (not read but probably protected anyway ;-)
      ROMs .36-.39 are 27C4001, main program.
      ROMs .5-.7, .9-.12, .18-.20, .22-.25 are 16M MASK, graphics.
      ROMs .32-.35 are 16M MASK, ES5505 samples.
      ROMs .8 and .21 are 4M MASK, graphics.
      ROMs .30-.31 are 27C2001, 68000 sound program.
      ROM .65 is 27C512, use unknown.

----

--------------------------------------------------------------------------
SIDE BY SIDE  / 2       JC-SYSTEM TYPE-C
--------------------------------------------------------------------------
 SIDE BY SIDE (E23) (C)TAITO 1996 VER 2.5J 1996/6/20 18:13:14
 E23-01 to 24 + E17-23 (BIOS ?)   E17:LANDING GEAR

 SIDE BY SIDE 2 (E38)   Not dumped
 ROMKIT : E38-01 to 21 , 23* to 26*

--------------------------------------------------------------------------
DENSYA DE GO (E35)     JC-SYSTEM TYPE-C with TRAIN BOARD (Ext.Sound)
--------------------------------------------------------------------------
 DENSYA DE GO VER 2.2J 1997/2/4
 E35-01 to 26 + E35-28(TRAIN BOARD) + E17-23(BIOS?)

 DENSYA DE GO EX VER 2.4J 1997/4/18 13:38:34
 ROMKIT : E35-30 to 33

--------------------------------------------------------------------------
DENSYA DE GO 2 (E52)   JC-SYSTEM TYPE-C with TRAIN BOARD (Ext.Sound)
--------------------------------------------------------------------------
 DENSYA DE GO 2 (KOUSOKUHEN RYOUSANSYA) VER 2.5 J 1998/3/2 15:30:55
 E52-01 to 24 , 25-1 to 28-1, 29, 30 + E35-28(TRAIN BOARD) + E17-23(BIOS?)

 DENSYA DE GO! 2 EX (3000BANDAI KOUSOKUHEN) VER 2.20 J 1998/7/15 17:42:38
 ROMKIT :  E52-31 to 38

----

Landing Gear
Taito, 1995

This is a flight simulator game running on Taito JC System hardware.
The system comprises two boards plugged together back-to-back via five high density connectors.
The top board contains the main CPU, all RAMs, graphics and sound hardware and the bottom
board contains the game ROMs, communication devices and a MC68HC11 MCU. Landing Gear
is a single player game so the board does not contain any communication hardware.
This hardware seems to be the earliest revision of the JC board. This is the only JC game that
had a 28-way edge connector, which is actually almost JAMMA. The power/GND, video and
most buttons (test/service/coin/start etc) work fine using any standard JAMMA cab. However the
analog controls are wired to another connector on the bottom PCB via an interface board.
When other JC bottom boards are swapped to this top board, they will run fine, but some of the
graphics are messed up. This is probably due to the different PALs and their changed locations.
This seems to mostly affect the texture mapping (tested with swapping Side By Side 2) as the title
and most of the background graphics are ok but the car textures are just a mess of pixels.


PCB Layout
----------

Top board: MOTHER PCB  K11X0835A  M43E0304A
|---------------------------------------------------------------------------------|
|                                                                 43256  43256    |
|54MHz                                                            43256           |
|                                                                                 |
|TC514260                                                                         |
|         TCO870HVP  uPD424210                                    TCO640FIO       |
|TC514260 (QFP208)   uPD424210             E07-08.65              (QFP120)        |
|E07-02.4                                                                         |
|                                                                                 |
|TMS320C31  43256                                                                 |
|(QFP132)   43256              MC68040RC25          E07-10.116  93C46.91          |
|labelled          TCO770CMU   (PGA TYPE)                    E07-04.115  TEST_SW  |
|"Taito E07-11"    (QFP208)                         E07-09.82      MB3771         |
|                                                                                 |
|                                                            MB8421-90            |
|TC528257   E07-06.37                                                             |
|                                                                                 |
|TC528257   TC514260                                                              |
|                              E07-07.49    CY7B991                               |
|TC528257   TC514260           E07-03.50                     TC511664             |
|                     TCO780FPA                                                   |
|TC528257   TC514260  (QFP240)           10MHz   MC68EC000   ENSONIC              |
|                                 CY7B991                    ESPR6 ES5510         |
|TC528257   TC514260                                                              |
|                                                                                 |
|TC528257   TC514260              TCO840GLU   TC51832       MC33274   TDA1543     |
|                                 (QFP144)    TC51832                             |
|TC528257   TC514260                             16MHz      MB87078               |
|                     TCO780FPA          30.4761MHz                               |
|TC528257   TC514260  (QFP240)                                                    |
|                                           ENSONIC                               |
|TC528257             E07-05.22             OTISR2                                |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.


Bottom board: JCG DAUGHTER PCB-L K91E0603A
|---------------------------------------------------------------------------------|
|                                                                                 |
|                                                                                 |
|                    E17-21.30  E17-22.31                                         |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|  E17-01.5   E17-07.18   E17-13.32                                               |
|                                                                                 |
|  *          E17-08.19   E17-14.33                                               |
|                                                                                 |
|  *          *           E17-15.34                     E17-23.65                 |
|                                                                                 |
|  E17-02.8   *           E17-16.35                     TC5563                    |
|                                                                                 |
|  E17-03.9   E17-09.22   E17-37.36                        MC68HC11M0             |
|                                                          (QFP80)                |
|  E17-04.10  E17-10.23   E17-38.37                                               |
|                                                                                 |
|  E17-05.11  E17-11.24   E17-39.38                                               |
|                                                          E09-21.69              |
|  E17-06.12  E17-12.25   E17-40.39                                               |
|                                                                                 |
|                                                                                 |
|                                                      E09-22.73                  |
|                                                                                 |
|                                                                                 |
|                                                      E17-32.96                  |
|                                                                                 |
|                                                                                 |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.
      MC68HC11: Surface-mounted QFP80 microcontroller with 64k internal ROM (not read but very likely protected)
      ROMs .36-.39 are 27C4001, main program.
      ROM .65 is 27C512, use unknown, possibly used with 68HC11 MCU.
      ROMs .30-.31 are 27C2001, sound program.
      ROMs .32-.35 are 16M MASK, sound.
      ROMs .5-.25 are 16M MASK, graphics.
      * Unpopulated ROM socket

----

Side By Side
Taito, 1996

This game runs on Taito JC System hardware and uses a 24kHz monitor.

PCB Layout
----------

Top board: MOTHER PCB-C K11X0838A  M43E0325A
|---------------------------------------------------------------------------------|
|                                                                 43256           |
|54MHz                                   E23-32-1.51              43256           |
|         424210 424210                                           43256           |
|                      E23-29.39                                                  |
|E23-25-1.3 TCO870HVP             E23-31.46                                       |
|           (QFP208)   E23-30.40             E23-34.72      93C46.87              |
|E23-26.4                                                                         |
|                              MC68040RC25            CXD1178Q    TCO640FIO       |
|424260     TMS320C31          (PGA TYPE)                         (QFP120)        |
|           (QFP132)                                                              |
|           labelled                                              TEST_SW         |
|424260     "Taito E07-11"                                      MB3771   RESET_SW |
|                                  E23-33.53                                      |
|                                             CY7B991       MB8421-90             |
|4218160    43256                    CY7B991                                      |
|                                                                                 |
|           43256        TCO770CMU                          E23-35.110            |
|4218160                 (QFP208)                                                 |
|                                    10MHz    MC68EC000     LC321664AJ-80         |
|E23-27.13  TCO780FPA                                                             |
|           (QFP240)                                        ENSONIC               |
|                      D482445                TC51832       ESPR6 ES5510          |
|                                             TC51832                             |
|4218160               D482445                                                    |
|                                 TCO840GLU                 MC33274   TDA1543     |
|                      D482445    (QFP144)                                        |
|4218160                                      16MHz         MB87078               |
|           TCO780FPA  D482445           30.4761MHz                               |
|           (QFP240)              ENSONIC                                         |
|E23-28.18                        OTISR2                                          |
|                                                                                 |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.
      CY7B991 : Programmable Skew Clock Buffer (PLCC32)
      4218160 : 2M x8 / 1M x16 DRAM
      424210  : 256K x16 DRAM
      424260  : 256K x16 DRAM
      43256   : 32K x8 SRAM
      D482445 : DRAM?
      LC321664: 64K x16 DRAM
      TC51832 : 32K x8 SRAM


Bottom board: JCG DAUGHTERL PCB-C K9100633A J9100434A (Sticker K91J0633A)
|---------------------------------------------------------------------------------|
|                                                                                 |
|                                                                                 |
|                    E23-23.30  E23-24.31                                         |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|  E23-01.5   E23-08.18   E23-15.32                                               |
|                                                                                 |
|  E23-02.6   E23-09.19   E23-16.33                                               |
|                                                                                 |
|  E23-03.7   E23-10.20   E23-17.34                     E17-23.65                 |
|                                                                                 |
|  E23-04.8   E23-11.21   E23-18.35                     6264                      |
|                                                                                 |
|  E23-05.9   E23-12.22   E23-19.36                        MC68HC11M0             |
|                                                          (QFP80)                |
|  E23-06.10  E23-13.23   E23-20.37                                               |
|                                                                                 |
|  *          *           E23-21.38                                               |
|                                                          E23-37.69              |
|  E23-07.12  E23-14.25   E23-22.39                              SMC_COM20020I    |
|                                                                                 |
|                                                                                 |
|                                                      E23-38.73                  |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|                                                                                 |
|---------------------------------------------------------------------------------|

Notes:
      All 3K files are PALs type PALCE 16V8 and saved in Jedec format.
      6264: 8K x8 SRAM
      SMC_COM20020I: Network communmication IC
      MC68HC11: Microcontroller with 64k internal ROM capability (not read but probably protected anyway ;-)
      ROMs .36-.39 are 27C4001, main program.
      ROMs .5-.12, .18-.25 are 16M MASK, graphics.
      ROMs .32-.35 are 16M MASK, sound data.
      ROMs .30-.31 are 27C2001, sound program.
      ROM  .65 is 27C512, linked to 68HC11 MCU
      *    Unpopulated socket.

*/

#include "driver.h"
#include "machine/random.h"
#include "taito_f3.h"

/* from Machine.c */
READ16_HANDLER(f3_68000_share_r);
WRITE16_HANDLER(f3_68000_share_w);
READ16_HANDLER(f3_68681_r);
WRITE16_HANDLER(f3_68681_w);
READ16_HANDLER(es5510_dsp_r);
WRITE16_HANDLER(es5510_dsp_w);
WRITE16_HANDLER(f3_volume_w);
WRITE16_HANDLER(f3_es5505_bank_w);
void f3_68681_reset(void);

data32_t *jc_unknown_ram;
data32_t *jc_unknown_ram2;
data32_t *jc_unknown_ram3;
data32_t *jc_unknown_ram4;
static int taitojc_ramgfx;
data32_t *jc_weirdram;

static struct GfxLayout jc_ram_layout =
{
	16,16,
	0x2000,
	4,
	{ 0,1,2,3 },
	{ 24,28,16,20,8,12,0,4, 56,60,48,52,40,44,32,36 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};


VIDEO_START( taitojc )
{
	/* if in doubt steal code from other drivers ;-) */
	int gfx_index=0;

	/* allocate RAM for characters */
//	jc_unknown_ram = auto_malloc(0x00100000);

 	/* find first empty slot to decode gfx */
	for (gfx_index = 0; gfx_index < MAX_GFX_ELEMENTS; gfx_index++)
		if (Machine->gfx[gfx_index] == 0)
			break;

	if (gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[gfx_index] = decodegfx((UINT8 *)jc_unknown_ram,&jc_ram_layout); // can't use RGN_FRAC ?
		if (!Machine->gfx[gfx_index])
			return 1;

	/* set the color information */
	Machine->gfx[gfx_index]->colortable = Machine->remapped_colortable;
	Machine->gfx[gfx_index]->total_colors = 1;	// is this correct ?
	taitojc_ramgfx = gfx_index;

	return 0;

}
VIDEO_UPDATE( taitojc )
{
	if ( keyboard_pressed_memory(KEYCODE_M) )
	{
		{
			FILE *fp;

			fp=fopen("jc_unknown_ram", "w+b");
			if (fp)
			{
				fwrite(jc_unknown_ram, 0x0100000, 1, fp);
				fclose(fp);
			}
		}
		{
			FILE *fp;

			fp=fopen("jc_unknown_ram2", "w+b");
			if (fp)
			{
				fwrite(jc_unknown_ram2, 0x010000, 1, fp);
				fclose(fp);
			}
		}
		{
			FILE *fp;

			fp=fopen("jc_unknown_ram3", "w+b");
			if (fp)
			{
				fwrite(jc_unknown_ram3, 0x0100000, 1, fp);
				fclose(fp);
			}
		}
		{
			FILE *fp;

			fp=fopen("jc_unknown_ram4", "w+b");
			if (fp)
			{
				fwrite(jc_unknown_ram4, 0x02000, 1, fp);
				fclose(fp);
			}
		}

	}

	/* just decode all the character ram, not optimized, not fast, but for now who gives a damn? */
	{
		int numchar;
		for (numchar = 0; numchar < 0x2000;numchar++)
				decodechar(Machine->gfx[taitojc_ramgfx],numchar,
					(UINT8 *)jc_unknown_ram,&jc_ram_layout);
	}

	{
		int x,y;
		const struct GfxElement *gfx = Machine->gfx[0];
		int gfx_shift = 2;

		for (y = 0; y < 64 ; y++) {
			for (x = 0; x < 64 ; x++) {

				UINT32 data;
				int tileno;

				data = jc_unknown_ram[((0xf8000)/4) + x + (y*64)];

				tileno = (data>>gfx_shift) &0x7f;
				tileno += 0x1f80;

				drawgfx(bitmap,gfx,tileno,0,0,0,x*16,y*16,cliprect,TRANSPARENCY_PEN,0);

			}

		}


	}



}

READ32_HANDLER ( jc_unknown_r )
{
	return 0xffffffff;
//	return mame_rand() ^ (mame_rand()<<16);
}

READ32_HANDLER ( jc_unk2_r )
{
	return 0xffffffff;
}

static ADDRESS_MAP_START( taitojc_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_READ(MRA32_ROM)
	AM_RANGE(0x04000000, 0x040fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x05900000, 0x0590ffff) AM_READ(MRA32_RAM) AM_BASE(&jc_weirdram)
//	AM_RANGE(0x05900004, 0x05900007) AM_READ(jc_unknown_r) // ?
	AM_RANGE(0x06400000, 0x0640ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x06600000, 0x06600003) AM_READ(jc_unk2_r)
	AM_RANGE(0x08000000, 0x080fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x10000000, 0x10001fff) AM_READ(MRA32_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( taitojc_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x04000000, 0x040fffff) AM_WRITE(MWA32_RAM) AM_BASE(&jc_unknown_ram)// jc_unknown_ram contains characters
	AM_RANGE(0x05900000, 0x0590ffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x06400000, 0x0640ffff) AM_WRITE(MWA32_RAM) AM_BASE(&jc_unknown_ram2)
	AM_RANGE(0x06600000, 0x06600003) AM_WRITE(MWA32_NOP) // ?
	AM_RANGE(0x08000000, 0x080fffff) AM_WRITE(MWA32_RAM) AM_BASE(&jc_unknown_ram3)
	AM_RANGE(0x10000000, 0x10001fff) AM_WRITE(MWA32_RAM) AM_BASE(&jc_unknown_ram4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x140fff) AM_READ(f3_68000_share_r)
	AM_RANGE(0x200000, 0x20001f) AM_READ(ES5505_data_0_r)
	AM_RANGE(0x260000, 0x2601ff) AM_READ(es5510_dsp_r)
	AM_RANGE(0x280000, 0x28001f) AM_READ(f3_68681_r)
	AM_RANGE(0xc00000, 0xc1ffff) AM_READ(MRA16_BANK1)
	AM_RANGE(0xc20000, 0xc3ffff) AM_READ(MRA16_BANK2)
	AM_RANGE(0xc40000, 0xc7ffff) AM_READ(MRA16_BANK3)
	AM_RANGE(0xff8000, 0xffffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x140000, 0x140fff) AM_WRITE(f3_68000_share_w)
	AM_RANGE(0x200000, 0x20001f) AM_WRITE(ES5505_data_0_w)
	AM_RANGE(0x260000, 0x2601ff) AM_WRITE(es5510_dsp_w)
	AM_RANGE(0x280000, 0x28001f) AM_WRITE(f3_68681_w)
	AM_RANGE(0x300000, 0x30003f) AM_WRITE(f3_es5505_bank_w)
	AM_RANGE(0x340000, 0x340003) AM_WRITE(f3_volume_w) /* 8 channel volume control */
	AM_RANGE(0xc00000, 0xc7ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0xff8000, 0xffffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

INPUT_PORTS_START( taitojc )
INPUT_PORTS_END





static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static MACHINE_INIT( jc )
{
	/* Sound cpu program loads to 0xc00000 so we use a bank */
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU2);
	cpu_setbank(1,&RAM[0x80000]);
	cpu_setbank(2,&RAM[0x90000]);
	cpu_setbank(3,&RAM[0xa0000]);

	RAM[0]=RAM[0x80000]; /* Stack and Reset vectors */
	RAM[1]=RAM[0x80001];
	RAM[2]=RAM[0x80002];
	RAM[3]=RAM[0x80003];

	cpu_set_reset_line(1, ASSERT_LINE);
// do not let the 68k start up until f3_shared_ram points to valid RAM
//	cpu_set_reset_line(1, CLEAR_LINE);
	f3_68681_reset();

	// Code assumes RAM at 05800000 is initalized to all FF, but then tests it
	// like normal RAM later.  Weird.
	memset(jc_weirdram, 0xff, 0xffff);
}

static struct ES5505interface es5505_interface =
{
	1,					/* total number of chips */
	{ 30476100/2 },		/* freq */
	{ REGION_SOUND1 },	/* Bank 0: Unused by F3 games? */
	{ REGION_SOUND1 },	/* Bank 1: All games seem to use this */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },		/* master volume */
};

static MACHINE_DRIVER_START( taitojc )
	MDRV_CPU_ADD(M68020, 25000000) // 68040 !!!
	MDRV_CPU_PROGRAM_MAP(taitojc_readmem,taitojc_writemem)
//	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_MACHINE_INIT(jc)

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 64*16)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 0*16, 32*16-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(taitojc)
	MDRV_VIDEO_UPDATE(taitojc)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5505, es5505_interface)
MACHINE_DRIVER_END

ROM_START( sidebs )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e23-19.036", 0x000000, 0x80000, CRC(7b75481b) SHA1(47332e045f92b31e4f35c38e6880a7287b9a5c2c) )
	ROM_LOAD32_BYTE("e23-20.037", 0x000001, 0x80000, CRC(cbd857dd) SHA1(ae33ad8b0c3559a3a9096351e9aa07782d3cb841) )
	ROM_LOAD32_BYTE("e23-21.038", 0x000002, 0x80000, CRC(357f2e10) SHA1(226922f2649d9ac78d253200f5bbff4fb3ac74c8) )
	ROM_LOAD32_BYTE("e23-22.039", 0x000003, 0x80000, CRC(c793ba43) SHA1(0ddbf625320968b4e18309d8e732ce4a2b9f4bce) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e23-23.030",  0x000001, 0x040000, CRC(cffbffe5) SHA1(c01ac44390dacab4b49bb066a46d81a184b07a1e) )
	ROM_LOAD16_BYTE( "e23-24.031",  0x000000, 0x040000, CRC(64bae246) SHA1(f929f664881487615b1259db43a0721135830274) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e23-01.005",  0x000000, 0x200000, CRC(c5018242) SHA1(d1a45aa6899d9a95b32aeba0f9a4520714d5f5a3) )
	ROM_LOAD( "e23-02.006",  0x000000, 0x200000, CRC(dbf24766) SHA1(2ee0bdb9f912f31bdf962a1af53ad1a6e3121a05) )
	ROM_LOAD( "e23-03.007",  0x000000, 0x200000, CRC(3ab59dd0) SHA1(39e93d306a59017bef0f2fdadc69f96aecbffca6) )
	ROM_LOAD( "e23-04.008",  0x000000, 0x200000, CRC(5060119c) SHA1(c8c5c33bd37121d222961ecfcdb79bbe6cbdf821) )
	ROM_LOAD( "e23-05.009",  0x000000, 0x200000, CRC(9aa0d956) SHA1(bd146e3db7c2a53701eca6b43f903899ed15623a) )
	ROM_LOAD( "e23-06.010",  0x000000, 0x200000, CRC(2077ac3a) SHA1(894e1bbfef8a693bda50aa8681e8f283c0d6b6cf) )
	ROM_LOAD( "e23-07.012",  0x000000, 0x200000, CRC(bf1cd99d) SHA1(f98e1dacb485c7e40b0b2277a690d74ae020c9d9) )
	ROM_LOAD( "e23-08.018",  0x000000, 0x200000, CRC(04738f5b) SHA1(0ebc8c61d87f10822148f3b272612dc10ac4652c) )
	ROM_LOAD( "e23-09.019",  0x000000, 0x200000, CRC(089fac53) SHA1(3af008c65f4b60ae221b90fdb52d4e6fe552e5a8) )
	ROM_LOAD( "e23-10.020",  0x000000, 0x200000, CRC(dccb19ed) SHA1(35749c823ad9a30f471367750c087435b678b42f) )
	ROM_LOAD( "e23-11.021",  0x000000, 0x200000, CRC(6981a46f) SHA1(262bf821275413c70a1a8374185df64c697d8b04) )
	ROM_LOAD( "e23-12.022",  0x000000, 0x200000, CRC(c93fab09) SHA1(1ed292b11bf6bf09d1783d247f3de0c899e4df28) )
	ROM_LOAD( "e23-13.023",  0x000000, 0x200000, CRC(985f6785) SHA1(81cd33957b943f4bb33323fbf491ace344f8245b) )
	ROM_LOAD( "e23-14.025",  0x000000, 0x200000, CRC(9d54eac7) SHA1(4f97b388a0fd0529d26e1cc31491a0e2ce6c381d) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e23-15.32",  0x000000, 0x200000, CRC(8955b7c7) SHA1(767626bd5cf6810b0368ee85e487c12ef7e8a23d))
	ROM_LOAD16_BYTE( "e23-16.33",  0x400000, 0x200000, CRC(1d63d785) SHA1(0cf74bb433e9c453e35f7a552fdf9e22084b2f49) )
	ROM_LOAD16_BYTE( "e23-17.34",  0x800000, 0x200000, CRC(1c54021a) SHA1(a1efbdb02d23a5d32ebd25cb8e99dface3178ebd) )
	ROM_LOAD16_BYTE( "e23-18.35",  0xc00000, 0x200000, CRC(1816f38a) SHA1(6451bdb4b4297aaf4987451bc0ddd97b0072e113) )
ROM_END

ROM_START( sidebs2 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("sbs2_p0.36", 0x000000, 0x80000, CRC(2dd78d09) SHA1(f0a0105c3f2827c8b55d1bc58ebeea0f71150fed) )
	ROM_LOAD32_BYTE("sbs2_p1.37", 0x000001, 0x80000, CRC(befeda1d) SHA1(3171c87b0872f3206653900e3dbd210ea9beba61) )
	ROM_LOAD32_BYTE("sbs2_p2.38", 0x000002, 0x80000, CRC(ade07d7e) SHA1(a5200ea3ddbfef37d302e7cb27015b6f6aa8a7c1) )
	ROM_LOAD32_BYTE("sbs2_p3.39", 0x000003, 0x80000, CRC(94e943d6) SHA1(2bc7332526b969e5084b9d73063f1c0d18ec5181) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e38-19.30", 0x000001, 0x040000, CRC(3f50cb7b) SHA1(76af65c9b74ede843a3182f79cecda8c3e3febe6) )
	ROM_LOAD16_BYTE( "e38-20.31", 0x000000, 0x040000, CRC(d01340e7) SHA1(76ee48d644dc1ec415d47e0df4864c64ac928b9d) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.65",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */
	ROM_LOAD( "93c46.87",  0x000000, 0x000080, CRC(14e5526c) SHA1(ae02bd8f1eff41738043931642c652732fbe3801) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e38-01.5",  0x000000, 0x200000, CRC(a3c2e2c7) SHA1(538208534f996782167e4cf0d157ad93ce2937bd) )
	ROM_LOAD( "e38-02.6",  0x000000, 0x200000, CRC(ecdfb75a) SHA1(85e7afa321846816fa3bd9074ad9dec95abe23fe) )
	ROM_LOAD( "e38-03.7",  0x000000, 0x200000, CRC(28e9cb59) SHA1(a2651fd81a1263573f868864ee049f8fc4177ffa) )
	ROM_LOAD( "e38-04.8",  0x000000, 0x080000, CRC(26cab05b) SHA1(d5bcf021646dade834840051e0ce083319c53985) )
	ROM_LOAD( "e38-05.9",  0x000000, 0x200000, CRC(bda366bf) SHA1(a7558e6d5e4583a2d8e3d2bfa8cabcc459d3ee83) )
	ROM_LOAD( "e38-06.10", 0x000000, 0x200000, CRC(308d2783) SHA1(22c309273444bc6c1df78069850958a739664998) )
	ROM_LOAD( "e38-07.11", 0x000000, 0x200000, CRC(226ba93d) SHA1(98e6342d070ddd988c1e9bff21afcfb28df86844) )
	ROM_LOAD( "e38-08.12", 0x000000, 0x200000, CRC(9c513b32) SHA1(fe26e39d3d65073d23d525bc17771f0c244a38c2) )
	ROM_LOAD( "e38-09.18", 0x000000, 0x200000, CRC(03c95a7f) SHA1(fc046cf5fcfcf5648f68af4bed78576f6d67b32f) )
	ROM_LOAD( "e38-10.19", 0x000000, 0x200000, CRC(0fb06794) SHA1(2d0e3b07ded698235572fe9e907a84d5779ac2c5) )
	ROM_LOAD( "e38-11.20", 0x000000, 0x200000, CRC(6a312351) SHA1(8037e377f8eef4cc6dd84aec9c829106f0bb130c) )
	ROM_LOAD( "e38-12.21", 0x000000, 0x080000, CRC(193a5774) SHA1(aa017ae4fec92bb87c0eb94f59d093853ddbabc9) )
	ROM_LOAD( "e38-13.22", 0x000000, 0x200000, CRC(1bd7582b) SHA1(35763b9489f995433f66ef72d4f6b6ac67df5480) )
	ROM_LOAD( "e38-14.23", 0x000000, 0x200000, CRC(f1699f27) SHA1(3c9a9cefe6f215fd9f0a9746da97147d14df1da4) )
	ROM_LOAD( "e38-15.24", 0x000000, 0x200000, CRC(2853c2e3) SHA1(046dbbd4bcb3b07cddab19a284fee9fe736f8def) )
	ROM_LOAD( "e38-16.25", 0x000000, 0x200000, CRC(fceafae2) SHA1(540ecd5d1aa64c0428a08ea1e4e634e00f7e6bd6) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e23-15.32", 0x000000, 0x200000, CRC(8955b7c7) SHA1(767626bd5cf6810b0368ee85e487c12ef7e8a23d) ) // from sidebs (redump)
	ROM_LOAD16_BYTE( "e38-17.33", 0x400000, 0x200000, CRC(61e81c7f) SHA1(aa650bc139685ad456c233b79aa60005a8fd6d79) )
	ROM_LOAD16_BYTE( "e38-18.34", 0x800000, 0x200000, CRC(43e2f149) SHA1(32ea9156948a886dda5bd071e9f493ddc2b06212) )
	ROM_LOAD16_BYTE( "e38-21.35", 0xc00000, 0x200000, CRC(25373c5f) SHA1(ab9f917dbde7c808be2cd836ce2d3fc558e290f1) )

	/* PALS
	e23-28.18    NOT A ROM
	e23-27.13    NOT A ROM
	e23-26.4     NOT A ROM
	e23-25-1.3   NOT A ROM
	e23-30.40    NOT A ROM
	e23-29.39    NOT A ROM
	e23-31.46    NOT A ROM
	e23-32-1.51  NOT A ROM
	e23-34.72    NOT A ROM
	e23-33.53    NOT A ROM
	e23-35.110   NOT A ROM
	e23-38.73    NOT A ROM
	e23-37.69    NOT A ROM
	*/
ROM_END

ROM_START( dendeg )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e35-21.036", 0x000000, 0x80000, CRC(bc70ca97) SHA1(724a24da9d6f163c26e7528ee2c15bd06f2c4382) )
	ROM_LOAD32_BYTE("e35-22.037", 0x000001, 0x80000, CRC(83b17de8) SHA1(538ddc16727e08e9a2a8ff6b4f030dc044993aa0) )
	ROM_LOAD32_BYTE("e35-23.038", 0x000002, 0x80000, CRC(1da4acd6) SHA1(2ce11c5f37287526bb1d39185f793d79fc73d5b5) )
	ROM_LOAD32_BYTE("e35-24.039", 0x000003, 0x80000, CRC(0318afb0) SHA1(9c86330c85536fb1a093ed40610b1c3ddb7813c3) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e35-25.030",  0x100001, 0x040000, CRC(8104de13) SHA1(e518fbaf91704cf5cb8ffbb4833e3adba8c18658) )
	ROM_LOAD16_BYTE( "e35-26.031",  0x100000, 0x040000, CRC(61821cc9) SHA1(87cd5bd3bb22c9f4ca4b6d96f75434d48418321b) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e35-01.005",  0x000000, 0x200000, CRC(bd1975cb) SHA1(a08c6f4a84f9d4c2a5aa67cc2045aedd4580b8dc) )
	ROM_LOAD( "e35-02.006",  0x000000, 0x200000, CRC(e5caa459) SHA1(c38d795b96fff193223cd3df9f51ebdc2971b719) )
	ROM_LOAD( "e35-03.007",  0x000000, 0x200000, CRC(86ea5bcf) SHA1(1cee7f677b786b2fa9f50e723decd08cd69fbdef) )
	ROM_LOAD( "e35-04.008",  0x000000, 0x200000, CRC(afc07c36) SHA1(f3f7b04766a9a2c8b298e1692aea24abc7001432) )
	ROM_LOAD( "e35-05.009",  0x000000, 0x200000, CRC(a94486c5) SHA1(c3f869aa0557411f747038a1e0ed6eedcf91fda5) )
	ROM_LOAD( "e35-06.010",  0x000000, 0x200000, CRC(cf328021) SHA1(709ce80f9338637172dbf4e9adcacdb3e5b4ccdb) )
	ROM_LOAD( "e35-07.011",  0x000000, 0x200000, CRC(a88a5a86) SHA1(a4a393fe9df3654e41d32e015ca3977d13206dfe) )
	ROM_LOAD( "e35-08.012",  0x000000, 0x200000, CRC(99425ff6) SHA1(3bd6fe7204dece55459392170b42d4c6a9d3ef5b) )
	ROM_LOAD( "e35-09.018",  0x000000, 0x200000, CRC(be18ddf1) SHA1(d4fe26e427244c5b421b6421ff3bf9af303673d5) )
	ROM_LOAD( "e35-10.019",  0x000000, 0x200000, CRC(44ea9474) SHA1(161ff94b31c3dc2fa4b1e556ed62624147687443) )
	ROM_LOAD( "e35-11.020",  0x000000, 0x200000, CRC(dc8f5e88) SHA1(e311252db8a7232a5325a3eff5c1890d20bd3f8f) )
	ROM_LOAD( "e35-12.021",  0x000000, 0x200000, CRC(039b604c) SHA1(7e394e7cddc6bf42f3834d5331203e8496597a90) )
	ROM_LOAD( "e35-13.022",  0x000000, 0x200000, CRC(2dc9dff1) SHA1(bc7ad64bc359f18a065e36749cc29c75e52202e2) )
	ROM_LOAD( "e35-14.023",  0x000000, 0x200000, CRC(cab50364) SHA1(e3272f844ecadfd63a1e3c965526a851f8cde94d) )
	ROM_LOAD( "e35-15.024",  0x000000, 0x200000, CRC(aea86b35) SHA1(092f34f844fc6a779a6f18c03d44a9d39a101373) )
	ROM_LOAD( "e35-16.025",  0x000000, 0x200000, CRC(161481b6) SHA1(cc3c2939ac8911c197e9930580d316066f345772) )
	ROM_LOAD( "e35-28.trn",  0x000000, 0x040000, CRC(d1b571c1) SHA1(cac7d3f0285544fe36b8b744edfbac0190cdecab) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e35-17.032",  0x000000, 0x200000, CRC(3bc23aa1) SHA1(621e0f2f5f3dbd7d7ce05c9cd317958c361c1c26) )
	ROM_LOAD16_BYTE( "e35-18.033",  0x400000, 0x200000, CRC(a084d3aa) SHA1(18ea39366006e362e16d6732ce3e79cd3bfa041e) )
	ROM_LOAD16_BYTE( "e35-19.034",  0x800000, 0x200000, CRC(ebe2dcef) SHA1(16ae41e0f3bb242cbc2922f53cacbd99961a3f97) )
	ROM_LOAD16_BYTE( "e35-20.035",  0xc00000, 0x200000, CRC(a1d4b30d) SHA1(e02f613b93d3b3ee1eb23f5b7f62c5448ed3966d) )
ROM_END

ROM_START( dendegx )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e35-30.036", 0x000000, 0x80000, CRC(57ee0975) SHA1(c7741a7e0e9c1fdebc6b942587d7ac5a6f26f66d) )//ex
	ROM_LOAD32_BYTE("e35-31.037", 0x000001, 0x80000, CRC(bd5f2651) SHA1(73b760df351170ace019e4b61c82d8c6296a3632) )//ex
	ROM_LOAD32_BYTE("e35-32.038", 0x000002, 0x80000, CRC(66be29d5) SHA1(e73937f5bda709a606d5cdf7316b26051317c22f) )//ex
	ROM_LOAD32_BYTE("e35-33.039", 0x000003, 0x80000, CRC(76a6bde2) SHA1(ca456ec3f0410777362e3eb977ae156866271bd5) )//ex

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e35-25.030",  0x100000, 0x040000, CRC(8104de13) SHA1(e518fbaf91704cf5cb8ffbb4833e3adba8c18658) )
	ROM_LOAD16_BYTE( "e35-26.031",  0x100001, 0x040000, CRC(61821cc9) SHA1(87cd5bd3bb22c9f4ca4b6d96f75434d48418321b) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e35-01.005",  0x000000, 0x200000, CRC(bd1975cb) SHA1(a08c6f4a84f9d4c2a5aa67cc2045aedd4580b8dc) )
	ROM_LOAD( "e35-02.006",  0x000000, 0x200000, CRC(e5caa459) SHA1(c38d795b96fff193223cd3df9f51ebdc2971b719) )
	ROM_LOAD( "e35-03.007",  0x000000, 0x200000, CRC(86ea5bcf) SHA1(1cee7f677b786b2fa9f50e723decd08cd69fbdef) )
	ROM_LOAD( "e35-04.008",  0x000000, 0x200000, CRC(afc07c36) SHA1(f3f7b04766a9a2c8b298e1692aea24abc7001432) )
	ROM_LOAD( "e35-05.009",  0x000000, 0x200000, CRC(a94486c5) SHA1(c3f869aa0557411f747038a1e0ed6eedcf91fda5) )
	ROM_LOAD( "e35-06.010",  0x000000, 0x200000, CRC(cf328021) SHA1(709ce80f9338637172dbf4e9adcacdb3e5b4ccdb) )
	ROM_LOAD( "e35-07.011",  0x000000, 0x200000, CRC(a88a5a86) SHA1(a4a393fe9df3654e41d32e015ca3977d13206dfe) )
	ROM_LOAD( "e35-08.012",  0x000000, 0x200000, CRC(99425ff6) SHA1(3bd6fe7204dece55459392170b42d4c6a9d3ef5b) )
	ROM_LOAD( "e35-09.018",  0x000000, 0x200000, CRC(be18ddf1) SHA1(d4fe26e427244c5b421b6421ff3bf9af303673d5) )
	ROM_LOAD( "e35-10.019",  0x000000, 0x200000, CRC(44ea9474) SHA1(161ff94b31c3dc2fa4b1e556ed62624147687443) )
	ROM_LOAD( "e35-11.020",  0x000000, 0x200000, CRC(dc8f5e88) SHA1(e311252db8a7232a5325a3eff5c1890d20bd3f8f) )
	ROM_LOAD( "e35-12.021",  0x000000, 0x200000, CRC(039b604c) SHA1(7e394e7cddc6bf42f3834d5331203e8496597a90) )
	ROM_LOAD( "e35-13.022",  0x000000, 0x200000, CRC(2dc9dff1) SHA1(bc7ad64bc359f18a065e36749cc29c75e52202e2) )
	ROM_LOAD( "e35-14.023",  0x000000, 0x200000, CRC(cab50364) SHA1(e3272f844ecadfd63a1e3c965526a851f8cde94d) )
	ROM_LOAD( "e35-15.024",  0x000000, 0x200000, CRC(aea86b35) SHA1(092f34f844fc6a779a6f18c03d44a9d39a101373) )
	ROM_LOAD( "e35-16.025",  0x000000, 0x200000, CRC(161481b6) SHA1(cc3c2939ac8911c197e9930580d316066f345772) )
	ROM_LOAD( "e35-28.trn",  0x000000, 0x040000, CRC(d1b571c1) SHA1(cac7d3f0285544fe36b8b744edfbac0190cdecab) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e35-17.032",  0x000000, 0x200000, CRC(3bc23aa1) SHA1(621e0f2f5f3dbd7d7ce05c9cd317958c361c1c26) )
	ROM_LOAD16_BYTE( "e35-18.033",  0x400000, 0x200000, CRC(a084d3aa) SHA1(18ea39366006e362e16d6732ce3e79cd3bfa041e) )
	ROM_LOAD16_BYTE( "e35-19.034",  0x800000, 0x200000, CRC(ebe2dcef) SHA1(16ae41e0f3bb242cbc2922f53cacbd99961a3f97) )
	ROM_LOAD16_BYTE( "e35-20.035",  0xc00000, 0x200000, CRC(a1d4b30d) SHA1(e02f613b93d3b3ee1eb23f5b7f62c5448ed3966d) )
ROM_END

ROM_START( dendeg2 )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e52-25-1.036", 0x000000, 0x80000, CRC(fadf5b4c) SHA1(48f3e1425bb9552d472a2720e1c9a752db2b43ed) )
	ROM_LOAD32_BYTE("e52-26-1.037", 0x000001, 0x80000, CRC(7cf5230d) SHA1(b3416886d7cfc88520f6bf378529086bf0095db5) )
	ROM_LOAD32_BYTE("e52-27-1.038", 0x000002, 0x80000, CRC(25f0d81d) SHA1(c33c3e6b1ad49b63b31a2f1227d43141faef4eab) )
	ROM_LOAD32_BYTE("e52-28-1.039", 0x000003, 0x80000, CRC(e76ff6a1) SHA1(674c00f19df034de8134d48a8c2d2e42f7eb1be7) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e52-29.030",  0x100001, 0x040000, CRC(6010162a) SHA1(f14920b26887f5387b3e261b63573d850195982a) )
	ROM_LOAD16_BYTE( "e52-30.031",  0x100000, 0x040000, CRC(2881af4a) SHA1(5918f6508b3cd3bef3751e3bda2a48152569c1cd) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e52-01.005",  0x000000, 0x200000, CRC(8db39c3c) SHA1(74b3305ebdf679ae274c73b7b32d2adea602bedc) )
	ROM_LOAD( "e52-02.006",  0x000000, 0x200000, CRC(b8d6f066) SHA1(99553ad66643ebf7fc71a9aee526d8f206b41dcc) )
	ROM_LOAD( "e52-03.007",  0x000000, 0x200000, CRC(a37d164b) SHA1(767a7d2de8b91a00c5fe74710937457e8568a422) )
	ROM_LOAD( "e52-04.008",  0x000000, 0x200000, CRC(403a4142) SHA1(aa5fea79a7a838642152586689d0f9b33311252c) )
	ROM_LOAD( "e52-05.009",  0x000000, 0x200000, CRC(1ad0c612) SHA1(4ffc373fca8c1e1a5edbad3305b08f0867e9809c) )
	ROM_LOAD( "e52-06.010",  0x000000, 0x200000, CRC(aa35e536) SHA1(2c1b2ee0d2587db6d6dd60b081bfcef3bb0dd9fa) )
	ROM_LOAD( "e52-07.011",  0x000000, 0x200000, CRC(80a27984) SHA1(57b8c41809de09582600b8ff30c135d5abd044b8) )
	ROM_LOAD( "e52-08.012",  0x000000, 0x200000, CRC(d52e6b9c) SHA1(382a5fd4533ab641a09321208464d83f72e161e3) )
	ROM_LOAD( "e52-09.018",  0x000000, 0x200000, CRC(830e0a3c) SHA1(ab96f380e53f72945f73a6cfcc80d12e1b1afce8) )
	ROM_LOAD( "e52-10.019",  0x000000, 0x200000, CRC(671e41c6) SHA1(281899d83dba104daf7d7c2bd0834cab4c022472) )
	ROM_LOAD( "e52-11.020",  0x000000, 0x200000, CRC(1bc22680) SHA1(1f71db88d6df3b4bdf090b77bc83a67906bb31da) )
	ROM_LOAD( "e52-12.021",  0x000000, 0x200000, CRC(a8bb91c5) SHA1(959a9fedb7839e1e4e7658d920bd5da4fd8cae48) )
	ROM_LOAD( "e52-13.022",  0x000000, 0x200000, CRC(943af3f4) SHA1(bfc81aa5e5c22e44601428b9e980f09d0c65e38e) )
	ROM_LOAD( "e52-14.023",  0x000000, 0x200000, CRC(f311a9b4) SHA1(1f25571ac9468d453e92886e003400fffdc149f2) )
	ROM_LOAD( "e52-15.024",  0x000000, 0x200000, CRC(b45a6199) SHA1(9339a1520b38d1f6538171a0b01df87de3e9c2d1) )
	ROM_LOAD( "e52-16.025",  0x000000, 0x200000, CRC(db6dd6e2) SHA1(d345dbd745514d4777d52c4360787ea8c462ffb1) )
	ROM_LOAD( "e52-17.052",  0x000000, 0x200000, CRC(4ac11921) SHA1(c4816e1d68bb52ee59e7a2e6de617c1093020944) )
	ROM_LOAD( "e52-18.053",  0x000000, 0x200000, CRC(7f3e4af7) SHA1(ab35744014175a802e73c8b70de4e7508f0a1cd1) )
	ROM_LOAD( "e52-19.054",  0x000000, 0x200000, CRC(2e5ff408) SHA1(91f95721b98198082e950c50f33324820719e9ed) )
	ROM_LOAD( "e52-20.055",  0x000000, 0x200000, CRC(e90eb71e) SHA1(f07518c718f773e20412393c0ebb3243f9b1d96c) )
	ROM_LOAD( "e35-28.trn",  0x000000, 0x040000, CRC(d1b571c1) SHA1(cac7d3f0285544fe36b8b744edfbac0190cdecab) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e52-21.032",  0x000000, 0x200000, CRC(ba58081d) SHA1(bcb6c8781191d48f906ed404a3e7388097a64781) )
	ROM_LOAD16_BYTE( "e52-22.033",  0x400000, 0x200000, CRC(dda281b1) SHA1(4851a6bf7902548c5033090a0e5c15f74c00ef58) )
	ROM_LOAD16_BYTE( "e52-23.034",  0x800000, 0x200000, CRC(ebe2dcef) SHA1(16ae41e0f3bb242cbc2922f53cacbd99961a3f97) ) // same as e35-19.034 from dendeg
	ROM_LOAD16_BYTE( "e52-24.035",  0xc00000, 0x200000, CRC(a9a678da) SHA1(b980ae644ef0312acd63b017028af9bf2b084c29) )
ROM_END

ROM_START( dendeg2x )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e52-35.036", 0x000000, 0x80000, CRC(d5b33eb8) SHA1(e05ad73986741827b7bbeac72af0a8324384bf6b) ) //2ex
	ROM_LOAD32_BYTE("e52-36.037", 0x000001, 0x80000, CRC(f3f3fabd) SHA1(4f88080091af2208d671c491284d992b5036908c) ) //2ex
	ROM_LOAD32_BYTE("e52-37.038", 0x000002, 0x80000, CRC(65b8ef31) SHA1(b61b391b160e81715ff355aeef65026d7e4dd9af) ) //2ex
	ROM_LOAD32_BYTE("e52-38.039", 0x000003, 0x80000, CRC(cf61f321) SHA1(c8493d2499afba673174b26044aca537e384916c) ) //2ex

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e52-29.030",  0x100001, 0x040000, CRC(6010162a) SHA1(f14920b26887f5387b3e261b63573d850195982a) )
	ROM_LOAD16_BYTE( "e52-30.031",  0x100000, 0x040000, CRC(2881af4a) SHA1(5918f6508b3cd3bef3751e3bda2a48152569c1cd) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e52-01.005",  0x000000, 0x200000, CRC(8db39c3c) SHA1(74b3305ebdf679ae274c73b7b32d2adea602bedc) )
	ROM_LOAD( "e52-02.006",  0x000000, 0x200000, CRC(b8d6f066) SHA1(99553ad66643ebf7fc71a9aee526d8f206b41dcc) )
	ROM_LOAD( "e52-03.007",  0x000000, 0x200000, CRC(a37d164b) SHA1(767a7d2de8b91a00c5fe74710937457e8568a422) )
	ROM_LOAD( "e52-04.008",  0x000000, 0x200000, CRC(403a4142) SHA1(aa5fea79a7a838642152586689d0f9b33311252c) )
	ROM_LOAD( "e52-05.009",  0x000000, 0x200000, CRC(1ad0c612) SHA1(4ffc373fca8c1e1a5edbad3305b08f0867e9809c) )
	ROM_LOAD( "e52-31.010",  0x000000, 0x200000, CRC(e9e2eb3d) SHA1(97e2dc907faa512d3fb140dafe3250f04991ff07) ) //2ex
	ROM_LOAD( "e52-32.011",  0x000000, 0x200000, CRC(43d452fd) SHA1(20a4064904cf2f21d5f03236c99c9e21a49a1206) ) //2ex
	ROM_LOAD( "e52-08.012",  0x000000, 0x200000, CRC(d52e6b9c) SHA1(382a5fd4533ab641a09321208464d83f72e161e3) )
	ROM_LOAD( "e52-09.018",  0x000000, 0x200000, CRC(830e0a3c) SHA1(ab96f380e53f72945f73a6cfcc80d12e1b1afce8) )
	ROM_LOAD( "e52-10.019",  0x000000, 0x200000, CRC(671e41c6) SHA1(281899d83dba104daf7d7c2bd0834cab4c022472) )
	ROM_LOAD( "e52-11.020",  0x000000, 0x200000, CRC(1bc22680) SHA1(1f71db88d6df3b4bdf090b77bc83a67906bb31da) )
	ROM_LOAD( "e52-12.021",  0x000000, 0x200000, CRC(a8bb91c5) SHA1(959a9fedb7839e1e4e7658d920bd5da4fd8cae48) )
	ROM_LOAD( "e52-13.022",  0x000000, 0x200000, CRC(943af3f4) SHA1(bfc81aa5e5c22e44601428b9e980f09d0c65e38e) )
	ROM_LOAD( "e52-33.023",  0x000000, 0x200000, CRC(6906f41f) SHA1(0d3f6fc4772417190d123ad38b0b8a8a532159c6) ) //2ex
	ROM_LOAD( "e52-34.024",  0x000000, 0x200000, CRC(4ae1064b) SHA1(a5f1ad3262f8ffd09e9063a6dbe98d883b11a156) ) //2ex
	ROM_LOAD( "e52-16.025",  0x000000, 0x200000, CRC(db6dd6e2) SHA1(d345dbd745514d4777d52c4360787ea8c462ffb1) )
	ROM_LOAD( "e52-17.052",  0x000000, 0x200000, CRC(4ac11921) SHA1(c4816e1d68bb52ee59e7a2e6de617c1093020944) )
	ROM_LOAD( "e52-18.053",  0x000000, 0x200000, CRC(7f3e4af7) SHA1(ab35744014175a802e73c8b70de4e7508f0a1cd1) )
	ROM_LOAD( "e52-19.054",  0x000000, 0x200000, CRC(2e5ff408) SHA1(91f95721b98198082e950c50f33324820719e9ed) )
	ROM_LOAD( "e52-20.055",  0x000000, 0x200000, CRC(e90eb71e) SHA1(f07518c718f773e20412393c0ebb3243f9b1d96c) )
	ROM_LOAD( "e35-28.trn",  0x000000, 0x040000, CRC(d1b571c1) SHA1(cac7d3f0285544fe36b8b744edfbac0190cdecab) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e52-21.032",  0x000000, 0x200000, CRC(ba58081d) SHA1(bcb6c8781191d48f906ed404a3e7388097a64781) )
	ROM_LOAD16_BYTE( "e52-22.033",  0x400000, 0x200000, CRC(dda281b1) SHA1(4851a6bf7902548c5033090a0e5c15f74c00ef58) )
	ROM_LOAD16_BYTE( "e52-23.034",  0x800000, 0x200000, CRC(ebe2dcef) SHA1(16ae41e0f3bb242cbc2922f53cacbd99961a3f97) ) // same as e35-19.034 from dendeg
	ROM_LOAD16_BYTE( "e52-24.035",  0xc00000, 0x200000, CRC(a9a678da) SHA1(b980ae644ef0312acd63b017028af9bf2b084c29) )
ROM_END

ROM_START( landgear )
	ROM_REGION(0x200000, REGION_CPU1, 0) /* 68040 code */
 	ROM_LOAD32_BYTE("e17-37.36", 0x000000, 0x80000, CRC(e6dda113) SHA1(786cbfae420b6ee820a93731e59da3442245b6b8) )
	ROM_LOAD32_BYTE("e17-38.37", 0x000001, 0x80000, CRC(86fa29bd) SHA1(f711528143c042cdc4a26d9e6965a882a73f397c) )
	ROM_LOAD32_BYTE("e17-39.38", 0x000002, 0x80000, CRC(ccbbcc7b) SHA1(52d91fcaa1683d2679ed4f14ebc11dc487527898) )
	ROM_LOAD32_BYTE("e17-40.39", 0x000003, 0x80000, CRC(ce9231d2) SHA1(d2c3955d910dbd0cac95862047c58791af626722) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 Code? */
	ROM_LOAD16_BYTE( "e17-21.30",  0x100001, 0x040000, CRC(8b54f46c) SHA1(c6d16197ab7768945becf9b49b6d286113b4d1cc) )
	ROM_LOAD16_BYTE( "e17-22.31",  0x100000, 0x040000, CRC(b96f6cd7) SHA1(0bf086e5dc6d524cd00e33df3e3d2a8b9231eb72) )

	ROM_REGION( 0x010000, REGION_USER1, 0 ) /* MC68HC11M0 related? */
	ROM_LOAD( "e17-23.065",  0x000000, 0x010000, CRC(80ac1428) SHA1(5a2a1e60a11ecdb8743c20ddacfb61f9fd00f01c) )

	ROM_REGION( 0x00080, REGION_USER2, 0 ) /* eeprom */

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* other roms, not sorted */
	ROM_LOAD( "e17-01.5",   0x000000, 0x200000, CRC(42aa56a6) SHA1(945c338515ceb946c01480919546869bb8c3d323) )
	ROM_LOAD( "e17-02.8",   0x000000, 0x200000, CRC(df7e2405) SHA1(684d6fc398791c48101e6cb63acbf0d691ed863c) )
	ROM_LOAD( "e17-03.9",   0x000000, 0x200000, CRC(64820c4f) SHA1(ee18e4e2b01ec21c33ec1f0eb43f6d0cd48d7225) )
	ROM_LOAD( "e17-04.10",  0x000000, 0x200000, CRC(7dc2cae3) SHA1(90638a1efc353428ce4155ca29f67accaf0499cd) )
	ROM_LOAD( "e17-05.11",  0x000000, 0x200000, CRC(3f70acd4) SHA1(e8c1c6214631e3e39d1fc9df13d1862442a47e5d) )
	ROM_LOAD( "e17-06.12",  0x000000, 0x200000, CRC(107ff481) SHA1(2a48cedec9641ff08776e5d8b1bf1f5b250d4179) )
	ROM_LOAD( "e17-07.18",  0x000000, 0x200000, CRC(0f180eb0) SHA1(5e1dd920f110a62a029bace6f4cb80fee0fdaf03) )
	ROM_LOAD( "e17-08.19",  0x000000, 0x200000, CRC(3107e154) SHA1(59a99770c2aa535cac6569f41b03be1554e0e800) )
	ROM_LOAD( "e17-09.22",  0x000000, 0x200000, CRC(19e9a1d1) SHA1(26f1a91e3757da510d685a11add08e3e00317796) )
	ROM_LOAD( "e17-10.23",  0x000000, 0x200000, CRC(a6bdf6b8) SHA1(e8d76d38f2c7e428a3c2f555571e314351d74a69) )
	ROM_LOAD( "e17-11.24",  0x000000, 0x200000, CRC(4e986d93) SHA1(b218a0360c1d0eca5a907f2b402f352e0329fe41) )
	ROM_LOAD( "e17-12.25",  0x000000, 0x200000, CRC(0727ddfa) SHA1(68bf83a3c46cd042a7ad27a530c8bed6360d8492) )

	ROM_REGION16_BE( 0x1000000, REGION_SOUND1, ROMREGION_ERASE00  )
	ROM_LOAD16_BYTE( "e17-13.32",  0x000000, 0x200000, CRC(6cf238e7) SHA1(0745d2dcfea26178adde3ad08650156e8e30651f) )
	ROM_LOAD16_BYTE( "e17-14.33",  0x400000, 0x200000, CRC(5efec311) SHA1(f253bc40f2567f59ddfb617fddb8b9a389bfac89) )
	ROM_LOAD16_BYTE( "e17-15.34",  0x800000, 0x200000, CRC(41d7a7d0) SHA1(f5a8b79c1d47611e93d46aaf921107b52090bb5f) )
	ROM_LOAD16_BYTE( "e17-16.35",  0xc00000, 0x200000, CRC(6cf9f277) SHA1(03ca51fadc6b0b6502804346f18eeb55ab87b0e7) )

	/* Pals
	e07-02.4     NOT A ROM
	e07-03.50    NOT A ROM
	e07-04.115   NOT A ROM
	e07-05.22    NOT A ROM
	e07-06.37    NOT A ROM
	e07-07.49    NOT A ROM
	e07-08.65    NOT A ROM
	e07-09.82    NOT A ROM
	e07-10.116   NOT A ROM
	e09-21.69    NOT A ROM
	e09-22.73    NOT A ROM
	e17-32.96    NOT A ROM
	*/
ROM_END




GAMEX( 1997, dendeg,  0,      taitojc, taitojc, 0, ROT0, "Taito", "Densya De Go", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, dendegx, dendeg, taitojc, taitojc, 0, ROT0, "Taito", "Densya De Go Ex", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, dendeg2, 0,      taitojc, taitojc, 0, ROT0, "Taito", "Densya De Go 2", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, dendeg2x,dendeg2,taitojc, taitojc, 0, ROT0, "Taito", "Densya De Go 2 Ex", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, sidebs,  0,      taitojc, taitojc, 0, ROT0, "Taito", "Side By Side", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, sidebs2, 0,      taitojc, taitojc, 0, ROT0, "Taito", "Side By Side 2", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1997, landgear,0,      taitojc, taitojc, 0, ROT0, "Taito", "Landing Gear", GAME_NOT_WORKING|GAME_NO_SOUND )