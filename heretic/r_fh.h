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
extern int fh_fixdac;        // wait for retrace before palette changes (-fixDAC)

extern void (*skycolfunc)(void);
extern void (*transcolfunc)(void);
extern void (*transfuzzcolfunc)(void);
extern byte ds_flatcolor;
extern byte skyflatcolor;

void R_FH_SetDrawers(void);
byte R_FlatColor(int flatnum);
void R_ComputeSkyColor(void);

void R_DrawSpanFlat(void);

#ifdef FH_BACKBUF
// fhvideo.c
void FHV_Init(void);
void FHV_SetPalette(byte *palette);
void FHV_Update(void);
void FHV_Shutdown(void);
#endif

#ifdef FH_MODEY
// fh_modey.c
extern byte *fh_planar;      // set: V_Draw* go straight into this page
extern int fh_directframe;   // this frame's view was drawn into VRAM
void FHY_InitGraphics(void);
void R_FHY_InitTables(void);
void R_FHY_SetupFrame(void);
void R_FHY_SetDrawers(void);
void FHY_DrawPatch(int x, int y, patch_t *patch, int mode);
void FHY_Composite(void);
void FHY_ReadPage(void);
void FHY_Update(void);
#endif

#endif
