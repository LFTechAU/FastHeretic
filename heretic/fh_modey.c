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

#if defined(FH_MODEY) || (defined(FH_VBD) && !defined(FH_RW))

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

#ifdef FH_MODEX
#define PAGEBYTES  19200         // 320x240: 3 pages fit in 64 KB
#else
#define PAGEBYTES  0x4000
#endif

// where 320x200 graphics go on the page (Mode X: centred, status bar at
// the bottom; Mode Y: as they are)
#define YOFF (fhy_sbmode ? FH_SBOY : FH_OY)

// Half height Mode Y (FHTICH.EXE): 100 lines, each shown 4 times. Only
// even screen rows are drawn; screen row r goes to page row r >> YH.
#ifdef FH_MODEYH
#define YH 1
#else
#define YH 0
#endif
#define PAGEROWS (RENDERHEIGHT >> YH)

extern byte *destscreen, *destview, *pcscreen;
extern int screenblocks;
extern byte *tinttable;

byte *my_ylookup[RENDERHEIGHT];  // view rows in the page being drawn
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
int fhy_sbmode;                   // status bar overlays (unused at 320x200)

#define SETMASK(m) \
	if ((m) != my_curmask) \
	{ \
		my_curmask = (m); \
		outp(SC_DATA, my_curmask); \
	}

#define READPLANE(p) outp(GC_DATA, (p))

#ifdef FH_MODEY

//==========================================================================
// Setup
//==========================================================================

void FHY_InitGraphics(void)
{
	union REGS regs;

	regs.w.ax = 0x13;
	int386(0x10, &regs, &regs);
	pcscreen = (byte *)0xA0000;
	destscreen = (byte *)0xA0000 + PAGEBYTES;

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
#if YH
	outp(CRTC_INDEX, 9);                        // max scan line: 4 per row
	outp(CRTC_INDEX + 1, (inp(CRTC_INDEX + 1) & 0xe0) | 3);
#endif
#ifdef FH_MODEX
	{ // 320x240 at 60 Hz (Michael Abrash's Mode X timings)
		static const unsigned short crtc[] = {0x0d06, 0x3e07, 0x4109,
			0xea10, 0xac11, 0xdf12, 0x0014, 0xe715, 0x0616, 0xe317};
		int i;
		outp(0x3c2, 0xe3);                      // 480 line timing
		outp(CRTC_INDEX, 0x11);                 // unprotect CRTC 0-7
		outp(CRTC_INDEX + 1, inp(CRTC_INDEX + 1) & 0x7f);
		for (i = 0; i < 10; i++)
			outpw(CRTC_INDEX, crtc[i]);
		outpw(SC_INDEX, 0x0f02);
		memset(pcscreen, 0, 0x10000);
		fh_bordbuf = Z_Malloc(RENDERWIDTH * FH_VAH, PU_STATIC, NULL);
		memset(fh_bordbuf, 0, RENDERWIDTH * FH_VAH);
	}
#endif
	outp(GC_INDEX, 4);                          // leave read map selected
	my_curmask = 15;
	borderpages = sbpages = 3;
}

int FHY_Direct(void)
{
	return 1;
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

	for (i = 0; i < viewheight; i++)
		my_ylookup[i] = destscreen + ((viewwindowy + i) >> YH) * 80;
}

//==========================================================================
// Column drawers
//==========================================================================

// first row to draw: in half height only rows on even screen lines
#define YFIRST(y) ((y) + (((y) + viewwindowy) & YH))

#define YCOL_SETUP \
	int count, yl; byte *dest; fixed_t frac, fracstep; \
	yl = YFIRST(dc_yl); \
	if (dc_yh < yl) return; \
	count = (dc_yh - yl) >> YH; \
	SETMASK(my_mask[dc_x]); \
	dest = my_ylookup[yl] + my_ofs[dc_x]; \
	frac = dc_texturemid + (yl - centery) * dc_iscale; \
	fracstep = dc_iscale << YH;

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
	odd = (dc_x + (yl >> YH)) & 1;
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
	odd = (dc_x + (yl >> YH)) & 1;
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

	int yl;

	yl = YFIRST(dc_yl);
	if (dc_yh < yl)
		return;
	count = (dc_yh - yl) >> YH;
	SETMASK(my_mask[dc_x]);
	dest = my_ylookup[yl] + my_ofs[dc_x];
