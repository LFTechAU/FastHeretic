// r_fhdraw.c : FastHeretic detail-aware drawing routines
//
// Low detail draws every column two pixels wide, Potato detail four pixels
// wide (the FastDoom approach). Also the flat (untextured) floor/ceiling
// and flat sky fillers, and the translucency-off column drawers.
//
// FastHeretic is free software, licensed under the GNU GPL v2.

#include <string.h>
#include "doomdef.h"
#include "r_local.h"
#include "r_fh.h"

extern byte *tinttable;
extern byte *dc_translation;
extern byte *ylookup[MAXHEIGHT];
extern int columnofs[MAXWIDTH];
extern int detailshift;

int fh_detail;           // 0 = high, 1 = low, 2 = potato
int fh_flatsurfaces;     // 1 = untextured floors and ceilings
int fh_flatsky;          // 1 = sky drawn as a single colour
int fh_notrans;          // 1 = translucent sprites drawn solid
int fh_showfps;          // 1 = frame rate counter
int fh_near;             // 1 = cull distant non-monster sprites
int fh_vsync;            // 1 = wait for retrace before blits
int fh_csv;
int fh_fixdac;           // wait for retrace before palette changes
             // 1 = append timedemo results to FHBENCH.CSV

byte skyflatcolor;

void (*skycolfunc)(void);

//==========================================================================
// Pixel writers for each detail level
//==========================================================================

#define PUT1(d,c) (*(d) = (c))
#define PUT2(d,c) (*(unsigned short *)(d) = (unsigned short)((c) * 0x0101))
#define PUT4(d,c) (*(unsigned int *)(d) = (unsigned int)(c) * 0x01010101u)

#define COLUMN_SETUP(SHIFT) \
	int count; byte *dest; fixed_t frac, fracstep; \
	count = dc_yh - dc_yl; \
	if (count < 0) return; \
	dest = ylookup[dc_yl] + columnofs[dc_x << (SHIFT)]; \
	fracstep = dc_iscale; \
	frac = dc_texturemid + (dc_yl - centery) * fracstep;

//--------------------------------------------------------------------------
// Solid columns (walls, sprites)
//--------------------------------------------------------------------------

#define MAKE_COLUMN(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	byte *source; byte *colormap; \
	COLUMN_SETUP(SHIFT) \
	source = dc_source; colormap = dc_colormap; \
	do \
	{ \
		PUT(dest, colormap[source[(frac >> FRACBITS) & 127]]); \
		dest += SCREENWIDTH; \
		frac += fracstep; \
	} while (count--); \
}

MAKE_COLUMN(R_DrawColumnLowFH, 1, PUT2)
MAKE_COLUMN(R_DrawColumnPotato, 2, PUT4)

//--------------------------------------------------------------------------
// Translated (player colour) columns
//--------------------------------------------------------------------------

#define MAKE_TRANSCOLUMN(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	COLUMN_SETUP(SHIFT) \
	do \
	{ \
		PUT(dest, dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]]); \
		dest += SCREENWIDTH; \
		frac += fracstep; \
	} while (count--); \
}

MAKE_TRANSCOLUMN(R_DrawTranslatedColumnLow, 1, PUT2)
MAKE_TRANSCOLUMN(R_DrawTranslatedColumnPotato, 2, PUT4)

//--------------------------------------------------------------------------
// Translucent (tinttable) columns
//--------------------------------------------------------------------------

#define MAKE_FUZZCOLUMN(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	COLUMN_SETUP(SHIFT) \
	do \
	{ \
		PUT(dest, tinttable[((*dest) << 8) + \
			dc_colormap[dc_source[(frac >> FRACBITS) & 127]]]); \
		dest += SCREENWIDTH; \
		frac += fracstep; \
	} while (count--); \
}

#define MAKE_TRANSFUZZCOLUMN(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	COLUMN_SETUP(SHIFT) \
	do \
	{ \
		PUT(dest, tinttable[((*dest) << 8) + \
			dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]]]); \
		dest += SCREENWIDTH; \
		frac += fracstep; \
	} while (count--); \
}

//--------------------------------------------------------------------------
// Saturn style ghosts: a checkerboard of solid pixels (FastDoom -saturn)
//--------------------------------------------------------------------------

#define MAKE_SATURN(NAME, SHIFT, PUT, SOURCE) \
void NAME(void) \
{ \
	int odd; \
	COLUMN_SETUP(SHIFT) \
	odd = (dc_x + dc_yl) & 1; \
	do \
	{ \
		if (odd) \
			PUT(dest, SOURCE); \
		odd ^= 1; \
		dest += SCREENWIDTH; \
		frac += fracstep; \
	} while (count--); \
}

#define SAT_SRC dc_colormap[dc_source[(frac >> FRACBITS) & 127]]
#define SAT_TSRC dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]]

