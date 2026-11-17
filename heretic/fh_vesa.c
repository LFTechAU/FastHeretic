//
// fh_vesa.c : FastHeretic VESA 320x200 256 colour modes
//
// FHTICVBR.EXE copies the screen buffer to a VESA mode (linear frame
// buffer when there is one). FHTICVBD.EXE draws straight into a linear
// frame buffer with 3 pages, like Mode Y but without planes.
// Ideas from FastDoom's i_vesa.c (Victor Nieto, GPL).
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#if defined(FH_VBR) || defined(FH_VBD)

#include <string.h>
#include <dos.h>
#include <conio.h>
#include "doomdef.h"
#include "r_local.h"

// DPMI real mode call structure
typedef struct
{
	unsigned EDI, ESI, EBP, reserved, EBX, EDX, ECX, EAX;
	unsigned short flags, ES, DS, FS, GS, IP, CS, SP, SS;
} rmregs_t;

static rmregs_t rm;
static unsigned short dosseg;    // 1 KB of DOS memory for VBE blocks
static byte *dosbuf;

byte *fhe_lfb;                   // the frame buffer (linear or A0000)
int fhe_pitch = SCREENWIDTH;     // bytes per scan line
int fhe_pages = 1;               // pages that fit in video memory
int fhe_linear;                  // using a linear frame buffer
static int fhe_mode;

static void RMInt10(void)
{
	union REGS regs;
	struct SREGS sregs;

	memset(&regs, 0, sizeof(regs));
	segread(&sregs);
	regs.w.ax = 0x0300;
	regs.h.bl = 0x10;
	sregs.es = sregs.ds;
	regs.x.edi = (unsigned)&rm;
	int386x(0x31, &regs, &regs, &sregs);
}

static int VBE_Call(unsigned ax, unsigned bx, unsigned cx, unsigned dx,
	int usebuf, unsigned off)
{
	memset(&rm, 0, sizeof(rm));
	rm.EAX = ax;
	rm.EBX = bx;
	rm.ECX = cx;
	rm.EDX = dx;
	if (usebuf)
	{
		rm.ES = dosseg;
		rm.EDI = off;
	}
	RMInt10();
	return (rm.EAX & 0xffff) == 0x004f;
}

static void *MapPhysical(unsigned phys, unsigned size)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0800;
	regs.w.bx = phys >> 16;
	regs.w.cx = phys & 0xffff;
	regs.w.si = size >> 16;
	regs.w.di = size & 0xffff;
	int386(0x31, &regs, &regs);
	if (regs.x.cflag)
		return NULL;
	return (void *)(((unsigned)regs.w.bx << 16) | regs.w.cx);
}

#define W(p, o) (*(unsigned short *)((byte *)(p) + (o)))
#define D(p, o) (*(unsigned *)((byte *)(p) + (o)))

// Set up a VESA w x h x 8 mode. Returns false if there is none, in which
// case plain mode 13h is set instead.
static int fhe_h = 200;
static unsigned fhe_winseg = 0xA000;
static int fhe_gran = 64;        // window granularity in KB
static int fhe_bank = -1;
static void SetBank(int bank);

