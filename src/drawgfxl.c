/* don't put this file in the makefile, it is #included by common.c to */
/* generate 8-bit and 16-bit versions                                  */

DECLARE(blockmove_opaque,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata),
{
	DATA_TYPE *end;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata <= end - 8)
		{
			dstdata[0] = paldata[srcdata[0]];
			dstdata[1] = paldata[srcdata[1]];
			dstdata[2] = paldata[srcdata[2]];
			dstdata[3] = paldata[srcdata[3]];
			dstdata[4] = paldata[srcdata[4]];
			dstdata[5] = paldata[srcdata[5]];
			dstdata[6] = paldata[srcdata[6]];
			dstdata[7] = paldata[srcdata[7]];
			dstdata += 8;
			srcdata += 8;
		}
		while (dstdata < end)
			*(dstdata++) = paldata[*(srcdata++)];

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_opaque_flipx,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata),
{
	DATA_TYPE *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata <= end - 8)
		{
			srcdata -= 8;
			dstdata[0] = paldata[srcdata[8]];
			dstdata[1] = paldata[srcdata[7]];
			dstdata[2] = paldata[srcdata[6]];
			dstdata[3] = paldata[srcdata[5]];
			dstdata[4] = paldata[srcdata[4]];
			dstdata[5] = paldata[srcdata[3]];
			dstdata[6] = paldata[srcdata[2]];
			dstdata[7] = paldata[srcdata[1]];
			dstdata += 8;
		}
		while (dstdata < end)
			*(dstdata++) = paldata[*(srcdata--)];

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


