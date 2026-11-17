//
// fh_uncap.c : FastHeretic uncapped frame rate
//
// The game still runs at 35 tics a second, but frames are drawn as fast as
// the machine can go. Each frame shows the world part way between the last
// two tics: the player's view, things and moving floors and ceilings slide
// smoothly instead of jumping 35 times a second. Idea from FastDoom.
//
// Nothing is added to mobj_t or sector_t, so saved games stay compatible
// with the original HERETIC.EXE. The positions from the previous tic are
// kept in side tables, and while a frame is drawn the real positions are
// swapped for the in-between ones and put back afterwards.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <stdlib.h>
#include <i86.h>
#include "doomdef.h"
#include "p_local.h"
#include "r_local.h"

int fh_uncapped;                 // option: draw frames between tics
int fh_uncapnow;                 // uncapped timing is running
volatile int ticcount_hr;        // 560 Hz, 16 per tic, while uncapped
fixed_t fh_interpweight = FRACUNIT;

#define MAXINTERP 2048
#define SNAPDIST (64 * FRACUNIT) // further than this in one tic: teleported

typedef struct
{
	mobj_t *mo;
	int type;
	fixed_t px, py, pz;          // last tic
	angle_t pangle;
	fixed_t cx, cy, cz;          // current tic, while swapped
	angle_t cangle;
	int active;
} interp_t;

static interp_t interp[MAXINTERP];
static int numinterp;
static fixed_t *prevfloor, *prevceil, *curfloor, *curceil;
static int interpsectors;
static int interpvalid;
static fixed_t prevviewz, curviewz;
static int swapped;

// Called when a level starts: the old positions mean nothing now
void FH_ResetInterpolation(void)
{
	interpvalid = 0;
	numinterp = 0;
}

// Called before each tic runs: remember where everything is now
void FH_SaveInterpolation(void)
{
	int i;
	sector_t *sec;
	mobj_t *mo;
	interp_t *ip;

	if (!fh_uncapnow || gamestate != GS_LEVEL)
		return;
	if (interpsectors < numsectors)
	{
		free(prevfloor); free(prevceil); free(curfloor); free(curceil);
		prevfloor = malloc(numsectors * sizeof(fixed_t));
		prevceil = malloc(numsectors * sizeof(fixed_t));
		curfloor = malloc(numsectors * sizeof(fixed_t));
		curceil = malloc(numsectors * sizeof(fixed_t));
		if (!prevfloor || !prevceil || !curfloor || !curceil)
			I_Error("FH_SaveInterpolation: out of memory");
		interpsectors = numsectors;
	}
	ip = interp;
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		prevfloor[i] = sec->floorheight;
		prevceil[i] = sec->ceilingheight;
		for (mo = sec->thinglist; mo && ip < interp + MAXINTERP; mo = mo->snext)
		{
			ip->mo = mo;
			ip->type = mo->type;
			ip->px = mo->x;
			ip->py = mo->y;
			ip->pz = mo->z;
			ip->pangle = mo->angle;
			ip++;
		}
	}
	numinterp = ip - interp;
	prevviewz = players[displayplayer].viewz;
	interpvalid = 1;
}

static fixed_t Lerp(fixed_t a, fixed_t b, fixed_t w)
{
	return a + FixedMul(b - a, w);
}

// Before drawing: move everything to where it is part way through the tic
void FH_InterpolateBegin(void)
{
	int i;
	sector_t *sec;
	interp_t *ip;
	mobj_t *mo;
	fixed_t w = fh_interpweight;
	player_t *pl = &players[displayplayer];

	swapped = 0;
	if (!fh_uncapnow || !interpvalid || w >= FRACUNIT
		|| interpsectors < numsectors)
		return;
	swapped = 1;
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		curfloor[i] = sec->floorheight;
		curceil[i] = sec->ceilingheight;
		sec->floorheight = Lerp(prevfloor[i], curfloor[i], w);
		sec->ceilingheight = Lerp(prevceil[i], curceil[i], w);
	}
	for (i = 0, ip = interp; i < numinterp; i++, ip++)
	{
		mo = ip->mo;
		ip->active = 0;
		// the thing may have been removed and its memory reused this tic
		if (mo->thinker.function != P_MobjThinker || mo->type != ip->type)
			continue;
		if (abs(mo->x - ip->px) > SNAPDIST || abs(mo->y - ip->py) > SNAPDIST
			|| abs(mo->z - ip->pz) > SNAPDIST)
			continue;
		ip->active = 1;
		ip->cx = mo->x;
		ip->cy = mo->y;
		ip->cz = mo->z;
		ip->cangle = mo->angle;
		mo->x = Lerp(ip->px, ip->cx, w);
		mo->y = Lerp(ip->py, ip->cy, w);
		mo->z = Lerp(ip->pz, ip->cz, w);
		if (mo == pl->mo)
			mo->angle = ip->pangle + FixedMul((int)(ip->cangle - ip->pangle), w);
	}
	curviewz = pl->viewz;
	if (pl->mo && abs(curviewz - prevviewz) < SNAPDIST)
		pl->viewz = Lerp(prevviewz, curviewz, w);
}

// After drawing: put everything back
void FH_InterpolateEnd(void)
{
	int i;
	sector_t *sec;
	interp_t *ip;

	if (!swapped)
		return;
	swapped = 0;
	for (i = 0, sec = sectors; i < numsectors; i++, sec++)
	{
		sec->floorheight = curfloor[i];
		sec->ceilingheight = curceil[i];
	}
	for (i = 0, ip = interp; i < numinterp; i++, ip++)
	{
		if (!ip->active)
			continue;
		ip->mo->x = ip->cx;
		ip->mo->y = ip->cy;
		ip->mo->z = ip->cz;
		ip->mo->angle = ip->cangle;
	}
	players[displayplayer].viewz = curviewz;
}

// Switch the timer between 35 Hz and 560 Hz
void FH_SetUncapped(int on)
{
	extern int ticcount;
	void I_SetTimerRate(int rate);

	if (on == fh_uncapnow)
		return;
	_disable();
	if (on)
		ticcount_hr = ticcount << 4;
	fh_uncapnow = on;
	_enable();
	I_SetTimerRate(on ? 560 : 35);
	FH_ResetInterpolation();
}
