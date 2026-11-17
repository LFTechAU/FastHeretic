//
// fh_hires.c : FastHeretic high resolution builds
//
// The 3D view is drawn at RENDERWIDTH x RENDERHEIGHT. Everything else
// (status bar, menus, messages, intermission, title pages) is still drawn
// by the original code at 320x200 and scaled up by a whole number FH_S:
// centred on the screen, with the status bar along the bottom. The border
// around a small view is drawn at full resolution into its own buffer.
//
// The shared part (border, scaled patches) is used by Mode X too. The VESA
// part draws straight into one of 3 video pages when the card has a
// linear frame buffer with room for them (FHM...D.EXE), otherwise into
// system memory, copying only what changed to the card (FHM...R.EXE).
// Ideas from FastDoom's high resolution modes (Victor Nieto, GPL).
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include "doomdef.h"

#ifdef FH_HIRES

#include <string.h>
#include <conio.h>
#include <stdio.h>
#include "r_local.h"
#include "r_fh.h"

#define RW  RENDERWIDTH
#define RH  RENDERHEIGHT
#define S   FH_S
#define VAH FH_VAH

extern byte *tinttable;
extern boolean shareware;

byte *fh_bordbuf;                // border around a small view, RW x VAH

//==========================================================================
// Shared: scaled patches and the view border
//==========================================================================

// Draw a patch scaled by S into a linear buffer. ox, oy: top left corner
// in buffer pixels (offsets already applied). mode 0 = solid,
// 1 = translucent, 2 = solid with a shadow.
void FHH_PatchToBuf(byte *buf, int bw, int bh, int ox, int oy,
	patch_t *patch, int mode)
{
	int col, w, sx, sy, px, py, i, sh;
	column_t *column;
	byte *source, *dest;

	w = SHORT(patch->width);
	for (sh = (mode == 2); sh >= 0; sh--)
	{ // the shadow first, 2 pixels right and down
		int off = sh ? 2 * S : 0;
		for (col = 0; col < w; col++)
		{
			for (sx = 0; sx < S; sx++)
			{
				px = ox + off + col * S + sx;
				if (px < 0 || px >= bw)
					continue;
				column = (column_t *)((byte *)patch
					+ LONG(patch->columnofs[col]));
				while (column->topdelta != 0xff)
				{
					source = (byte *)column + 3;
					for (i = 0; i < column->length; i++, source++)
					{
						py = oy + off + (column->topdelta + i) * S;
						for (sy = 0; sy < S; sy++, py++)
						{
							if (py < 0 || py >= bh)
								continue;
							dest = buf + py * bw + px;
							if (sh)
								*dest = tinttable[(*dest) << 8];
							else if (mode == 1)
								*dest = tinttable[((*dest) << 8) + *source];
							else
								*dest = *source;
						}
					}
					column = (column_t *)((byte *)column + column->length + 4);
				}
			}
		}
	}
}

static void BorderPatch(int x, int y, char *name)
{
	patch_t *p = W_CacheLumpName(name, PU_CACHE);
	FHH_PatchToBuf(fh_bordbuf, RW, VAH, x - SHORT(p->leftoffset) * S,
		y - SHORT(p->topoffset) * S, p, 0);
}

// Draw the border around a small view, at full resolution
void R_DrawViewBorder(void)
{
	byte *src, *dest;
	int x, y, vx, vy, vw, vh;

	if (scaledviewwidth == RW || !fh_bordbuf)
		return;
	src = W_CacheLumpName(shareware ? "FLOOR04" : "FLAT513", PU_CACHE);
	dest = fh_bordbuf;
	for (y = 0; y < VAH; y++, dest += RW)
	{
		if (y % S)
		{ // same as the row above
			memcpy(dest, dest - RW, RW);
			continue;
		}
		for (x = 0; x < RW; x++)
			dest[x] = src[(((y / S) & 63) << 6) + ((x / S) & 63)];
	}
	vx = viewwindowx;
	vy = viewwindowy;
	vw = scaledviewwidth;
	vh = viewheight;
	for (x = vx; x < vx + vw; x += 16 * S)
	{
		BorderPatch(x, vy - 4 * S, "bordt");
		BorderPatch(x, vy + vh, "bordb");
	}
	for (y = vy; y < vy + vh; y += 16 * S)
	{
		BorderPatch(vx - 4 * S, y, "bordl");
		BorderPatch(vx + vw, y, "bordr");
	}
	BorderPatch(vx - 4 * S, vy - 4 * S, "bordtl");
	BorderPatch(vx + vw, vy - 4 * S, "bordtr");
	BorderPatch(vx + vw, vy + vh, "bordbr");
	BorderPatch(vx - 4 * S, vy + vh, "bordbl");
	BorderNeedRefresh = false;
	UpdateState |= I_FULLSCRN;
}