MAKE_SATURN(R_DrawSaturnColumn, 0, PUT1, SAT_SRC)
MAKE_SATURN(R_DrawSaturnColumnLow, 1, PUT2, SAT_SRC)
MAKE_SATURN(R_DrawSaturnColumnPotato, 2, PUT4, SAT_SRC)
MAKE_SATURN(R_DrawSaturnTransColumn, 0, PUT1, SAT_TSRC)
MAKE_SATURN(R_DrawSaturnTransColumnLow, 1, PUT2, SAT_TSRC)
MAKE_SATURN(R_DrawSaturnTransColumnPotato, 2, PUT4, SAT_TSRC)

MAKE_FUZZCOLUMN(R_DrawFuzzColumnLow, 1, PUT2)
MAKE_FUZZCOLUMN(R_DrawFuzzColumnPotato, 2, PUT4)
MAKE_TRANSFUZZCOLUMN(R_DrawTranslatedFuzzColumnLow, 1, PUT2)
MAKE_TRANSFUZZCOLUMN(R_DrawTranslatedFuzzColumnPotato, 2, PUT4)

//--------------------------------------------------------------------------
// Sky columns (no colour mapping, 1:1 vertical scale)
//--------------------------------------------------------------------------

#define MAKE_SKYCOLUMN(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	int count; byte *dest; byte *source; \
	count = dc_yh - dc_yl; \
	if (count < 0) return; \
	dest = ylookup[dc_yl] + columnofs[dc_x << (SHIFT)]; \
	source = dc_source + (dc_texturemid >> FRACBITS) + (dc_yl - centery); \
	do \
	{ \
		PUT(dest, *source); \
		source++; \
		dest += SCREENWIDTH; \
	} while (count--); \
}

MAKE_SKYCOLUMN(R_DrawSkyColumn, 0, PUT1)
MAKE_SKYCOLUMN(R_DrawSkyColumnLow, 1, PUT2)
MAKE_SKYCOLUMN(R_DrawSkyColumnPotato, 2, PUT4)

#define MAKE_SKYFLAT(NAME, SHIFT, PUT) \
void NAME(void) \
{ \
	int count; byte *dest; byte c; \
	count = dc_yh - dc_yl; \
	if (count < 0) return; \
	dest = ylookup[dc_yl] + columnofs[dc_x << (SHIFT)]; \
	c = skyflatcolor; \
	do \
	{ \
		PUT(dest, c); \
		dest += SCREENWIDTH; \
	} while (count--); \
}

MAKE_SKYFLAT(R_DrawSkyFlat, 0, PUT1)
MAKE_SKYFLAT(R_DrawSkyFlatLow, 1, PUT2)
MAKE_SKYFLAT(R_DrawSkyFlatPotato, 2, PUT4)

//--------------------------------------------------------------------------
// Spans (floors and ceilings)
//--------------------------------------------------------------------------

#define MAKE_SPAN(NAME, SHIFT, PUT, STEP) \
void NAME(void) \
{ \
	fixed_t xfrac, yfrac, xstep, ystep; \
	byte *dest; byte *source; byte *colormap; \
	int count, spot; \
	xfrac = ds_xfrac; yfrac = ds_yfrac; \
	xstep = ds_xstep; ystep = ds_ystep; \
	source = ds_source; colormap = ds_colormap; \
	dest = ylookup[ds_y] + columnofs[ds_x1 << (SHIFT)]; \
	count = ds_x2 - ds_x1; \
	do \
	{ \
		spot = ((yfrac >> (16 - 6)) & (63 * 64)) + ((xfrac >> 16) & 63); \
		PUT(dest, colormap[source[spot]]); \
		dest += STEP; \
		xfrac += xstep; \
		yfrac += ystep; \
	} while (count--); \
}

MAKE_SPAN(R_DrawSpanLowFH, 1, PUT2, 2)
MAKE_SPAN(R_DrawSpanPotato, 2, PUT4, 4)

// Flat (untextured) span: one colour per flat, still lit by distance.
// Works for every detail level since it fills whole pixels.
byte ds_flatcolor;

void R_DrawSpanFlat(void)
{
	byte *dest;

	dest = ylookup[ds_y] + columnofs[ds_x1 << detailshift];
	memset(dest, ds_colormap[ds_flatcolor],
		(ds_x2 - ds_x1 + 1) << detailshift);
}

//==========================================================================
// Average colours for flat floors and the flat sky
//==========================================================================

static byte *flatcolors;    // one entry per flat, 0 = not yet computed
static byte *flatknown;

static byte BestColor(int r, int g, int b, byte *palette)
{
	int i, dr, dg, db, dist, best, bestdist;

	best = 0;
	bestdist = 0x7fffffff;
	for (i = 0; i < 256; i++)
	{
		dr = r - palette[i * 3];
		dg = g - palette[i * 3 + 1];
		db = b - palette[i * 3 + 2];
		dist = dr * dr + dg * dg + db * db;
		if (dist < bestdist)
		{
			bestdist = dist;
			best = i;
		}
	}
	return (byte)best;
}

