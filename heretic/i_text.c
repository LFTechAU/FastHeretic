// i_text.c : minimal text-mode helpers (replaces Watcom graph.lib calls)
// FastHeretic - GPL

#include <i86.h>
#include "i_text.h"

static short txt_row = 1, txt_col = 1;
static unsigned char txt_fg = 7, txt_bg = 0;

static void SetCursor(void)
{
	union REGS r;
	r.h.ah = 0x02;
	r.h.bh = 0;
	r.h.dh = txt_row - 1;
	r.h.dl = txt_col - 1;
	int386(0x10, &r, &r);
}

void _settextposition(short row, short col)
{
	txt_row = row;
	txt_col = col;
	SetCursor();
}

long _setbkcolor(long color)
{
	long old = txt_bg;
	txt_bg = (unsigned char)(color & 7);
	return old;
}

short _settextcolor(short color)
{
	short old = txt_fg;
	txt_fg = (unsigned char)(color & 15);
	return old;
}

void _outtext(char const *text)
{
	unsigned short *screen = (unsigned short *)0xB8000;
	unsigned short attr = (unsigned short)((txt_bg << 4) | txt_fg) << 8;

	while (*text)
	{
		char c = *text++;
		if (c == '\n')
		{
			txt_col = 1;
			if (txt_row < 25)
				txt_row++;
			continue;
		}
		if (txt_row >= 1 && txt_row <= 25 && txt_col >= 1 && txt_col <= 80)
			screen[(txt_row - 1) * 80 + (txt_col - 1)] = attr | (unsigned char)c;
		if (++txt_col > 80)
		{
			txt_col = 1;
			if (txt_row < 25)
				txt_row++;
		}
	}
	SetCursor();
}

short _settextcursor(short shape)
{
	union REGS r;
	r.h.ah = 0x01;
	r.h.ch = (shape >> 8) & 0xff;
	r.h.cl = shape & 0xff;
	int386(0x10, &r, &r);
	return 0x0607;
}