// Messages are overlays here, cleaned off by the page compositing
void R_DrawTopBorder(void)
{
}

#ifndef FH_MODEY

//==========================================================================
// VESA: linear pages or a system memory frame
//==========================================================================

extern byte *destscreen, *pcscreen;
extern byte *ylookup[];
extern int screenblocks;
extern byte *fhe_lfb;
extern int fhe_pitch, fhe_pages, fhe_linear;
void FHE_ShowPage(int n, int waitvsync);

#define PAGESIZE (RW * RH)

byte *fh_planar;                 // set: V_Draw* draw into this page
int fh_directframe;              // this frame's view went straight to the page
int fhy_sbmode;                  // status bar overlays: anchor at the bottom
int fhy_bc, fhy_sc;

static byte *hiscreen;           // the frame in system memory (copy builds)
static int direct;               // drawing into video pages
static int npages = 1;
static int borderpages, sbpages, fullpages, lastdirect;
static int curbox[4] = {RW, RH, 0, 0};
static int pagebox[3][4];
static int fbox[4] = {RW, RH, 0, 0};     // copy builds: changed this frame
static byte rowbuf[RW];

static int PageNum(byte *p)
{
	return direct ? (p - fhe_lfb) / PAGESIZE : 0;
}

// note a changed rectangle (copy builds send only these to the card)
static void Mark(int x1, int y1, int x2, int y2)
{
	if (x1 < fbox[0]) fbox[0] = x1;
	if (y1 < fbox[1]) fbox[1] = y1;
	if (x2 > fbox[2]) fbox[2] = x2;
	if (y2 > fbox[3]) fbox[3] = y2;
}

void FHY_InitGraphics(void)
{
	if (!FHE_Init(RW, RH, 3))
	{
		I_Error("No VESA %dx%d 256 colour mode found.\n"
			"Load a VESA driver such as UNIVBE, or try FHTIC.EXE.", RW, RH);
	}
#ifdef FH_VBD
	direct = fhe_linear && fhe_pitch == RW && fhe_pages >= 3;
#endif
	if (direct)
	{
		npages = 3;
		pcscreen = fhe_lfb;
		destscreen = fhe_lfb + PAGESIZE;
		memset(fhe_lfb, 0, PAGESIZE * 3);
	}
	else
	{
		npages = 1;
		hiscreen = Z_Malloc(PAGESIZE, PU_STATIC, NULL);
		memset(hiscreen, 0, PAGESIZE);
		pcscreen = destscreen = hiscreen;
		FHE_CopyRect(hiscreen, RW, 0, 0, RW, RH);
	}
	fh_bordbuf = Z_Malloc(RW * VAH, PU_STATIC, NULL);
	memset(fh_bordbuf, 0, RW * VAH);
	borderpages = sbpages = fullpages = npages;
}

int FHY_Direct(void)
{
	return 1;
}

void R_FHY_InitTables(void)
{
	borderpages = sbpages = npages;
}

void R_FHY_SetupFrame(void)
{
	int i;

	for (i = 0; i < viewheight; i++)
		ylookup[i] = destscreen + (viewwindowy + i) * RW;
}

void FHY_OverlayOn(void)
{
	fh_planar = destscreen;
}

void FHY_OverlayOff(void)
{
	fh_planar = NULL;
}

static void AddBox(int x1, int y1, int x2, int y2)
{
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	if (x2 > RW) x2 = RW;
	if (y2 > RH) y2 = RH;
	if (x1 < curbox[0]) curbox[0] = x1;
	if (y1 < curbox[1]) curbox[1] = y1;
	if (x2 > curbox[2]) curbox[2] = x2;
	if (y2 > curbox[3]) curbox[3] = y2;
}