DECLARE(blockmove_transpen,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transpen),
{
	DATA_TYPE *end;
	int trans4;
	UINT32 *sd4;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	trans4 = transpen * 0x01010101;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = *(srcdata++);
			if (col != transpen) *dstdata = paldata[col];
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			UINT32 col4;

			if ((col4 = *(sd4++)) != trans4)
			{
				UINT32 xod4;

				xod4 = col4 ^ trans4;
				if (xod4 & 0x000000ff) dstdata[BL0] = paldata[(col4) & 0xff];
				if (xod4 & 0x0000ff00) dstdata[BL1] = paldata[(col4 >>  8) & 0xff];
				if (xod4 & 0x00ff0000) dstdata[BL2] = paldata[(col4 >> 16) & 0xff];
				if (xod4 & 0xff000000) dstdata[BL3] = paldata[col4 >> 24];
			}
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = *(srcdata++);
			if (col != transpen) *dstdata = paldata[col];
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_transpen_flipx,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transpen),
{
	DATA_TYPE *end;
	int trans4;
	UINT32 *sd4;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;
	srcdata -= 3;

	trans4 = transpen * 0x01010101;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (col != transpen) *dstdata = paldata[col];
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			UINT32 col4;

			if ((col4 = *(sd4--)) != trans4)
			{
				UINT32 xod4;

				xod4 = col4 ^ trans4;
				if (xod4 & 0xff000000) dstdata[BL0] = paldata[col4 >> 24];
				if (xod4 & 0x00ff0000) dstdata[BL1] = paldata[(col4 >> 16) & 0xff];
				if (xod4 & 0x0000ff00) dstdata[BL2] = paldata[(col4 >>  8) & 0xff];
				if (xod4 & 0x000000ff) dstdata[BL3] = paldata[col4 & 0xff];
			}
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (col != transpen) *dstdata = paldata[col];
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


#define PEN_IS_OPAQUE ((1<<col)&transmask) == 0

DECLARE(blockmove_transmask,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transmask),
{
	DATA_TYPE *end;
	UINT32 *sd4;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = *(srcdata++);
			if (PEN_IS_OPAQUE) *dstdata = paldata[col];
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			int col;
			UINT32 col4;

			col4 = *(sd4++);
			col = (col4 >>  0) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL0] = paldata[col];
			col = (col4 >>  8) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL1] = paldata[col];
			col = (col4 >> 16) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL2] = paldata[col];
			col = (col4 >> 24) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL3] = paldata[col];
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = *(srcdata++);
			if (PEN_IS_OPAQUE) *dstdata = paldata[col];
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_transmask_flipx,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transmask),
{
	DATA_TYPE *end;
	UINT32 *sd4;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;
	srcdata -= 3;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (((long)srcdata & 3) && dstdata < end)	/* longword align */
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (PEN_IS_OPAQUE) *dstdata = paldata[col];
			dstdata++;
		}
		sd4 = (UINT32 *)srcdata;
		while (dstdata <= end - 4)
		{
			int col;
			UINT32 col4;

			col4 = *(sd4--);
			col = (col4 >> 24) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL0] = paldata[col];
			col = (col4 >> 16) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL1] = paldata[col];
			col = (col4 >>  8) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL2] = paldata[col];
			col = (col4 >>  0) & 0xff;
			if (PEN_IS_OPAQUE) dstdata[BL3] = paldata[col];
			dstdata += 4;
		}
		srcdata = (unsigned char *)sd4;
		while (dstdata < end)
		{
			int col;

			col = srcdata[3];
			srcdata--;
			if (PEN_IS_OPAQUE) *dstdata = paldata[col];
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


DECLARE(blockmove_transcolor,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transcolor),
{
	DATA_TYPE *end;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			int col;

			col = paldata[*(srcdata++)];
			if (col != transcolor) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_transcolor_flipx,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transcolor),
{
	DATA_TYPE *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			int col;

			col = paldata[*(srcdata--)];
			if (col != transcolor) *dstdata = col;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


DECLARE(blockmove_transthrough,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transcolor),
{
	DATA_TYPE *end;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			if (*dstdata == transcolor) *dstdata = paldata[*srcdata];
			srcdata++;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_transthrough_flipx,(
		const UINT8 *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		const unsigned short *paldata,int transcolor),
{
	DATA_TYPE *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			if (*dstdata == transcolor) *dstdata = paldata[*srcdata];
			srcdata--;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


DECLARE(blockmove_opaque_noremap,(
		const DATA_TYPE *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo),
{
	while (srcheight)
	{
		memcpy(dstdata,srcdata,srcwidth * sizeof(DATA_TYPE));
		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_opaque_noremap_flipx,(
		const DATA_TYPE *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo),
{
	DATA_TYPE *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata <= end - 8)
		{
			srcdata -= 8;
			dstdata[0] = srcdata[8];
			dstdata[1] = srcdata[7];
			dstdata[2] = srcdata[6];
			dstdata[3] = srcdata[5];
			dstdata[4] = srcdata[4];
			dstdata[5] = srcdata[3];
			dstdata[6] = srcdata[2];
			dstdata[7] = srcdata[1];
			dstdata += 8;
		}
		while (dstdata < end)
			*(dstdata++) = *(srcdata--);

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})


DECLARE(blockmove_transthrough_noremap,(
		const DATA_TYPE *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		int transcolor),
{
	DATA_TYPE *end;

	srcmodulo -= srcwidth;
	dstmodulo -= srcwidth;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			if (*dstdata == transcolor) *dstdata = *srcdata;
			srcdata++;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})

DECLARE(blockmove_transthrough_noremap_flipx,(
		const DATA_TYPE *srcdata,int srcwidth,int srcheight,int srcmodulo,
		DATA_TYPE *dstdata,int dstmodulo,
		int transcolor),
{
	DATA_TYPE *end;

	srcmodulo += srcwidth;
	dstmodulo -= srcwidth;
	srcdata += srcwidth-1;

	while (srcheight)
	{
		end = dstdata + srcwidth;
		while (dstdata < end)
		{
			if (*dstdata == transcolor) *dstdata = *srcdata;
			srcdata--;
			dstdata++;
		}

		srcdata += srcmodulo;
		dstdata += dstmodulo;
		srcheight--;
	}
})



DECLARE(drawgfx_core,(
		struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color),
{
	int ox;
	int oy;
	int ex;
	int ey;


	/* check bounds */
	ox = sx;
	oy = sy;

	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;

	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	{
		UINT8 *sd = gfx->gfxdata + code * gfx->char_modulo;		/* source data */
		int sw = ex-sx+1;										/* source width */
		int sh = ey-sy+1;										/* source height */
		int sm = gfx->line_modulo;								/* source modulo */
		DATA_TYPE *dd = ((DATA_TYPE *)dest->line[sy]) + sx;		/* dest data */
		int dm = ((DATA_TYPE *)dest->line[1])-((DATA_TYPE *)dest->line[0]);	/* dest modulo */
		const unsigned short *paldata = &gfx->colortable[gfx->color_granularity * color];

		if (flipx)
		{
			if ((sx-ox) == 0) sd += gfx->width - sw;
		}
		else
			sd += (sx-ox);

		if (flipy)
		{
			if ((sy-oy) == 0) sd += sm * (gfx->height - sh);
			dd += dm * (sh - 1);
			dm = -dm;
		}
		else
			sd += sm * (sy-oy);

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				BLOCKMOVE(opaque,flipx,(sd,sw,sh,sm,dd,dm,paldata));
				break;

			case TRANSPARENCY_PEN:
				BLOCKMOVE(transpen,flipx,(sd,sw,sh,sm,dd,dm,paldata,transparent_color));
				break;

			case TRANSPARENCY_PENS:
				BLOCKMOVE(transmask,flipx,(sd,sw,sh,sm,dd,dm,paldata,transparent_color));
				break;

			case TRANSPARENCY_COLOR:
				BLOCKMOVE(transcolor,flipx,(sd,sw,sh,sm,dd,dm,paldata,transparent_color));
				break;

			case TRANSPARENCY_THROUGH:
				BLOCKMOVE(transthrough,flipx,(sd,sw,sh,sm,dd,dm,paldata,transparent_color));
				break;
		}
	}
})



DECLARE(copybitmap_core,(
		struct osd_bitmap *dest,struct osd_bitmap *src,
		int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color),
{
	int ox;
	int oy;
	int ex;
	int ey;


	/* check bounds */
	ox = sx;
	oy = sy;

	ex = sx + src->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;

	ey = sy + src->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	{
		DATA_TYPE *sd = ((DATA_TYPE *)src->line[0]);							/* source data */
		int sw = ex-sx+1;														/* source width */
		int sh = ey-sy+1;														/* source height */
		int sm = ((DATA_TYPE *)src->line[1])-((DATA_TYPE *)src->line[0]);		/* source modulo */
		DATA_TYPE *dd = ((DATA_TYPE *)dest->line[sy]) + sx;						/* dest data */
		int dm = ((DATA_TYPE *)dest->line[1])-((DATA_TYPE *)dest->line[0]);		/* dest modulo */

		if (flipx)
		{
			if ((sx-ox) == 0) sd += src->width - sw;
		}
		else
			sd += (sx-ox);

		if (flipy)
		{
			if ((sy-oy) == 0) sd += sm * (src->height - sh);
			dd += dm * (sh - 1);
			dm = -dm;
		}
		else
			sd += sm * (sy-oy);

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				BLOCKMOVE(opaque_noremap,flipx,(sd,sw,sh,sm,dd,dm));
				break;

			case TRANSPARENCY_PEN:
			case TRANSPARENCY_COLOR:
				BLOCKMOVE(transpen_noremap,flipx,(sd,sw,sh,sm,dd,dm,transparent_color));
				break;

			case TRANSPARENCY_THROUGH:
				BLOCKMOVE(transthrough_noremap,flipx,(sd,sw,sh,sm,dd,dm,transparent_color));
				break;
		}
	}
})