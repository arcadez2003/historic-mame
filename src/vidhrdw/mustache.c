/***************************************************************************

	Mustache Boy
	(c)1987 March Electronics

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap;
static int control_byte;

static const int bgcols[16][8][3]=
{

 {{0x00,0x00,0x00},
 {0xdf,0xdf,0x00},
 {0xf0,0xf0,0x00},
 {0xff,0xff,0x00},
 {0xbf,0xbf,0xbf},
 {0xd0,0xd0,0xe0},
 {0xe0,0xe0,0xd0},
 {0xff,0xff,0xff}},

 {{0x7f,0x00,0x00},
 {0xbf,0x00,0x00},
 {0xff,0x1f,0x1f},
 {0xff,0x7f,0x7f},
 {0x33,0x8f,0xff},
 {0x00,0xaa,0xff},
 {0x69,0xbf,0xff},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x55,0x2a,0x00},
 {0xaa,0x55,0x00},
 {0xff,0x7f,0x00},
 {0x00,0xbf,0xbf},
 {0x3f,0xff,0xff},
 {0x7f,0xff,0xff},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x00,0x55,0x10},
 {0x00,0xaa,0x20},
 {0x00,0xff,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x80,0x60,0x40},
 {0x90,0x70,0x50},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x69,0xbf,0xff},
 {0x00,0xaa,0xff},
 {0x33,0x8f,0xff},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0xbf,0x00,0xbf},
 {0xff,0x00,0xff},
 {0xff,0xbf,0xff},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x7f,0x7f,0x7f},
 {0x3f,0x3f,0x3f},
 {0xbf,0xbf,0xbf},
 {0x5f,0x5f,0x5f},
 {0x8f,0x8f,0x8f},
 {0xcf,0xcf,0xcf},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x55,0x00,0x10},
 {0xaa,0x00,0x20},
 {0xff,0x00,0x30},
 {0x40,0x40,0x40},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0xff,0xcf,0xcf},
 {0xff,0xd0,0xd0},
 {0x55,0x55,0x00},
 {0xff,0x00,0x00},
 {0xff,0xca,0xca},
 {0xff,0xd4,0xd4},
 {0xff,0xff,0xff}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}},

 {{0x00,0x00,0x00},
 {0x10,0x10,0x10},
 {0x20,0x20,0x20},
 {0x30,0x30,0x30},
 {0x40,0x40,0x40},
 {0x50,0x50,0x50},
 {0x60,0x60,0x60},
 {0x70,0x70,0x70}}
};


PALETTE_INIT(mustache)
{
	int i,j;
	for(j=0;j<16;j++)
 	 for(i=0;i<8;i++)
	  palette_set_color(j*8+i,bgcols[j][i][0],bgcols[j][i][1],bgcols[j][i][2]);

	for(j=0;j<16;j++)
	 for(i=0;i<16;i++)
	  palette_set_color(128+j*16+i,i*0x10,i*0x10,i*0x10);
}


WRITE_HANDLER (mustache_video_control_w)
{
	if((control_byte^data)&8) /* tile bank  */
	{
		control_byte=data;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( mustache_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap,0,0x100-data);
	tilemap_set_scrollx(bg_tilemap,1,0x100-data);
	tilemap_set_scrollx(bg_tilemap,2,0x100-data);
	tilemap_set_scrollx(bg_tilemap,3,0x100);
}

WRITE_HANDLER( mustache_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
	}
}


static UINT32 scan_rows_flipx( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return row*num_cols + (num_cols-1) - col;
}

static void get_bg_tile_info(int tile_index)
{
	int attr = videoram[2*tile_index+1];
	int code = videoram[2*tile_index] + ((attr&0xe0)<<3)+ ( (control_byte&8) << 7);

	SET_TILE_INFO(
			0,
			code,
			attr & 0xf,0)
}


VIDEO_START( mustache )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,scan_rows_flipx,TILEMAP_OPAQUE,8,8,64,32);
	tilemap_set_scroll_rows(bg_tilemap,4);

	return 0;
}

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	struct rectangle clip = *cliprect;
	const struct GfxElement *gfx = Machine->gfx[1];
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sy = 240-spriteram[offs];
		int sx = 240-spriteram[offs+3];
		int code = spriteram[offs+2];
		int attr = spriteram[offs+1];
		int color = (attr & 0xf0)>>4;

		if (sy == 240) continue;

		code+=(attr&0x0c)<<6;

		if((control_byte & 0xa))
			clip.max_y = Machine->visible_area.max_y;
		else
			clip.max_y = Machine->visible_area.max_y - 56;


		drawgfx(bitmap,gfx,
				code,
				color,
				0,0,
				sx,sy,
				&clip,TRANSPARENCY_PEN,0);

	}
}


VIDEO_UPDATE( mustache )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}


