/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 40
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)


struct blstroid_mo_data
{
	int *sprite_map;
	int *redraw_list, *redraw;
};



/*************************************
 *
 *		Globals we own
 *
 *************************************/



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;

static void *int1_timer[32];

static unsigned long priority[8];



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void blstroid_vh_stop (void);
void blstroid_sound_reset (void);
void blstroid_update_display_list (int scanline);

#if 0
static int blstroid_debug (void);
#endif


/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int blstroid_vh_start(void)
{
	static struct atarigen_modesc blstroid_modesc =
	{
		256,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		2, 3, 0x1ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		blstroid_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (XDIM, YDIM, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		blstroid_vh_stop ();
		return 1;
	}

	/* reset the timers */
	memset (int1_timer, 0, sizeof (int1_timer));

	/* initialize the palette remapping */
	atarigen_init_remap (COLOR_PALETTE_555, 0);

	/* initialize the displaylist system */
	return atarigen_init_display_list (&blstroid_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void blstroid_vh_stop (void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty)
		free (playfielddirty);
	playfielddirty = 0;
}



/*************************************
 *
 *		Scan line interrupt handlers
 *
 *************************************/

void blstroid_int1off_callback (int param)
{
	/* clear the interrupt generated as well */
	cpu_clear_pending_interrupts (0);
}


void blstroid_int1_callback (int param)
{
	/* generate the interrupt */
	cpu_cause_interrupt (0, 1);

	/* set ourselves up to go off next frame */
	int1_timer[param] = timer_set (TIME_IN_HZ (Machine->drv->frames_per_second), param, blstroid_int1_callback);
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int blstroid_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void blstroid_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_playfieldram[offset], newword);

		playfielddirty[offset / 2] = 1;

		/* modifying an interrupt state? */
		if ((offset & 0x7f) == 0x50)
		{
			int row = (offset >> 7) & 0x1f;

			if ((newword & 0x8000) && !int1_timer[row])
				int1_timer[row] = timer_set (cpu_getscanlinetime (8 * row), row, blstroid_int1_callback);
			else if (!(newword & 0x8000) && int1_timer[row])
			{
				timer_remove (int1_timer[row]);
				int1_timer[row] = 0;
			}
		}
	}
}



/*************************************
 *
 *		Priority RAM write handler
 *
 *************************************/

