//
// fhvideo.c : FastHeretic glue for FastDoom's backbuffered video modes
//
// Heretic draws everything into `screen` (320x200, 256 colours). In these
// builds `screen` is FastDoom's `backbuffer`, and FastDoom's converters
// (heretic/fd/) turn it into CGA, EGA, Hercules, Plantronics, Sigma or
// InColor output. A palette change rebuilds the colour lookup table for
// the new palette.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include "doomdef.h"

#ifdef FH_BACKBUF

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

byte backbuffer[SCREENWIDTH * SCREENHEIGHT];
byte fd_gamma[256];
unsigned short *currentscreen;
int CGApalette1;
int snowfix;
unsigned char CGAmodel = 1;   // CGA_NEW

#if defined(MODE_T4025) || defined(MODE_T4050) || defined(MODE_T8025) \
	|| defined(MODE_T8043) || defined(MODE_T8050) || defined(MODE_MDA) \
	|| defined(MODE_CMDA) || defined(MODE_VT100)
#define FH_TEXT
#endif

int GetClosestColor(const byte *colors, int num_colors, int r1, int g1, int b1);

#ifdef FH_TEXT
//==========================================================================
// Text modes: each character cell shows one or two blocks of the picture
// (a half block character with different top and bottom colours).
//==========================================================================

#if defined(MODE_T4025)
#define TCOLS 40
#define TROWS 25
#define THALF 0
#elif defined(MODE_T4050)
#define TCOLS 40
#define TROWS 25
#define THALF 1
#elif defined(MODE_T8025)
#define TCOLS 80
#define TROWS 25
#define THALF 1
#elif defined(MODE_T8043)
#define TCOLS 80
#define TROWS 43
#define THALF 1
#elif defined(MODE_T8050)
#define TCOLS 80
#define TROWS 50
#define THALF 1
#elif defined(MODE_MDA)
#define TCOLS 80
#define TROWS 25
#define THALF 0
#elif defined(MODE_CMDA)      // colour text at the MDA address (B000)
#define TCOLS 80
#define TROWS 25
#define THALF 1
#elif defined(MODE_VT100)     // shaded ASCII, also sent to a serial terminal
#define TCOLS 80
#define TROWS 24
#define THALF 0
#endif

#if defined(MODE_MDA) || defined(MODE_CMDA)
#define TVRAM ((unsigned short *)0xB0000)
#else
#define TVRAM ((unsigned short *)0xB8000)
#endif

static const byte textcolors[48] = {
	0x00, 0x00, 0x00,  0x00, 0x00, 0x2A,  0x00, 0x2A, 0x00,  0x00, 0x2A, 0x2A,
	0x2A, 0x00, 0x00,  0x2A, 0x00, 0x2A,  0x2A, 0x15, 0x00,  0x2A, 0x2A, 0x2A,
	0x15, 0x15, 0x15,  0x15, 0x15, 0x3F,  0x15, 0x3F, 0x15,  0x15, 0x3F, 0x3F,
	0x3F, 0x15, 0x15,  0x3F, 0x15, 0x3F,  0x3F, 0x3F, 0x15,  0x3F, 0x3F, 0x3F};

static byte textlut[256];
static unsigned short textbuf[TCOLS * TROWS];  // what the card shows now
static int cgacard;                             // -cga: real CGA, no BIOS blink call
static int rowofs[TROWS * 2];                   // backbuffer offset of each sample row
static int colofs[TCOLS];

#ifdef MODE_VT100
//--------------------------------------------------------------------------
// Serial VT100 terminal: -com 1..4 (default 1), -baud N (default 38400).
// Only changed characters are sent, and only as many as the port can take
// without waiting, so a slow terminal lags behind instead of slowing the
// game down.
//--------------------------------------------------------------------------

static int vtport;
static byte vtshown[TROWS][TCOLS];      // what the terminal shows
static char vtq[256];
static int vtqhead, vtqtail;
static int vtpos;                       // next cell to check
static int vtcurx = -1, vtcury = -1;

static void VT_Put(const char *str)
{
	while (*str)
	{
		vtq[vtqtail] = *str++;
		vtqtail = (vtqtail + 1) & 255;
	}
}

static void VT_Init(void)
{
	static const int ports[4] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};
	int p, com = 1, baud = 38400, div;

	p = M_CheckParm("-com");
	if (p && p < myargc - 1)
		com = atoi(myargv[p + 1]);
	if (com < 1 || com > 4)
		com = 1;
	p = M_CheckParm("-baud");
	if (p && p < myargc - 1)
		baud = atoi(myargv[p + 1]);
	if (baud < 300 || baud > 115200)
		baud = 38400;
	vtport = ports[com - 1];
	div = 115200 / baud;
	outp(vtport + 1, 0);                 // no interrupts
	outp(vtport + 3, 0x80);              // divisor latch
	outp(vtport + 0, div & 0xff);
	outp(vtport + 1, div >> 8);
	outp(vtport + 3, 0x03);              // 8N1
	outp(vtport + 2, 0xc7);              // FIFO on (16550)
	outp(vtport + 4, 0x03);              // DTR, RTS
	memset(vtshown, ' ', sizeof(vtshown));
	VT_Put("\x1b[2J\x1b[H");
}

