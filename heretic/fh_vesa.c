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

// Set up a VESA 320x200x8 mode. Returns false if there is none, in which
// case plain mode 13h is set instead.
int FHE_Init(int wantpages)
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
		if (W(info, 0x12) != 320 || W(info, 0x14) != 200 || info[0x19] != 8)
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
	if ((W(info, 0) & 0x80) && D(info, 0x28) && !M_CheckParm("-noLFB"))
	{
		byte *p = MapPhysical(D(info, 0x28),
			(fhe_pitch * 200 * (wantpages > 1 ? wantpages : 1) + 0xffff)
			& ~0xffff);
		if (p && VBE_Call(0x4f02, best | 0x4000, 0, 0, false, 0))
		{
			fhe_lfb = p;
			fhe_linear = 1;
			return true;
		}
	}
	// banked: 320x200 fits in the first 64 KB window
	if (fhe_pitch * 200 <= 0x10000
		&& VBE_Call(0x4f02, best, 0, 0, false, 0))
	{
		VBE_Call(0x4f05, 0, 0, 0, false, 0);
		fhe_pages = 1;
		return true;
	}

fallback:
	regs.w.ax = 0x13;
	int386(0x10, &regs, &regs);
	fhe_lfb = (byte *)0xA0000;
	fhe_pitch = SCREENWIDTH;
	fhe_pages = 1;
	fhe_linear = 0;
	fhe_mode = 0x13;
	return false;
}

// Show page n (scan line n*200)
void FHE_ShowPage(int n, int waitvsync)
{
	if (fhe_mode == 0x13)
		return;
	VBE_Call(0x4f07, waitvsync ? 0x80 : 0, 0, n * 200, false, 0);
}

// Copy rows of a 320 wide buffer into the frame buffer page
void FHE_CopyRows(byte *dest, byte *src, int y, int h)
{
	if (fhe_pitch == SCREENWIDTH)
	{
		memcpy(dest + y * SCREENWIDTH, src + y * SCREENWIDTH,
			h * SCREENWIDTH);
		return;
	}
	for (; h > 0; h--, y++)
		memcpy(dest + y * fhe_pitch, src + y * SCREENWIDTH, SCREENWIDTH);
}

#endif
