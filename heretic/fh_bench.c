//
// fh_bench.c : FastHeretic benchmark mode
//
//   FHTIC -bench QUICK.BNC [demo] [-advanced]
//
// Plays the demo (demo1 if none is given) as a timedemo once for each
// line of the .BNC file, with that line's settings, then shows all the
// results together. Each run is also added to FHBENCH.CSV, and the
// whole table is written to FHBENCH.TXT. -advanced adds the 1% and 0.1%
// low frame rates (the slowest frames).
//
// The .BNC files are FastDoom's: a first line with the number of runs,
// then one run per line:
//
//   detail,size,floors,walls,sprites,psprites,sky,ghosts,items,fps,
//   uncapped,melt,cpu,bus
//
//   detail    high, low, potato
//   size      0 to 8, FastDoom's screen size (7 is Heretic's size 10)
//   floors    default, flat (one colour), flatter (lit by distance)
//   sky       default, flat
//   ghosts    default, flat (solid), saturn, flatsaturn, translucent
//   items     far, near
//   fps       nofps, fps
//   cpu       auto, 386sx, 386dx, i486, pentium, k5, cy5x86 ... as the
//             command line options
//
// walls, sprites, psprites, uncapped, melt and bus are read but FastHeretic
// has nothing to change for them.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "doomdef.h"
#include "r_local.h"
#include "fh_cpu.h"

#define MAXRUNS   64
#define MAXFRAMES 16384

typedef struct
{
	char line[128];
	int gametics, realtics;
	int fps100;                 // frames a second x 100
	int low1, low01;            // 1% and 0.1% low, x 100
} run_t;

int fh_bench;                   // benchmark mode on
int fh_benchnext;               // start the next run at the top of the loop

static run_t runs[MAXRUNS];
static int numruns, currun;
static char benchfile[64];
static char benchdemo[16] = "demo1";
static int advanced;
static int starttic;

static unsigned short *frametimes;
static int numframes;
static int lastframe;

extern int screenblocks;
extern boolean singletics, timingdemo, demoplayback;
extern int starttime;
extern byte *demobuffer, *demo_p;
void FH_SetUncapped(int on);
void G_InitNew(skill_t skill, int episode, int map);
int I_GetTime(void);

// The program's name, for telling the video modes apart in FHBENCH.CSV
static char *ExeName(void)
{
	static char name[16];
	char *p = myargv[0], *s;

	if ((s = strrchr(p, '\\')) != NULL)
		p = s + 1;
	if ((s = strrchr(p, '/')) != NULL)
		p = s + 1;
	strncpy(name, p, sizeof(name) - 1);
	if ((s = strchr(name, '.')) != NULL)
		*s = 0;
	return name;
}

static int Is(char *a, char *b)
{
	return !stricmp(a, b);
}

static void Apply(char *line)
{
	char buf[128], *tok;
	int col = 0, i;
	static const struct { char *name; int cpu; } cpus[] = {
		{"auto", CPU_AUTO}, {"386sx", CPU_I386SX}, {"386dx", CPU_I386DX},
		{"i486", CPU_I486}, {"pentium", CPU_P5}, {"pentiumP54CS", CPU_P54CS},
		{"pentiumMMX", CPU_PMMX}, {"pentiumII", CPU_PII}, {"k5", CPU_K5},
		{"k6", CPU_K6}, {"cy386", CPU_CY386}, {"cy486", CPU_CY486},
		{"cy5x86", CPU_CY5X86}, {"cy6x86", CPU_CY6X86},
		{"cy6x86mx", CPU_CY6X86MX}, {"umc486", CPU_UMC486},
		{"winchip", CPU_WINCHIP}, {"mp6", CPU_MP6},
	};

	strncpy(buf, line, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;
	for (tok = strtok(buf, ",\r\n"); tok; tok = strtok(NULL, ",\r\n"), col++)
	{
		while (*tok == ' ')
			tok++;
		switch (col)
		{
		case 0:
			if (Is(tok, "high")) fh_detail = 0;
			if (Is(tok, "low")) fh_detail = 1;
			if (Is(tok, "potato")) fh_detail = 2;
			break;
		case 1:
			// FastDoom's screen size 0-8 is Heretic's 3-11
			i = atoi(tok) + 3;
			if (i >= 3 && i <= 11)
				screenblocks = i;
			break;
		case 2:
			if (Is(tok, "default")) fh_flatsurfaces = 0;
			if (Is(tok, "flat")) fh_flatsurfaces = 2;
			if (Is(tok, "flatter")) fh_flatsurfaces = 1;
			break;
		case 6:
			if (Is(tok, "default")) fh_flatsky = 0;
			if (Is(tok, "flat")) fh_flatsky = 1;
			break;
		case 7:
			if (Is(tok, "default") || Is(tok, "translucent")) fh_notrans = 0;
			if (Is(tok, "flat")) fh_notrans = 1;
			if (Is(tok, "saturn") || Is(tok, "flatsaturn")) fh_notrans = 2;
			break;
		case 8:
			if (Is(tok, "far")) fh_near = 0;
			if (Is(tok, "near")) fh_near = 1;
			break;
		case 9:
			if (Is(tok, "nofps")) fh_showfps = 0;
			if (Is(tok, "fps")) fh_showfps = 1;
			break;
		case 12:
			for (i = 0; i < sizeof(cpus) / sizeof(cpus[0]); i++)
				if (Is(tok, cpus[i].name))
					fh_cpu = cpus[i].cpu;
			FH_SelectCPU();
			break;
		}
	}
	R_SetViewSize(screenblocks, fh_detail);
}

static void StartRun(void)
{
	skill_t skill;
	int episode, map, i;

	Apply(runs[currun].line);
	if (demobuffer)
		Z_ChangeTag(demobuffer, PU_CACHE);
	demobuffer = demo_p = W_CacheLumpName(benchdemo, PU_STATIC);
	skill = *demo_p++;
	episode = *demo_p++;
	map = *demo_p++;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		playeringame[i] = *demo_p++;
#ifdef FH_HEXEN
		PlayerClass[i] = *demo_p++;
#endif
	}
#ifdef FH_HEXEN
	G_StartNewInit();
#endif
	G_InitNew(skill, episode, map);     // also sets starttime
	usergame = false;
	demoplayback = true;
	timingdemo = true;
	singletics = true;
	starttic = gametic;
	numframes = 0;
	lastframe = ticcount_hr;
}

