//
// fh_modey.c : FastHeretic Mode Y (planar 320x200, 3 pages)
//
// The 3D view is drawn straight into a hidden video page, the status bar,
// border and menus are copied or drawn on top, then the page is flipped.
// This saves the full view copy that Mode 13h does every frame, and in
// low and potato detail one byte write covers 2 or 4 pixels.
//
// Based on FastDoom's Mode Y code (Victor Nieto, GPL).
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#ifdef FH_MODEY

#include <string.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include "doomdef.h"
#include "r_local.h"
#include "r_fh.h"

#define SC_INDEX   0x3C4
#define SC_DATA    0x3C5
#define GC_INDEX   0x3CE
#define GC_DATA    0x3CF
#define CRTC_INDEX 0x3D4

#define PAGEBYTES  0x4000

extern byte *destscreen, *destview, *pcscreen;
extern int screenblocks;
extern byte *tinttable;

byte *my_ylookup[SCREENHEIGHT];  // view rows in the page being drawn
int my_ofs[SCREENWIDTH];         // byte offset of each view column
byte my_mask[SCREENWIDTH];       // map mask of each view column
byte my_rplane[SCREENWIDTH];     // plane to read back for translucency
byte my_curmask;                 // map mask the card has now

byte *fh_planar;                 // set: V_Draw* draw into this page
int fh_directframe;              // this frame's view went straight to VRAM

static int borderpages = 3;      // pages that still need the border copied
static int sbpages = 3;          // pages that still need the status bar
static int lastdirect;
int fhy_bc, fhy_sc;

#define SETMASK(m) \
	if ((m) != my_curmask) \
	{ \
		my_curmask = (m); \
		outp(SC_DATA, my_curmask); \
	}

#define READPLANE(p) outp(GC_DATA, (p))

//==========================================================================
// Setup
//==========================================================================

void FHY_InitGraphics(void)
{
	union REGS regs;

	regs.w.ax = 0x13;
	int386(0x10, &regs, &regs);
	pcscreen = (byte *)0xA0000;
	destscreen = (byte *)0xA4000;

	outp(SC_INDEX, 4);                          // memory mode: unchain
	outp(SC_DATA, (inp(SC_DATA) & ~8) | 4);
	outp(GC_INDEX, 5);                          // graphics mode
	outp(GC_DATA, inp(GC_DATA) & ~0x13);
	outp(GC_INDEX, 6);                          // misc
	outp(GC_DATA, inp(GC_DATA) & ~2);
	outpw(SC_INDEX, 0x0f02);                    // map mask, all planes
	memset(pcscreen, 0, 0x10000);
	outp(CRTC_INDEX, 0x14);                     // underline: byte mode off
	outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) & ~0x40);
	outp(CRTC_INDEX, 0x17);                     // mode control: byte mode
	outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) | 0x40);
	outp(GC_INDEX, 4);                          // leave read map selected
	my_curmask = 15;
	borderpages = sbpages = 3;
}

// Called whenever the view size or detail changes.
void R_FHY_InitTables(void)
{
	int x, px;

	for (x = 0; x < viewwidth; x++)
	{
		px = viewwindowx + (x << detailshift);
		my_ofs[x] = px >> 2;
		switch (detailshift)
		{
		case 0:
			my_mask[x] = 1 << (px & 3);
			my_rplane[x] = px & 3;
			break;
		case 1:
			my_mask[x] = 3 << (px & 3);
			my_rplane[x] = px & 3;
			break;
		default:
			my_mask[x] = 15;
			my_rplane[x] = 0;
			break;
		}
	}
	borderpages = sbpages = 3;
}

// Called once per frame, after destscreen has moved to the next page.
void R_FHY_SetupFrame(void)
{
	int i;
	byte *row;

	row = destscreen + viewwindowy * 80;
	for (i = 0; i < viewheight; i++, row += 80)
		my_ylookup[i] = row;
}

//==========================================================================
// Column drawers
//==========================================================================