static void VT_Send(void)
{
	int n;

	// the port takes 16 bytes at a time when its FIFO is empty
	while (vtqhead != vtqtail && (inp(vtport + 5) & 0x20))
	{
		for (n = 0; n < 16 && vtqhead != vtqtail; n++)
		{
			outp(vtport, vtq[vtqhead]);
			vtqhead = (vtqhead + 1) & 255;
		}
	}
}

// Queue changed cells while there is room in the queue
static void VT_Update(void)
{
	int checked, x, y;
	char buf[16];
	byte c;

	VT_Send();
	for (checked = 0; checked < TROWS * TCOLS; checked++)
	{
		if (((vtqtail - vtqhead) & 255) > 200)
			break;
		y = vtpos / TCOLS;
		x = vtpos % TCOLS;
		vtpos = (vtpos + 1) % (TROWS * TCOLS);
		c = (byte)textbuf[y * TCOLS + x];
		if (c == vtshown[y][x])
			continue;
		vtshown[y][x] = c;
		if (y != vtcury || x != vtcurx)
		{
			sprintf(buf, "\x1b[%d;%dH", y + 1, x + 1);
			VT_Put(buf);
		}
		buf[0] = c;
		buf[1] = 0;
		VT_Put(buf);
		vtcury = y;
		vtcurx = x + 1;
		if (vtcurx >= TCOLS)
			vtcurx = -1;            // terminals differ at the right edge
	}
	VT_Send();
}
#endif

static void FHT_InitGraphics(void)
{
	union REGS regs;
	int i, sub;

	cgacard = M_CheckParm("-cga");
#ifdef MODE_VT100
	VT_Init();
#endif
#if defined(MODE_MDA) || defined(MODE_CMDA)
	regs.w.ax = 0x07;
	int386(0x10, &regs, &regs);
#else
	regs.w.ax = (TCOLS == 40) ? 0x01 : 0x03;
	int386(0x10, &regs, &regs);
	if (TROWS > 25)
	{ // 8x8 font: 43 rows on EGA, 50 on VGA
		regs.w.ax = 0x1112;
		regs.w.bx = 0;
		int386(0x10, &regs, &regs);
	}
#endif
	regs.h.ah = 0x01;                   // hide the cursor
	regs.h.ch = 0x3F;
	int386(0x10, &regs, &regs);
#if defined(MODE_MDA) || defined(MODE_CMDA)
	outp(0x3B8, 0x09);                  // no blink: 16 background shades
#else
	if (cgacard)
		outp(0x3D8, TCOLS == 40 ? 0x08 : 0x09);
	else
	{
		regs.w.ax = 0x1003;             // no blink: 16 background colours
		regs.w.bx = 0;
		int386(0x10, &regs, &regs);
	}
#endif
	// sample the middle of each block
	sub = TROWS * (THALF ? 2 : 1);
	for (i = 0; i < sub; i++)
		rowofs[i] = ((i * SCREENHEIGHT + SCREENHEIGHT / 2) / sub) * SCREENWIDTH;
	for (i = 0; i < TCOLS; i++)
		colofs[i] = (i * SCREENWIDTH + SCREENWIDTH / 2) / TCOLS;
	memset(textbuf, 0xff, sizeof(textbuf));
}

static void FHT_SetPalette(byte *palette)
{
	int i, r, g, b;

	for (i = 0; i < 256; i++, palette += 3)
	{
		r = gammatable[usegamma][palette[0]] >> 2;
		g = gammatable[usegamma][palette[1]] >> 2;
		b = gammatable[usegamma][palette[2]] >> 2;
#if defined(MODE_VT100)
		{ // ASCII shades by brightness
			static const char shades[] = " .:-=+*#%@";
			int l = (r * 30 + g * 59 + b * 11) / 100;   // 0..63
			textlut[i] = shades[(l * 10) / 64];
		}
#elif defined(MODE_MDA)
		{ // shade character by brightness: space, light, medium, dark, full
			static const byte shades[5] = {0x20, 0xB0, 0xB1, 0xB2, 0xDB};
			int l = (r * 30 + g * 59 + b * 11) / 100;   // 0..63
			textlut[i] = shades[(l * 5) / 64];
		}
#else
		textlut[i] = GetClosestColor(textcolors, 16, r, g, b);
#endif
	}
	memset(textbuf, 0xff, sizeof(textbuf));     // redraw everything
}

static void FHT_Update(void)
{
	int x, y;
	unsigned short cell, *dst, *old;
	byte *top, *bot;

	dst = TVRAM;
	old = textbuf;
	for (y = 0; y < TROWS; y++)
	{
#if THALF
		top = screen + rowofs[y * 2];
		bot = screen + rowofs[y * 2 + 1];
#else
		top = bot = screen + rowofs[y];
#endif
		for (x = 0; x < TCOLS; x++, dst++, old++)
		{
#if defined(MODE_MDA) || defined(MODE_VT100)
			cell = 0x0700 | textlut[top[colofs[x]]];
#elif THALF
			// upper half block: foreground on top, background below
			cell = 0xDF | (textlut[top[colofs[x]]] << 8)
				| (textlut[bot[colofs[x]]] << 12);
#else
			cell = 0xDB | (textlut[top[colofs[x]]] << 8);
#endif
			if (cell != *old)
			{
				*old = cell;
				*dst = cell;
			}
		}
	}
#ifdef MODE_VT100
	VT_Update();
#endif
}