// -bench: read the .BNC file. Returns 1 when benchmark mode is on.
int FH_BenchInit(void)
{
	FILE *f;
	char buf[256];
	int p = M_CheckParm("-bench");

	if (!p || p >= myargc - 1)
		return 0;
	strncpy(benchfile, myargv[p + 1], sizeof(benchfile) - 1);
	if (p < myargc - 2 && myargv[p + 2][0] != '-')
		strncpy(benchdemo, myargv[p + 2], sizeof(benchdemo) - 1);
	advanced = M_CheckParm("-advanced");
	f = fopen(benchfile, "r");
	if (!f)
		I_Error("Can't open the benchmark file %s", benchfile);
	while (numruns < MAXRUNS && fgets(buf, sizeof(buf), f))
	{
		// the first line (the number of runs) has no commas
		if (!strchr(buf, ','))
			continue;
		buf[strcspn(buf, "\r\n")] = 0;
		strncpy(runs[numruns].line, buf, sizeof(runs[0].line) - 1);
		numruns++;
	}
	fclose(f);
	if (!numruns)
		I_Error("No runs in %s", benchfile);
	if (W_CheckNumForName(benchdemo) < 0)
		I_Error("No demo %s in the WAD", benchdemo);
	frametimes = Z_Malloc(MAXFRAMES * sizeof(*frametimes), PU_STATIC, NULL);
	fh_bench = 1;
	return 1;
}

// Called instead of the first G_TimeDemo
void FH_BenchStart(void)
{
	FH_SetUncapped(1);          // 560 Hz timer for frame times
	currun = 0;
	StartRun();
}

// Called after each frame is shown
void FH_BenchFrame(void)
{
	int now = ticcount_hr;

	if (!fh_bench || !timingdemo)
		return;
	if (numframes < MAXFRAMES)
		frametimes[numframes++] = now - lastframe;
	lastframe = now;
}

static int CompareDesc(const void *a, const void *b)
{
	return *(unsigned short *)b - *(unsigned short *)a;
}

// Average frame rate of the slowest frames (x 100)
static int Low(int div)
{
	int n = numframes / div, i, sum = 0;

	if (n < 1)
		n = 1;
	if (n > numframes)
		return 0;
	for (i = 0; i < n; i++)
		sum += frametimes[i];
	if (!sum)
		return 0;
	return 56000 * n / sum;     // 560 Hz ticks
}

static void Finish(void)
{
	static char text[MAXRUNS * 120 + 400];
	char *t = text;
	FILE *f;
	int i;

	t += sprintf(t, FH_NAME " benchmark: %s, %s, %s, CPU found: %s\n\n",
		ExeName(), benchfile, benchdemo, fh_cpunames[fh_cpudetected]);
	t += sprintf(t, "     fps%s  settings\n", advanced ? "   1% low  0.1% low" : "");
	for (i = 0; i < numruns; i++)
	{
		t += sprintf(t, "%5d.%02d", runs[i].fps100 / 100, runs[i].fps100 % 100);
		if (advanced)
			t += sprintf(t, " %6d.%02d %6d.%02d", runs[i].low1 / 100,
				runs[i].low1 % 100, runs[i].low01 / 100, runs[i].low01 % 100);
		t += sprintf(t, "  %s\n", runs[i].line);
	}
	f = fopen("FHBENCH.TXT", "w");
	if (f)
	{
		fputs(text, f);
		fclose(f);
	}
	I_Error("%s", text);
}

// Called from G_CheckDemoStatus when a timedemo ends
void FH_BenchEnd(void)
{
	run_t *r = &runs[currun];
	FILE *f;

	r->gametics = gametic - starttic;
	r->realtics = I_GetTime() - starttime;
	r->fps100 = r->realtics ? r->gametics * 3500 / r->realtics : 0;
	if (advanced && numframes > 1)
	{
		// the first frame includes loading the level: leave it out
		qsort(frametimes + 1, numframes - 1, sizeof(*frametimes), CompareDesc);
		memmove(frametimes, frametimes + 1, (numframes - 1) * sizeof(*frametimes));
		numframes--;
		r->low1 = Low(100);
		r->low01 = Low(1000);
	}
	f = fopen("FHBENCH.CSV", "a");
	if (f)
	{
		fprintf(f, "%s,%s,%s,%d,%d,%d.%02d,%d.%02d,%d.%02d,%s\n", ExeName(), benchdemo,
			fh_cpunames[fh_cpusel], r->gametics, r->realtics,
			r->fps100 / 100, r->fps100 % 100, r->low1 / 100, r->low1 % 100,
			r->low01 / 100, r->low01 % 100, r->line);
		fclose(f);
	}
	timingdemo = false;
	demoplayback = false;
	if (++currun >= numruns)
		Finish();
	fh_benchnext = 1;
}

// Top of the main loop: start the next run
void FH_BenchNext(void)
{
	fh_benchnext = 0;
	StartRun();
}