#ifdef FH_HIRES
	{ // taller view: stretch the sky, never read past its 200 rows
		fixed_t frac = dc_texturemid + (yl - centery) * FH_SKYISCALE;
		int r;
		do
		{
			r = frac >> FRACBITS;
			if (r < 0) r = 0; else if (r > 199) r = 199;
			*dest = dc_source[r];
			frac += FH_SKYISCALE;
			dest += 80;
		} while (count--);
	}
	(void)source;
#else
	source = dc_source + (dc_texturemid >> FRACBITS) + (yl - centery);
	do
	{
		*dest = *source;
		source += 1 << YH;
		dest += 80;
	} while (count--);
#endif
}

void R_DrawSkyFlatY(void)
{
	int count; byte *dest; byte c;

	int yl;

	yl = YFIRST(dc_yl);
	if (dc_yh < yl)
		return;
	count = (dc_yh - yl) >> YH;
	SETMASK(my_mask[dc_x]);
	dest = my_ylookup[yl] + my_ofs[dc_x];
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

#if YH
	if ((ds_y + viewwindowy) & 1)
		return;
#endif
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
#if YH
	if ((ds_y + viewwindowy) & 1)
		return;
#endif
	FHY_HLine(my_ylookup[ds_y],
		viewwindowx + (ds_x1 << detailshift),
		viewwindowx + ((ds_x2 + 1) << detailshift) - 1,
		ds_colormap[ds_flatcolor]);
}

//==========================================================================
// Drawer selection
//==========================================================================

void R_DrawColumnYA(void);      // fhasmy.asm
void R_DrawColumnYHA(void);

#if YH
#define R_DrawColumnYA R_DrawColumnYHA
#endif

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


#else // FH_VBD: linear frame buffer with 3 pages

#define YH 0
#define PAGESIZE (SCREENWIDTH * SCREENHEIGHT)

extern byte *fhe_lfb;
extern int fhe_pitch, fhe_pages, fhe_linear;
int FHE_Init(int w, int h, int wantpages);
void FHE_ShowPage(int n, int waitvsync);
void FHE_CopyRows(byte *dest, byte *src, int y, int h);

static byte *ramscreen;          // the normal screen buffer
extern byte *ylookup[];
static int fhd_ok;               // 3 linear pages with 320 byte lines

void FHY_InitGraphics(void)
{
	FHE_Init(SCREENWIDTH, SCREENHEIGHT, 3);
	fhd_ok = fhe_linear && fhe_pitch == SCREENWIDTH && fhe_pages >= 3;
#ifdef FHY_DEBUG
	printf("VBE: linear %d pitch %d pages %d -> direct %d\n",
		fhe_linear, fhe_pitch, fhe_pages, fhd_ok);
#endif
	pcscreen = fhe_lfb;
	destscreen = fhd_ok ? fhe_lfb + PAGESIZE : fhe_lfb;
	if (fhd_ok)
		memset(fhe_lfb, 0, PAGESIZE * 3);
	ramscreen = screen;
	borderpages = sbpages = 3;
}

void R_FHY_InitTables(void)
{
	borderpages = sbpages = 3;
}

void R_FHY_SetupFrame(void)
{
	int i;

	if (!fhd_ok)
		return;     // no pages: draw into the screen buffer as Mode 13h
	for (i = 0; i < viewheight; i++)
		ylookup[i] = destscreen + (viewwindowy + i) * SCREENWIDTH;
}

int FHY_Direct(void)
{
	return fhd_ok;
}

#endif // FH_MODEY

//==========================================================================
// Patches drawn straight into the page (menus, messages, fullscreen HUD)
//==========================================================================

// Overlays drawn outside the view, per page: x1, y1, x2, y2 (exclusive)
static int curbox[4] = {RENDERWIDTH, RENDERHEIGHT, 0, 0};
static int pagebox[3][4];