// V_DrawPatch and friends while an overlay is on: scaled into the page
void FHY_DrawPatch(int x, int y, patch_t *patch, int mode)
{
	int ox, oy, w, h;

	ox = FH_OX + (x - SHORT(patch->leftoffset)) * S;
	oy = (fhy_sbmode ? FH_SBOY : FH_OY) + (y - SHORT(patch->topoffset)) * S;
	w = SHORT(patch->width) * S + (mode == 2 ? 2 * S : 0);
	h = SHORT(patch->height) * S + (mode == 2 ? 2 * S : 0);
	if (ox < viewwindowx || ox + w > viewwindowx + scaledviewwidth
		|| oy < viewwindowy || oy + h > viewwindowy + viewheight)
	{ // outside the view: clean it off this page next time round
		AddBox(ox, oy, ox + w, oy + h);
	}
	Mark(ox < 0 ? 0 : ox, oy < 0 ? 0 : oy, ox + w > RW ? RW : ox + w,
		oy + h > RH ? RH : oy + h);
	FHH_PatchToBuf(fh_planar, RW, RH, ox, oy, patch, mode);
}

void FHY_NoteBox(int x, int y, patch_t *patch, int mode)
{
}

// Scale rows v1..v2-1 of the 320x200 screen into the page at oy + v * S,
// with black at the sides
static void ScaleRows(int v1, int v2, int oy)
{
	int v, sy, x, y;
	byte *src, *dst;

	for (v = v1; v < v2; v++)
	{
		src = screen + v * SCREENWIDTH;
		if (S > 1)
		{
			dst = rowbuf;
			for (x = 0; x < SCREENWIDTH; x++, dst += S)
				memset(dst, src[x], S);
			src = rowbuf;
		}
		for (sy = 0; sy < S; sy++)
		{
			y = oy + v * S + sy;
			if (y < 0 || y >= RH)
				continue;
			dst = destscreen + y * RW;
			if (FH_OX)
			{
				memset(dst, 0, FH_OX);
				memset(dst + FH_OX + 320 * S, 0, RW - FH_OX - 320 * S);
			}
			memcpy(dst + FH_OX, src, 320 * S);
		}
	}
	Mark(0, oy + v1 * S, RW, oy + v2 * S);
}

static void StatusBar(void)
{
	ScaleRows(SCREENHEIGHT - SBARHEIGHT, SCREENHEIGHT, FH_SBOY);
}

// Copy part of the border buffer into the page
static void CopyBord(int x, int y, int w, int h)
{
	byte *src, *dst;

	if (y + h > VAH)
		h = VAH - y;
	if (w <= 0 || h <= 0)
		return;
	Mark(x, y, x + w, y + h);
	src = fh_bordbuf + y * RW + x;
	dst = destscreen + y * RW + x;
	for (; h > 0; h--, src += RW, dst += RW)
		memcpy(dst, src, w);
}

// Put back what was under an overlay (outside the view)
static void Restore(int x, int y, int w, int h)
{
	int v1, v2;

	if (w <= 0 || h <= 0)
		return;
	if (y < VAH)
		CopyBord(x, y, w, h);
	if (y + h > VAH)
	{
		v1 = ((y > VAH ? y : VAH) - FH_SBOY) / S;
		v2 = (y + h - FH_SBOY + S - 1) / S;
		if (v1 < SCREENHEIGHT - SBARHEIGHT) v1 = SCREENHEIGHT - SBARHEIGHT;
		if (v2 > SCREENHEIGHT) v2 = SCREENHEIGHT;
		if (v2 > v1)
			ScaleRows(v1, v2, FH_SBOY);
	}
}