int FHE_Init(int w, int h, int wantpages)
{
	union REGS regs;
	unsigned short modes[256];
	int i, n, best;
	unsigned seg, off;
	byte *info;

	fhe_lfb = (byte *)0xA0000;
	fhe_pitch = SCREENWIDTH;
	fhe_pages = 1;
	fhe_linear = 0;
	fhe_mode = 0x13;
	fhe_h = h;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0100;             // DPMI: allocate DOS memory
	regs.w.bx = 1024 / 16;
	int386(0x31, &regs, &regs);
	if (regs.x.cflag)
		goto fallback;
	dosseg = regs.w.ax;
	dosbuf = (byte *)((unsigned)dosseg << 4);

	memset(dosbuf, 0, 1024);
	memcpy(dosbuf, "VBE2", 4);
	if (!VBE_Call(0x4f00, 0, 0, 0, true, 0) || memcmp(dosbuf, "VESA", 4))
		goto fallback;

	// copy the mode list, the mode info calls may overwrite it
	seg = W(dosbuf, 0x10);
	off = W(dosbuf, 0x0e);
	for (n = 0; n < 255; n++)
	{
		modes[n] = *(unsigned short *)((seg << 4) + off + n * 2);
		if (modes[n] == 0xffff)
			break;
	}

	best = -1;
	info = dosbuf + 512;
	for (i = 0; i < n; i++)
	{
		memset(info, 0, 256);
		if (!VBE_Call(0x4f01, 0, modes[i], 0, true, 512))
			continue;
		if (!(W(info, 0) & 1) || !(W(info, 0) & 0x10))
			continue;                   // not supported or not graphics
		if (W(info, 0x12) != w || W(info, 0x14) != h || info[0x19] != 8)
			continue;
		if (info[0x1b] != 4)
			continue;                   // not packed pixel
		best = modes[i];
		if ((W(info, 0) & 0x80) && D(info, 0x28))
			break;                      // has a linear frame buffer: take it
	}
	if (best < 0)
		goto fallback;

	VBE_Call(0x4f01, 0, best, 0, true, 512);
	fhe_mode = best;
	fhe_pitch = W(info, 0x10);
	fhe_pages = info[0x1d] + 1;
	fhe_gran = W(info, 0x04) ? W(info, 0x04) : 64;
	fhe_winseg = W(info, 0x08) ? W(info, 0x08) : 0xA000;
	if ((W(info, 0) & 0x80) && D(info, 0x28) && !M_CheckParm("-noLFB"))
	{
		byte *p = MapPhysical(D(info, 0x28),
			(fhe_pitch * h * (wantpages > 1 ? wantpages : 1) + 0xffff)
			& ~0xffff);
		if (p && VBE_Call(0x4f02, best | 0x4000, 0, 0, false, 0))
		{
			fhe_lfb = p;
			fhe_linear = 1;
			return true;
		}
	}
	// banked: copies switch the 64 KB window as they go
	if (VBE_Call(0x4f02, best, 0, 0, false, 0))
	{
		fhe_lfb = (byte *)(fhe_winseg << 4);
		fhe_bank = -1;
		fhe_pages = 1;
		SetBank(0);
		return true;
	}

fallback:
	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x13;
	int386(0x10, &regs, &regs);
	fhe_lfb = (byte *)0xA0000;
	fhe_pitch = SCREENWIDTH;
	fhe_pages = 1;
	fhe_linear = 0;
	fhe_mode = 0x13;
	return false;
}

// Show page n
void FHE_ShowPage(int n, int waitvsync)
{
	if (fhe_mode == 0x13)
		return;
	VBE_Call(0x4f07, waitvsync ? 0x80 : 0, 0, n * fhe_h, false, 0);
}

static void SetBank(int bank)
{
	if (bank == fhe_bank)
		return;
	fhe_bank = bank;
	VBE_Call(0x4f05, 0, 0, bank * (64 / fhe_gran), false, 0);
}

// Copy a rectangle of a buffer (srcpitch bytes per line) to the screen
void FHE_CopyRect(byte *src, int srcpitch, int x, int y, int w, int h)
{
	unsigned off, chunk;
	int len;
	byte *s;

	if (w <= 0 || h <= 0)
		return;
	src += y * srcpitch + x;
	if (fhe_linear || fhe_mode == 0x13)
	{
		byte *d = fhe_lfb + y * fhe_pitch + x;
		if (w == srcpitch && w == fhe_pitch)
		{
			memcpy(d, src, w * h);
			return;
		}
		for (; h > 0; h--, src += srcpitch, d += fhe_pitch)
			memcpy(d, src, w);
		return;
	}
	for (; h > 0; h--, y++, src += srcpitch)
	{
		off = y * fhe_pitch + x;
		s = src;
		for (len = w; len > 0; len -= chunk, off += chunk, s += chunk)
		{
			chunk = 0x10000 - (off & 0xffff);
			if (chunk > len)
				chunk = len;
			SetBank(off >> 16);
			memcpy(fhe_lfb + (off & 0xffff), s, chunk);
		}
	}
}

// Copy rows of a 320 wide buffer to the screen
void FHE_CopyRows(byte *dest, byte *src, int y, int h)
{
	FHE_CopyRect(src, SCREENWIDTH, 0, y, SCREENWIDTH, h);
}

#endif