static byte AverageColor(byte *pixels, int count, int step)
{
	byte *palette;
	int i, r, g, b, n;

	palette = W_CacheLumpName("PLAYPAL", PU_CACHE);
	r = g = b = n = 0;
	for (i = 0; i < count; i += step)
	{
		r += palette[pixels[i] * 3];
		g += palette[pixels[i] * 3 + 1];
		b += palette[pixels[i] * 3 + 2];
		n++;
	}
	if (!n)
		return 0;
	return BestColor(r / n, g / n, b / n, palette);
}

byte R_FlatColor(int flatnum)
{
	byte *source;

	if (!flatcolors)
	{
		flatcolors = Z_Malloc(numflats, PU_STATIC, NULL);
		flatknown = Z_Malloc(numflats, PU_STATIC, NULL);
		memset(flatknown, 0, numflats);
	}
	if (!flatknown[flatnum])
	{
		source = W_CacheLumpNum(firstflat + flatnum, PU_CACHE);
		flatcolors[flatnum] = AverageColor(source, 64 * 64, 3);
		flatknown[flatnum] = 1;
	}
	return flatcolors[flatnum];
}

void R_ComputeSkyColor(void)
{
	byte *palette;
	byte *column;
	int x, y, r, g, b, n;

	palette = W_CacheLumpName("PLAYPAL", PU_CACHE);
	r = g = b = n = 0;
	for (x = 0; x < 256; x += 8)
	{
		column = R_GetColumn(skytexture, x);
		for (y = 0; y < 128; y += 4)
		{
			r += palette[column[y] * 3];
			g += palette[column[y] * 3 + 1];
			b += palette[column[y] * 3 + 2];
			n++;
		}
	}
	skyflatcolor = BestColor(r / n, g / n, b / n, palette);
}

//==========================================================================
// Pick drawers for the current detail level and options
//==========================================================================

// Assembly versions (fhasm.asm)
void R_DrawColumnLowA(void);
void R_DrawColumnPotatoA(void);
void R_DrawSpanLowA(void);
void R_DrawSpanPotatoA(void);

void R_FH_SetDrawers(void)
{
#ifdef FH_MODEY
	R_FHY_SetDrawers();
	return;
#endif
	switch (detailshift)
	{
	case 0:
		colfunc = basecolfunc = R_DrawColumn;
		fuzzcolfunc = (fh_notrans == 1) ? R_DrawColumn : R_DrawFuzzColumn;
		transcolfunc = R_DrawTranslatedColumn;
		transfuzzcolfunc = (fh_notrans == 1) ? R_DrawTranslatedColumn
			: R_DrawTranslatedFuzzColumn;
		spanfunc = R_DrawSpan;
		skycolfunc = fh_flatsky ? R_DrawSkyFlat : R_DrawSkyColumn;
		break;
	case 1:
		colfunc = basecolfunc = R_DrawColumnLowA;
		fuzzcolfunc = (fh_notrans == 1) ? R_DrawColumnLowA : R_DrawFuzzColumnLow;
		transcolfunc = R_DrawTranslatedColumnLow;
		transfuzzcolfunc = (fh_notrans == 1) ? R_DrawTranslatedColumnLow
			: R_DrawTranslatedFuzzColumnLow;
		spanfunc = R_DrawSpanLowA;
		skycolfunc = fh_flatsky ? R_DrawSkyFlatLow : R_DrawSkyColumnLow;
		break;
	default:
		colfunc = basecolfunc = R_DrawColumnPotatoA;
		fuzzcolfunc = (fh_notrans == 1) ? R_DrawColumnPotatoA
			: R_DrawFuzzColumnPotato;
		transcolfunc = R_DrawTranslatedColumnPotato;
		transfuzzcolfunc = (fh_notrans == 1) ? R_DrawTranslatedColumnPotato
			: R_DrawTranslatedFuzzColumnPotato;
		spanfunc = R_DrawSpanPotatoA;
		skycolfunc = fh_flatsky ? R_DrawSkyFlatPotato : R_DrawSkyColumnPotato;
		break;
	}
	if (fh_notrans == 2)
	{
		switch (detailshift)
		{
		case 0:
			fuzzcolfunc = R_DrawSaturnColumn;
			transfuzzcolfunc = R_DrawSaturnTransColumn;
			break;
		case 1:
			fuzzcolfunc = R_DrawSaturnColumnLow;
			transfuzzcolfunc = R_DrawSaturnTransColumnLow;
			break;
		default:
			fuzzcolfunc = R_DrawSaturnColumnPotato;
			transfuzzcolfunc = R_DrawSaturnTransColumnPotato;
			break;
		}
	}
	if (fh_flatsurfaces)
		spanfunc = R_DrawSpanFlat;
}