#ifdef FH_MODEY
#define PAGENUM(p) (((int)(p) - 0xA0000) / PAGEBYTES)
#else
#define PAGENUM(p) (((p) - fhe_lfb) / PAGESIZE)
#endif

static void FHY_AddBox(int x1, int y1, int x2, int y2)
{
	if (x2 > RENDERWIDTH) x2 = RENDERWIDTH;
	if (y2 > RENDERHEIGHT) y2 = RENDERHEIGHT;
	if (x1 < curbox[0]) curbox[0] = x1;
	if (y1 < curbox[1]) curbox[1] = y1;
	if (x2 > curbox[2]) curbox[2] = x2;
	if (y2 > curbox[3]) curbox[3] = y2;
}

#ifdef FH_VBD
// V_Draw* draw into `screen`, which points at the page during overlays:
// just note what lands outside the view
void FHY_NoteBox(int x, int y, patch_t *patch, int mode)
{
	int w, h;

	y -= SHORT(patch->topoffset);
	x -= SHORT(patch->leftoffset);
	w = SHORT(patch->width) + (mode == 2 ? 2 : 0);
	h = SHORT(patch->height) + (mode == 2 ? 2 : 0);
	if (x < viewwindowx || x + w > viewwindowx + scaledviewwidth
		|| y < viewwindowy || y + h > viewwindowy + viewheight)
	{
		FHY_AddBox(x, y, x + w, y + h);
	}
}

// Overlays draw straight into the page
void FHY_OverlayOn(void)
{
	fh_planar = destscreen;
	screen = destscreen;
}

void FHY_OverlayOff(void)
{
	fh_planar = NULL;
	screen = ramscreen;
}

#else

void FHY_OverlayOn(void)
{
	fh_planar = destscreen;
}

void FHY_OverlayOff(void)
{
	fh_planar = NULL;
}

// mode 0 = solid, 1 = translucent, 2 = solid with shadow
void FHY_DrawPatch(int x, int y, patch_t *patch, int mode)
{
	int col, w, count, px, r;
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
	y += YOFF;
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
			top = fh_planar + ((px + 2) >> 2);
			while (c2->topdelta != 0xff)
			{
				r = y + 2 + c2->topdelta;
				count = c2->length;
				if (r + count > RENDERHEIGHT)
					count = RENDERHEIGHT - r;
				if (r & YH)
				{
					r++;
					count--;
				}
				dest = top + (r >> YH) * 80;
				for (; count > 0; count -= 1 << YH)
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
		top = fh_planar + (px >> 2);
		while (column->topdelta != 0xff)
		{
			source = (byte *)column + 3;
			r = y + column->topdelta;
			count = column->length;
			if (r & YH)
			{ // half height: odd screen rows are not shown
				r++;
				source++;
				count--;
			}
			dest = top + (r >> YH) * 80;
			if (mode == 1)
			{
				for (; count > 0; count -= 1 << YH)
				{
					*dest = tinttable[((*dest) << 8) + *source];
					source += 1 << YH;
					dest += 80;
				}
			}
			else
			{
				for (; count > 0; count -= 1 << YH)
				{
					*dest = *source;
					source += 1 << YH;
					dest += 80;
				}
			}
			column = (column_t *)((byte *)column + column->length + 4);
		}
	}
}

#endif // FH_VBD

//==========================================================================
// Linear screen buffer -> page copies
//==========================================================================

#ifdef FH_VBD

void FHY_CopyRect(int x, int y, int w, int h)
{
	byte *src, *dst;

	if (w <= 0 || h <= 0)
		return;
	src = ramscreen + y * SCREENWIDTH + x;
	dst = destscreen + y * SCREENWIDTH + x;
	for (; h > 0; h--, src += SCREENWIDTH, dst += SCREENWIDTH)
		memcpy(dst, src, w);
}

void FHY_CopyScreen(void)
{
	FHE_CopyRows(destscreen, ramscreen, 0, SCREENHEIGHT);
}