#define YCOL_SETUP \
	int count; byte *dest; fixed_t frac, fracstep; \
	count = dc_yh - dc_yl; \
	if (count < 0) return; \
	SETMASK(my_mask[dc_x]); \
	dest = my_ylookup[dc_yl] + my_ofs[dc_x]; \
	fracstep = dc_iscale; \
	frac = dc_texturemid + (dc_yl - centery) * fracstep;

void R_DrawColumnY(void)
{
	byte *source, *colormap;
	YCOL_SETUP
	source = dc_source;
	colormap = dc_colormap;
	do
	{
		*dest = colormap[source[(frac >> FRACBITS) & 127]];
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawTranslatedColumnY(void)
{
	YCOL_SETUP
	do
	{
		*dest = dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]];
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawFuzzColumnY(void)
{
	YCOL_SETUP
	READPLANE(my_rplane[dc_x]);
	do
	{
		*dest = tinttable[((*dest) << 8)
			+ dc_colormap[dc_source[(frac >> FRACBITS) & 127]]];
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawTranslatedFuzzColumnY(void)
{
	YCOL_SETUP
	READPLANE(my_rplane[dc_x]);
	do
	{
		*dest = tinttable[((*dest) << 8)
			+ dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]]];
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawSaturnColumnY(void)
{
	int odd;
	YCOL_SETUP
	odd = (dc_x + dc_yl) & 1;
	do
	{
		if (odd)
			*dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];
		odd ^= 1;
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawSaturnTransColumnY(void)
{
	int odd;
	YCOL_SETUP
	odd = (dc_x + dc_yl) & 1;
	do
	{
		if (odd)
			*dest = dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]];
		odd ^= 1;
		dest += 80;
		frac += fracstep;
	} while (count--);
}

void R_DrawSkyColumnY(void)
{
	int count; byte *dest; byte *source;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	SETMASK(my_mask[dc_x]);
	dest = my_ylookup[dc_yl] + my_ofs[dc_x];
	source = dc_source + (dc_texturemid >> FRACBITS) + (dc_yl - centery);
	do
	{
		*dest = *source++;
		dest += 80;
	} while (count--);
}

void R_DrawSkyFlatY(void)
{
	int count; byte *dest; byte c;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	SETMASK(my_mask[dc_x]);
	dest = my_ylookup[dc_yl] + my_ofs[dc_x];
	c = skyflatcolor;
	do
	{
		*dest = c;
		dest += 80;
	} while (count--);
}

//==========================================================================
// Span drawers
//
// A span is drawn one plane at a time: 4 passes in high detail, 2 in low
// and 1 in potato. Each pass writes consecutive bytes.
//==========================================================================

byte *ysp_dest;
int ysp_count;
unsigned ysp_pos, ysp_step;

void R_DrawSpanYCore(void);     // fhasmy.asm

void R_DrawSpanY(void)
{
	int passes, k, u;
	fixed_t xfrac, yfrac, xstep, ystep;

	passes = 4 >> detailshift;
	xstep = ds_xstep * passes;
	ystep = ds_ystep * passes;
	ysp_step = ((xstep << 10) & 0xffff0000) | ((ystep >> 6) & 0xffff);
	xfrac = ds_xfrac;
	yfrac = ds_yfrac;
	for (k = 0; k < passes; k++)
	{
		u = ds_x1 + k;
		if (u > ds_x2)
			break;
		SETMASK(my_mask[u]);
		ysp_dest = my_ylookup[ds_y] + my_ofs[u];
		ysp_count = (ds_x2 - u) / passes + 1;
		ysp_pos = ((xfrac << 10) & 0xffff0000) | ((yfrac >> 6) & 0xffff);
		R_DrawSpanYCore();
		xfrac += ds_xstep;
		yfrac += ds_ystep;
	}
}

// Fill screen pixels px1..px2 of a page row with one colour
static void FHY_HLine(byte *row, int px1, int px2, byte c)
{
	int g1, g2;
	byte lmask, rmask;

	g1 = px1 >> 2;
	g2 = px2 >> 2;
	lmask = (0x0f << (px1 & 3)) & 0x0f;
	rmask = 0x0f >> (3 - (px2 & 3));
	if (g1 == g2)
	{
		SETMASK(lmask & rmask);
		row[g1] = c;
		return;
	}
	SETMASK(lmask);
	row[g1] = c;
	if (g2 - g1 > 1)
	{
		SETMASK(15);
		memset(row + g1 + 1, c, g2 - g1 - 1);
	}
	SETMASK(rmask);
	row[g2] = c;
}

void R_DrawSpanFlatY(void)
{
	FHY_HLine(my_ylookup[ds_y],
		viewwindowx + (ds_x1 << detailshift),
		viewwindowx + ((ds_x2 + 1) << detailshift) - 1,
		ds_colormap[ds_flatcolor]);
}

//==========================================================================
// Drawer selection
//==========================================================================

void R_DrawColumnYA(void);      // fhasmy.asm

void R_FHY_SetDrawers(void)
{
	colfunc = basecolfunc = R_DrawColumnYA;
	transcolfunc = R_DrawTranslatedColumnY;
	switch (fh_notrans)
	{
	case 1:
		fuzzcolfunc = R_DrawColumnYA;
		transfuzzcolfunc = R_DrawTranslatedColumnY;
		break;
	case 2:
		fuzzcolfunc = R_DrawSaturnColumnY;
		transfuzzcolfunc = R_DrawSaturnTransColumnY;
		break;
	default:
		fuzzcolfunc = R_DrawFuzzColumnY;
		transfuzzcolfunc = R_DrawTranslatedFuzzColumnY;
		break;
	}
	spanfunc = fh_flatsurfaces ? R_DrawSpanFlatY : R_DrawSpanY;
	skycolfunc = fh_flatsky ? R_DrawSkyFlatY : R_DrawSkyColumnY;
}

//==========================================================================
// Patches drawn straight into the page (menus, messages, fullscreen HUD)
//==========================================================================

// Overlays drawn outside the view, per page: x1, y1, x2, y2 (exclusive)
static int curbox[4] = {SCREENWIDTH, SCREENHEIGHT, 0, 0};
static int pagebox[3][4];

#define PAGENUM(p) (((int)(p) - 0xA0000) / PAGEBYTES)

static void FHY_AddBox(int x1, int y1, int x2, int y2)
{
	if (x2 > SCREENWIDTH) x2 = SCREENWIDTH;
	if (y2 > SCREENHEIGHT) y2 = SCREENHEIGHT;
	if (x1 < curbox[0]) curbox[0] = x1;
	if (y1 < curbox[1]) curbox[1] = y1;
	if (x2 > curbox[2]) curbox[2] = x2;
	if (y2 > curbox[3]) curbox[3] = y2;
}

// mode 0 = solid, 1 = translucent, 2 = solid with shadow
void FHY_DrawPatch(int x, int y, patch_t *patch, int mode)
{
	int col, w, count, px;
	column_t *column;
	byte *source, *dest, *top;

	y -= SHORT(patch->topoffset);
	x -= SHORT(patch->leftoffset);
	w = SHORT(patch->width);
	if (x < 0 || x + w > SCREENWIDTH || y < 0
		|| y + SHORT(patch->height) > SCREENHEIGHT)
	{
		I_Error("Bad V_DrawPatch");
	}
	// anything outside the view must be cleaned off the other pages too
	if (x < viewwindowx || x + w > viewwindowx + scaledviewwidth
		|| y < viewwindowy || y + SHORT(patch->height) + (mode == 2 ? 2 : 0)
		> viewwindowy + viewheight)
	{ // remember it, so this page's border gets cleaned next time round
		FHY_AddBox(x, y, x + w + (mode == 2 ? 2 : 0),
			y + SHORT(patch->height) + (mode == 2 ? 2 : 0));
	}
	for (col = 0; col < w; col++)
	{
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
		px = x + col;
		if (mode == 2 && px + 2 < SCREENWIDTH)
		{ // shadow, 2 pixels right and down
			column_t *c2 = column;
			SETMASK(1 << ((px + 2) & 3));
			READPLANE((px + 2) & 3);
			top = fh_planar + (y + 2) * 80 + ((px + 2) >> 2);
			while (c2->topdelta != 0xff)
			{
				dest = top + c2->topdelta * 80;
				count = c2->length;
				if (y + 2 + c2->topdelta + count > SCREENHEIGHT)
					count = SCREENHEIGHT - (y + 2 + c2->topdelta);
				while (count-- > 0)
				{
					*dest = tinttable[(*dest) << 8];
					dest += 80;
				}
				c2 = (column_t *)((byte *)c2 + c2->length + 4);
			}
		}
		SETMASK(1 << (px & 3));
		if (mode == 1)
			READPLANE(px & 3);
		top = fh_planar + y * 80 + (px >> 2);
		while (column->topdelta != 0xff)
		{
			source = (byte *)column + 3;
			dest = top + column->topdelta * 80;
			count = column->length;
			if (mode == 1)
			{
				while (count--)
				{
					*dest = tinttable[((*dest) << 8) + *source++];
					dest += 80;
				}
			}
			else
			{
				while (count--)
				{
					*dest = *source++;
					dest += 80;
				}
			}
			column = (column_t *)((byte *)column + column->length + 4);
		}
	}
}

//==========================================================================
// Linear screen buffer -> page copies
//==========================================================================

// Copy a rectangle of `screen` into the page being drawn.
// x and w are rounded out to multiples of 4.
void FHY_CopyRect(int x, int y, int w, int h)
{
	int plane, i, j, bx1, bx2, n;
	byte *src, *dst;

	if (w <= 0 || h <= 0)
		return;
	bx1 = x >> 2;
	bx2 = (x + w + 3) >> 2;
	n = bx2 - bx1;
	for (plane = 0; plane < 4; plane++)
	{
		SETMASK(1 << plane);
		for (j = 0; j < h; j++)
		{
			src = screen + (y + j) * SCREENWIDTH + (bx1 << 2) + plane;
			dst = destscreen + (y + j) * 80 + bx1;
			for (i = 0; i < n; i++, src += 4)
				dst[i] = *src;
		}
	}
}

// Whole screen copy, used for automap, intermission, finale and pages
void FHY_CopyScreen(void)
{
	int plane, i;
	byte *src;
	unsigned *dst;

	for (plane = 0; plane < 4; plane++)
	{
		SETMASK(1 << plane);
		src = screen + plane;
		dst = (unsigned *)destscreen;
		for (i = 0; i < SCREENWIDTH * SCREENHEIGHT / 16; i++, src += 16)
		{
			*dst++ = src[0] | (src[4] << 8) | (src[8] << 16)
				| ((unsigned)src[12] << 24);
		}
	}
}

// Read the page being drawn back into `screen` (for screenshots)
void FHY_ReadPage(void)
{
	int plane, i;
	byte *dst, *src;

	for (plane = 0; plane < 4; plane++)
	{
		READPLANE(plane);
		src = destscreen;
		dst = screen + plane;
		for (i = 0; i < SCREENWIDTH * SCREENHEIGHT / 4; i++, dst += 4)
			*dst = src[i];
	}
	borderpages = sbpages = 3;
}

// Bring the border and status bar of the page up to date.
// Called after the view and status bar are drawn, before overlays.
void FHY_Composite(void)
{
	int vx2, vy2;
	int *box;

	if (!lastdirect)
		borderpages = sbpages = 3;
	if (UpdateState & I_FULLSCRN)
		borderpages = sbpages = 3;
	if (UpdateState & I_MESSAGES)
		borderpages = 3;
	if (UpdateState & I_STATBAR)
		sbpages = 3;
	UpdateState = I_NOUPDATE;

	vx2 = viewwindowx + scaledviewwidth;
	vy2 = viewwindowy + viewheight;
	box = pagebox[PAGENUM(destscreen)];
	if (box[2] > box[0] && !borderpages)
	{ // clean up overlays this page got last time, outside the view
		int t, b;
		if (box[1] < viewwindowy)
			FHY_CopyRect(box[0], box[1], box[2] - box[0],
				(box[3] < viewwindowy ? box[3] : viewwindowy) - box[1]);
		if (box[3] > vy2)
		{
			t = box[1] > vy2 ? box[1] : vy2;
			FHY_CopyRect(box[0], t, box[2] - box[0], box[3] - t);
		}
		t = box[1] > viewwindowy ? box[1] : viewwindowy;
		b = box[3] < vy2 ? box[3] : vy2;
		if (b > t)
		{
			if (box[0] < viewwindowx)
				FHY_CopyRect(box[0], t, (box[2] < viewwindowx ? box[2]
					: viewwindowx) - box[0], b - t);
			if (box[2] > vx2)
			{
				int l = box[0] > vx2 ? box[0] : vx2;
				FHY_CopyRect(l, t, box[2] - l, b - t);
			}
		}
	}
	box[0] = box[1] = box[2] = box[3] = 0;
	if (borderpages)
	{
		FHY_CopyRect(0, 0, SCREENWIDTH, viewwindowy);
		FHY_CopyRect(0, viewwindowy, viewwindowx, viewheight);
		FHY_CopyRect(vx2, viewwindowy, SCREENWIDTH - vx2, viewheight);
		FHY_CopyRect(0, vy2, SCREENWIDTH, SCREENHEIGHT - vy2);
		borderpages--;
		fhy_bc++;
		if (sbpages)
			sbpages--;
	}
	else if (sbpages)
	{
		if (vy2 <= SCREENHEIGHT - SBARHEIGHT)
			FHY_CopyRect(0, SCREENHEIGHT - SBARHEIGHT, SCREENWIDTH, SBARHEIGHT);
		sbpages--;
		fhy_sc++;
	}
}

// Show the page just drawn and move on to the next one
void FHY_Update(void)
{
	int *box;

	box = pagebox[PAGENUM(destscreen)];
	box[0] = curbox[0]; box[1] = curbox[1];
	box[2] = curbox[2]; box[3] = curbox[3];
	curbox[0] = SCREENWIDTH; curbox[1] = SCREENHEIGHT;
	curbox[2] = curbox[3] = 0;
	if (fh_directframe)
		lastdirect = 1;
	else
	{
		box[0] = box[1] = box[2] = box[3] = 0;
		FHY_CopyScreen();
		UpdateState = I_NOUPDATE;
		lastdirect = 0;
	}
#ifdef FHY_DEBUG
	{ // dump the 60th frame as raw planar-decoded bytes
		static int frames;
		if (++frames == 60)
		{
			static byte buf[64000];
			int plane, i;
			FILE *f;
			for (plane = 0; plane < 4; plane++)
			{
				READPLANE(plane);
				for (i = 0; i < 16000; i++)
					buf[i * 4 + plane] = destscreen[i];
			}
			f = fopen("PAGE.RAW", "wb");
			fwrite(buf, 1, 64000, f);
			fclose(f);
		}
	}
#endif
	if (fh_vsync)
	{
		while (inp(0x3da) & 8);
	}
	outpw(CRTC_INDEX, ((int)destscreen & 0xff00) + 0x0c);
	if (fh_vsync)
	{
		while (!(inp(0x3da) & 8));
	}
	if (destscreen == (byte *)0xA8000)
		destscreen = (byte *)0xA0000;
	else
		destscreen += PAGEBYTES;
	fh_directframe = 0;
}

#endif // FH_MODEY