void blstroid_priorityram_w (int offset, int data)
{
	int shift, which;

	/* pick which playfield palette to look at */
	which = (offset >> 5) & 7;

	/* upper 16 bits are for H == 1, lower 16 for H == 0 */
	shift = (offset >> 4) & 0x10;
	shift += (offset >> 1) & 0x0f;

	/* set or clear the appropriate bit */
	priority[which] = (priority[which] & ~(1 << shift)) | ((data & 1) << shift);
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void blstroid_update_display_list (int scanline)
{
	atarigen_update_display_list (atarigen_spriteram, 0, scanline);
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used total
 *
 *		Word 1: Vertical position
 *
 *			Bits 0-3   = vertical size of the object, in tiles
 *			Bits 7-15  = vertical position
 *
 *		Word 2: Image
 *
 *			Bits 0-13  = image index
 *			Bit  14    = vertical flip
 *			Bit  15    = horizontal flip
 *
 *		Word 3: Link
 *
 *			Bits 3-11  = link to the next motion object
 *
 *		Word 4: Horizontal position
 *
 *			Bits 0-3   = motion object palette
 *			Bits 6-15  = horizontal position
 *
 *---------------------------------------------------------------------------------
 */

void blstroid_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct blstroid_mo_data *modata = param;
	int *redraw_list = modata->redraw_list;
	int *redraw = modata->redraw;
	int *r, redraw_val;

	/* extract data from the various words */
	int pict = data[1] & 0x3fff;
	int vsize = (data[0] & 15) + 1;
	int xpos = data[3] >> 7;
	int ypos = 256 - ((data[0] >> 7) - 256) - vsize * 8;
	int color = data[3] & 15;
	int hflip = data[1] & 0x8000;
	int vflip = data[1] & 0x4000;
	int y, sy, yadv;

	/* adjust for v flip */
	if (vflip)
		ypos += (vsize - 1) * 8, yadv = -8;
	else
		yadv = 8;

	/* keep them in range to start */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	redraw_val = (xpos << 23) + (ypos << 14) + vsize;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* clip the X coordinate */
	if (xpos <= -8 || xpos >= XDIM)
		return;

	/* add an entry to the redraw list for later */
	for (r = redraw_list; r < redraw; )
		if (*r++ == redraw_val)
			break;

	/* but only add it if we don't have a matching entry already */
	if (r == redraw)
	{
		*redraw++ = redraw_val;
		modata->redraw = redraw;
	}

	/* loop over the height */
	for (y = 0, sy = ypos; y < vsize; y++, sy += yadv, pict++)
	{
		/* clip the Y coordinate */
		if (sy <= clip->min_y - 8)
			continue;
		else if (sy > clip->max_y)
			break;

		/* if this sprite's color has not been used yet this frame, grab it */
		/* note that by doing it here, we don't map sprites that aren't visible */
		if (!modata->sprite_map[color])
		{
			atarigen_alloc_dynamic_colors (0x000 + 16 * color + 1, 15);
			modata->sprite_map[color] = 1;
		}

		/* draw the sprite */
		drawgfx (bitmap, Machine->gfx[1], pict, color, hflip, vflip,
					xpos, sy, clip, TRANSPARENCY_PEN, 0);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void blstroid_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	struct blstroid_mo_data modata;
	int sprite_map[16], pf_map[16];
	int redraw_list[1024], *r;
	int x, y, offs, i;


/*	int hidebank = blstroid_debug ();*/


	/* reset color tracking */
	for (i = 0; i < 16; i++)
		sprite_map[i] = pf_map[i] = 0;


	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-12  = image number
	 *			Bits 13-15 = palette
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* loop over the visible Y portion */
	for (y = 0; y < YCHARS; y++)
	{
		offs = y * 64;

		/* loop over the visible X portion */
		for (x = 0; x < XCHARS; x++, offs++)
		{
			int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
			int color = data >> 13;

			/* update color statistics */
			pf_map[color] = 1;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				drawgfx (playfieldbitmap, Machine->gfx[0], data & 0x1fff, color, 0, 0,
						8 * x, 8 * y, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* allocate playfield colors */
	atarigen_alloc_fixed_colors (pf_map, 0x100, 16, 16);

	/* copy the playfield to the destination */
	copybitmap (bitmap, playfieldbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* prepare the motion object data structure */
	modata.sprite_map = sprite_map;
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, blstroid_render_mo, &modata);

	/* redraw playfield tiles with higher priority */
	for (r = redraw_list; r < modata.redraw; r++)
	{
		int val = *r;
		int xpos = (val >> 23) & 0x1ff;
		int ypos = (val >> 14) & 0x1ff;
		int h = val & 15;
		struct rectangle clip;
		int y, sx;

		/* wrap */
		if (xpos > XDIM) xpos -= 0x200;
		if (ypos > YDIM) ypos -= 0x200;

		/* make a clip */
		clip.min_x = xpos;
		clip.max_x = xpos + 7;
		clip.min_y = ypos;
		clip.max_y = ypos + h * 8 - 1;

		/* round the positions */
		xpos /= 8;
		ypos /= 8;

		/* loop over the columns */
		for (x = xpos + 1; x >= xpos; x--)
		{
			/* compute the scroll-adjusted x position */
			sx = (x * 8) & 0x1ff;
			if (sx > 0x1f8) sx -= 0x200;

			/* loop over the rows */
			for (y = ypos + h; y >= ypos; y--)
			{
				int sy, offs, data, color;

				/* compute the scroll-adjusted y position */
				sy = (y * 8) & 0x1ff;
				if (sy > 0x1f8) sy -= 0x200;

				/* process the data */
				offs = (y & 0x3f) * 64 + (x & 0x3f);
				data = READ_WORD (&atarigen_playfieldram[offs * 2]);
				color = data >> 13;

				/* the logic is more complicated than this, but this is close */
				if (!priority[color])
					drawgfx (bitmap, Machine->gfx[0], data & 0x1fff, color, 0, 0,
							sx, sy, &clip, TRANSPARENCY_NONE, 0);
			}
		}
	}

	/* update the active colors */
	atarigen_update_colors (-1);
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#if 0
static int blstroid_debug (void)
{
	static unsigned long oldpri[8];
	int hidebank = -1;

	if (memcmp (oldpri, priority, sizeof (oldpri)))
	{
		static FILE *f;
		int i;
		if (!f) f = fopen ("priority.log", "w");
		for (i = 0; i < 8; i++)
			fprintf (f, "%08lX ", priority[i]);
		fprintf (f, "\n");
		memcpy (oldpri, priority, sizeof (oldpri));
	}

	if (osd_key_pressed (OSD_KEY_Q)) hidebank = 0;
	if (osd_key_pressed (OSD_KEY_W)) hidebank = 1;
	if (osd_key_pressed (OSD_KEY_E)) hidebank = 2;
	if (osd_key_pressed (OSD_KEY_R)) hidebank = 3;
	if (osd_key_pressed (OSD_KEY_T)) hidebank = 4;
	if (osd_key_pressed (OSD_KEY_Y)) hidebank = 5;
	if (osd_key_pressed (OSD_KEY_U)) hidebank = 6;
	if (osd_key_pressed (OSD_KEY_I)) hidebank = 7;

	if (osd_key_pressed (OSD_KEY_A)) hidebank = 8;
	if (osd_key_pressed (OSD_KEY_S)) hidebank = 9;
	if (osd_key_pressed (OSD_KEY_D)) hidebank = 10;
	if (osd_key_pressed (OSD_KEY_F)) hidebank = 11;
	if (osd_key_pressed (OSD_KEY_G)) hidebank = 12;
	if (osd_key_pressed (OSD_KEY_H)) hidebank = 13;
	if (osd_key_pressed (OSD_KEY_J)) hidebank = 14;
	if (osd_key_pressed (OSD_KEY_K)) hidebank = 15;

	if (osd_key_pressed (OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (osd_key_pressed (OSD_KEY_9)) { }

		sprintf (name, "Dump %d", ++count);
		f = fopen (name, "wt");

		fprintf (f, "\n\nMotion Object Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nPlayfield Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Objects\n");
		for (i = 0; i < 0x40; i++)
		{
			fprintf (f, "   Object %02X:  Y=%04X  P=%04X  L=%04X  X=%04X\n",
					i,
					READ_WORD (&atarigen_spriteram[i*8+0]),
					READ_WORD (&atarigen_spriteram[i*8+2]),
					READ_WORD (&atarigen_spriteram[i*8+4]),
					READ_WORD (&atarigen_spriteram[i*8+6])
			);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}

	return hidebank;
}
#endif