// r_fh.h : FastHeretic renderer options and drawers
#ifndef __R_FH_H__
#define __R_FH_H__

extern int fh_detail;        // 0 = high, 1 = low, 2 = potato
extern int fh_flatsurfaces;  // 0 textured, 1 flat lit by distance, 2 flat
extern int fh_flatsky;       // single-colour sky
extern int fh_notrans;       // ghosts: 0 see-thru, 1 solid, 2 saturn
extern int fh_showfps;       // frame rate counter
extern int fh_near;          // only draw nearby non-monster sprites
extern int fh_vsync;         // wait for vertical retrace before blits
extern int fh_csv;           // append timedemo results to FHBENCH.CSV

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
