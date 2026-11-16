// r_fh.h : FastHeretic renderer options and drawers
#ifndef __R_FH_H__
#define __R_FH_H__

extern int fh_detail;        // 0 = high, 1 = low, 2 = potato
extern int fh_flatsurfaces;  // untextured floors and ceilings
extern int fh_flatsky;       // single-colour sky
extern int fh_notrans;       // translucent sprites drawn solid
extern int fh_showfps;       // frame rate counter

extern void (*skycolfunc)(void);
extern void (*transcolfunc)(void);
extern void (*transfuzzcolfunc)(void);
extern byte ds_flatcolor;
extern byte skyflatcolor;

void R_FH_SetDrawers(void);
byte R_FlatColor(int flatnum);
void R_ComputeSkyColor(void);

void R_DrawSpanFlat(void);

#endif
