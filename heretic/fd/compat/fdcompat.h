// fdcompat.h : lets FastDoom's video mode files build inside FastHeretic
#ifndef __FDCOMPAT__
#define __FDCOMPAT__

#include <string.h>

typedef unsigned char byte;
typedef int boolean;
#ifndef true
#define true 1
#define false 0
#endif

#ifndef SCREENWIDTH
#define SCREENWIDTH 320
#define SCREENHEIGHT 200
#endif

#define FD_NUMPAL 1              // one palette is processed at a time

extern byte backbuffer[SCREENWIDTH * SCREENHEIGHT];
extern byte *pcscreen, *destscreen;
extern unsigned short *currentscreen;
extern byte fd_gamma[256];       // current gamma, 0..63
#define gammatable fd_gamma

extern boolean CGApalette1;      // -palette1
extern boolean snowfix;          // -snow

int GetClosestColor(const byte *colors, int num_colors, int r1, int g1, int b1);
void I_Error(char *error, ...);

#define SetDWords(dest, value, n) memset((dest), (value), (n) * 4)
#define CopyDWords(src, dest, n) memcpy((void *)(dest), (src), (n) * 4)

// CGA 512 colour mode: which IBM CGA (-oldcga, default new)
enum CGA_MODEL { CGA_OLD, CGA_NEW };
extern unsigned char CGAmodel;
#ifndef MAXINT
#define MAXINT 0x7fffffff
#endif
#endif