#else // FastDoom graphics modes

void I_ProcessPalette(byte *palette);
void FD_SetPaletteIdx(int numpalette);
void I_FinishUpdate(void);

#if defined(MODE_CGA)
void CGA_InitGraphics(void);
#define FHV_MODEINIT CGA_InitGraphics
#elif defined(MODE_CGA_BW)
void CGA_BW_InitGraphics(void);
#define FHV_MODEINIT CGA_BW_InitGraphics
#elif defined(MODE_CGA16)
void CGA_16_InitGraphics(void);
#define FHV_MODEINIT CGA_16_InitGraphics
#elif defined(MODE_CGA512)
void CGA_512_InitGraphics(void);
#define FHV_MODEINIT CGA_512_InitGraphics
#elif defined(MODE_CGA_AFH)
void CGA_AFH_InitGraphics(void);
#define FHV_MODEINIT CGA_AFH_InitGraphics
#elif defined(MODE_CVB)
void CGA_CVBS_InitGraphics(void);
#define FHV_MODEINIT CGA_CVBS_InitGraphics
#elif defined(MODE_EGA)
void EGA_InitGraphics(void);
#define FHV_MODEINIT EGA_InitGraphics
#elif defined(MODE_HERC)
void HERC_InitGraphics(void);
void HERC_ShutdownGraphics(void);
#define FHV_MODEINIT HERC_InitGraphics
#define FHV_MODEDONE HERC_ShutdownGraphics
#elif defined(MODE_INCOLOR)
void InColor_InitGraphics(void);
void InColor_ShutdownGraphics(void);
#define FHV_MODEINIT InColor_InitGraphics
#define FHV_MODEDONE InColor_ShutdownGraphics
#elif defined(MODE_PCP)
void PCP_InitGraphics(void);
#define FHV_MODEINIT PCP_InitGraphics
#elif defined(MODE_SIGMA)
void Sigma_InitGraphics(void);
#define FHV_MODEINIT Sigma_InitGraphics
#else
#error FH_BACKBUF needs a MODE_ define
#endif

#endif // FH_TEXT

int GetClosestColor(const byte *colors, int num_colors, int r1, int g1, int b1)
{
	int i, result, distance, best;
	int dr, dg, db;

	result = 0;
	best = 0x7fffffff;
	for (i = 0; i < num_colors; i++, colors += 3)
	{
		dr = colors[0] - r1;
		dg = colors[1] - g1;
		db = colors[2] - b1;
		distance = dr * dr + dg * dg + db * db;
		if (distance == 0)
			return i;
		if (distance < best)
		{
			best = distance;
			result = i;
		}
	}
	return result;
}

void FHV_Init(void)
{
	CGApalette1 = M_CheckParm("-palette1");
	snowfix = M_CheckParm("-snow");
	if (M_CheckParm("-oldcga"))
		CGAmodel = 0;   // CGA_OLD
	screen = backbuffer;
#ifdef FH_TEXT
	FHT_InitGraphics();
#else
	FHV_MODEINIT();
#endif
}

void FHV_SetPalette(byte *palette)
{
	int i;

#ifdef FH_TEXT
	FHT_SetPalette(palette);
#else
	for (i = 0; i < 256; i++)
		fd_gamma[i] = gammatable[usegamma][i] >> 2;
	I_ProcessPalette(palette);
	FD_SetPaletteIdx(0);
#endif
}

void FHV_Update(void)
{
#ifdef FH_TEXT
	FHT_Update();
#else
	I_FinishUpdate();
#endif
	UpdateState = I_NOUPDATE;
#ifdef FHV_DEBUG
	{ // dump video memory on the 200th frame, to check the output
		static int frames;
		if (++frames == 200)
		{
			FILE *f = fopen("VRAM.RAW", "wb");
			FILE *g = fopen("GCREGS.TXT", "w");
			int p;
			for (p = 0; p < 9; p++)
			{
				outp(0x3ce, p);
				fprintf(g, "GC%d=%02x\n", p, inp(0x3cf));
			}
			fclose(g);
			for (p = 0; p < 4; p++)
			{
				outp(0x3ce, 4);
				outp(0x3cf, p);
				fwrite((byte *)0xA0000, 1, 0x10000, f);
			}
			fwrite((byte *)0xB0000, 1, 0x10000, f);
			fclose(f);
		}
	}
#endif
}

void FHV_Shutdown(void)
{
	union REGS regs;

#ifdef FHV_MODEDONE
	FHV_MODEDONE();
#endif
	regs.w.ax = 3;
	int386(0x10, &regs, &regs);
}

#endif // FH_BACKBUF