void FHY_ReadPage(void)
{
	if (destscreen != ramscreen)
		memcpy(ramscreen, destscreen, PAGESIZE);
	borderpages = sbpages = 3;
}

#else

// The 320 byte line that page row r is copied from
static byte *SrcRow(int r)
{
#ifdef FH_MODEX
	if (r < FH_VAH)
		return fh_bordbuf + r * SCREENWIDTH;
	return screen + (r - FH_SBOY) * SCREENWIDTH;
#else
	return screen + r * SCREENWIDTH;
#endif
}

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
			if ((y + j) & YH)
				continue;
			src = SrcRow(y + j) + (bx1 << 2) + plane;
			dst = destscreen + ((y + j) >> YH) * 80 + bx1;
			for (i = 0; i < n; i++, src += 4)
				dst[i] = *src;
		}
	}
}

// Whole screen copy, used for automap, intermission, finale and pages
void FHY_CopyScreen(void)
{
	int plane, i, r;
	byte *src;
	unsigned *dst;

	for (plane = 0; plane < 4; plane++)
	{
		SETMASK(1 << plane);
		dst = (unsigned *)destscreen;
		for (r = 0; r < PAGEROWS; r++)
		{
			if ((r << YH) < FH_OY || (r << YH) >= FH_OY + SCREENHEIGHT)
			{ // Mode X: black above and below the 320x200 picture
				memset(dst, 0, SCREENWIDTH / 4);
				dst += SCREENWIDTH / 16;
				continue;
			}
			src = screen + ((r << YH) - FH_OY) * SCREENWIDTH + plane;
			for (i = 0; i < SCREENWIDTH / 16; i++, src += 16)
			{
				*dst++ = src[0] | (src[4] << 8) | (src[8] << 16)
					| ((unsigned)src[12] << 24);
			}
		}
	}
}

// Read the page being drawn back into `screen` (for screenshots)
void FHY_ReadPage(void)
{
	int plane, i, r;
	byte *dst, *src;

	for (plane = 0; plane < 4; plane++)
	{
		READPLANE(plane);
		for (r = 0; r < SCREENHEIGHT; r++)
		{
			src = destscreen + ((r + FH_OY) >> YH) * 80;
			dst = screen + r * SCREENWIDTH + plane;
			for (i = 0; i < 80; i++, dst += 4)
				*dst = src[i];
		}
	}
	{ // and the whole page, for checking
		static byte buf[PAGEBYTES * 4];
		FILE *f;
		for (plane = 0; plane < 4; plane++)
		{
			READPLANE(plane);
			for (i = 0; i < PAGEBYTES; i++)
				buf[i * 4 + plane] = destscreen[i];
		}
		f = fopen("HIRES.RAW", "wb");
		if (f)
		{
			fwrite(buf, 1, PAGEBYTES * 4, f);
			fclose(f);
		}
	}
	borderpages = sbpages = 3;
}

#endif // FH_VBD

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
		FHY_CopyRect(0, vy2, SCREENWIDTH, RENDERHEIGHT - vy2);
		borderpages--;
		fhy_bc++;
		if (sbpages)
			sbpages--;
	}
	else if (sbpages)
	{
		if (vy2 <= RENDERHEIGHT - SBARHEIGHT)
			FHY_CopyRect(0, RENDERHEIGHT - SBARHEIGHT, SCREENWIDTH, SBARHEIGHT);
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
	curbox[0] = RENDERWIDTH; curbox[1] = RENDERHEIGHT;
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
#ifdef FH_VBD
	if (fhd_ok)
	{
		FHE_ShowPage(PAGENUM(destscreen), fh_vsync);
		destscreen += PAGESIZE;
		if (destscreen == fhe_lfb + PAGESIZE * 3)
			destscreen = fhe_lfb;
	}
	fh_directframe = 0;
	return;
#endif
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
	if (destscreen == (byte *)0xA0000 + 2 * PAGEBYTES)
		destscreen = (byte *)0xA0000;
	else
		destscreen += PAGEBYTES;
	fh_directframe = 0;
}

#endif // FH_MODEY || FH_VBD
