// fdscompat.h : what FastDoom's sound code (ns_*.c) needs from the game,
// for FastHeretic. The sound code is compiled with only fdsnd/ and
// fdsnd/compat/ on the include path, so FastDoom's names don't clash with
// Heretic's.
#ifndef __FDSCOMPAT__
#define __FDSCOMPAT__

#include <stddef.h>

typedef unsigned char byte;
#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum {false, true} boolean;
#endif


#define PU_STATIC 1
#define PU_CACHE 101
void *Z_Malloc(int size, int tag, void *user);
void Z_Free(void *ptr);
#define Z_MallocUnowned(size, tag) Z_Malloc(size, tag, NULL)

// FastDoom's numbered error messages
void FDS_Error(int code, ...);
#define I_Error FDS_Error

int M_CheckParm(char *check);

extern int ignoreSoundChecks;
extern int numChannels;
extern int reverseStereo;
extern char *sbkfile;
extern int snd_Rate;
extern int snd_PCMRate;
extern int snd_GoldIrq;
extern int snd_GoldDma;
extern int snd_MidiDevice;


#include "fastmath.h"
#endif