// Bring the border and status bar of the page up to date.
void FHY_Composite(void)
{
	int vx2, vy2;
	int *box;

	if (!lastdirect)
		borderpages = sbpages = npages;
	if (UpdateState & I_FULLSCRN)
		borderpages = sbpages = npages;
	if (UpdateState & I_MESSAGES)
		borderpages = npages;
	if (UpdateState & I_STATBAR)
		sbpages = npages;
	UpdateState = I_NOUPDATE;

	vx2 = viewwindowx + scaledviewwidth;
	vy2 = viewwindowy + viewheight;
	box = pagebox[PageNum(destscreen)];
	if (box[2] > box[0] && !borderpages)
	{ // clean up overlays this page got last time, outside the view
		int t, b;
		if (box[1] < viewwindowy)
			Restore(box[0], box[1], box[2] - box[0],
				(box[3] < viewwindowy ? box[3] : viewwindowy) - box[1]);
		if (box[3] > vy2)
		{
			t = box[1] > vy2 ? box[1] : vy2;
			Restore(box[0], t, box[2] - box[0], box[3] - t);
		}
		t = box[1] > viewwindowy ? box[1] : viewwindowy;
		b = box[3] < vy2 ? box[3] : vy2;
		if (b > t)
		{
			if (box[0] < viewwindowx)
				Restore(box[0], t, (box[2] < viewwindowx ? box[2]
					: viewwindowx) - box[0], b - t);
			if (box[2] > vx2)
			{
				int l = box[0] > vx2 ? box[0] : vx2;
				Restore(l, t, box[2] - l, b - t);
			}
		}
	}
	box[0] = box[1] = box[2] = box[3] = 0;
	if (borderpages)
	{
		if (scaledviewwidth != RW)
		{
			CopyBord(0, 0, RW, viewwindowy);
			CopyBord(0, viewwindowy, viewwindowx, viewheight);
			CopyBord(vx2, viewwindowy, RW - vx2, viewheight);
			CopyBord(0, vy2, RW, VAH - vy2);
		}
		if (vy2 <= VAH)
			StatusBar();
		borderpages--;
		fhy_bc++;
		if (sbpages)
			sbpages--;
	}
	else if (sbpages)
	{
		if (vy2 <= VAH)
			StatusBar();
		sbpages--;
		fhy_sc++;
	}
}

// Read the page being drawn back into `screen` (for screenshots)
void FHY_ReadPage(void)
{
	int u, v;

	for (v = 0; v < SCREENHEIGHT; v++)
		for (u = 0; u < SCREENWIDTH; u++)
			screen[v * SCREENWIDTH + u] =
				destscreen[(FH_OY + v * S) * RW + FH_OX + u * S];
	{ // and the full resolution picture, for checking
		FILE *f = fopen("HIRES.RAW", "wb");
		if (f)
		{
			fwrite(destscreen, 1, PAGESIZE, f);
			fclose(f);
		}
	}
	borderpages = sbpages = npages;
}

// Show the page just drawn and move on to the next one
void FHY_Update(void)
{
	int *box;

	box = pagebox[PageNum(destscreen)];
	box[0] = curbox[0]; box[1] = curbox[1];
	box[2] = curbox[2]; box[3] = curbox[3];
	curbox[0] = RW; curbox[1] = RH;
	curbox[2] = curbox[3] = 0;
	if (fh_directframe)
	{
		lastdirect = 1;
		fullpages = npages;
		Mark(viewwindowx, viewwindowy, viewwindowx + scaledviewwidth,
			viewwindowy + viewheight);
	}
	else
	{ // automap, intermission, finale, title: scale the whole screen
		box[0] = box[1] = box[2] = box[3] = 0;
		if (UpdateState != I_NOUPDATE || lastdirect)
			fullpages = npages;
		if (fullpages)
		{
			if (FH_OY)
			{
				memset(destscreen, 0, FH_OY * RW);
				memset(destscreen + (FH_OY + 200 * S) * RW, 0,
					(RH - FH_OY - 200 * S) * RW);
			}
			ScaleRows(0, SCREENHEIGHT, FH_OY);
			Mark(0, 0, RW, RH);
			fullpages--;
		}
		UpdateState = I_NOUPDATE;
		lastdirect = 0;
	}
	if (direct)
	{
		FHE_ShowPage(PageNum(destscreen), fh_vsync);
		destscreen += PAGESIZE;
		if (destscreen == fhe_lfb + PAGESIZE * 3)
			destscreen = fhe_lfb;
	}
	else if (fbox[2] > fbox[0] && fbox[3] > fbox[1])
	{
		if (fh_vsync)
		{
			while (inp(0x3da) & 8);
			while (!(inp(0x3da) & 8));
		}
		FHE_CopyRect(hiscreen, RW, fbox[0], fbox[1], fbox[2] - fbox[0],
			fbox[3] - fbox[1]);
	}
	fbox[0] = RW; fbox[1] = RH;
	fbox[2] = fbox[3] = 0;
	fh_directframe = 0;
}

#endif // !FH_MODEY
#endif // FH_HIRES
