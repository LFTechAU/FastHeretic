// r_fh.h : FastHeretic renderer options and drawers
#ifndef __R_FH_H__
#define __R_FH_H__

extern int fh_detail;        // 0 = high, 1 = low, 2 = potato
extern int fh_flatsurfaces;  // 0 textured, 1 flat lit by distance, 2 flat
extern int fh_flatsky;       // single-colour sky
extern int fh_notrans;       // ghosts: 0 see-thru, 1 solid, 2 saturn
extern int fh_showfps;       // frame rate counter
extern int fh_uncapped;      // draw frames between tics
extern int fh_uncapnow;
extern volatile int ticcount_hr;
extern fixed_t fh_interpweight;
void FH_ResetInterpolation(void);
void FH_SaveInterpolation(void);
void FH_InterpolateBegin(void);
void FH_InterpolateEnd(void);
void FH_SetUncapped(int on);
void FH_TryRunTicsUncapped(void);
extern int fh_near;          // only draw nearby non-monster sprites
extern int fh_vsync;         // wait for vertical retrace before blits
extern int fh_csv;           // append timedemo results to FHBENCH.CSV
extern int fh_fixdac;
// fh_bench.c
extern int fh_bench, fh_benchnext;
int FH_BenchInit(void);
void FH_BenchStart(void);
void FH_BenchFrame(void);
void FH_BenchEnd(void);
void FH_BenchNext(void);        // wait for retrace before palette changes (-fixDAC)

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

#if defined(FH_MODEY) || defined(FH_VBD) || (defined(FH_VBR) && defined(FH_HIRES))
// fh_modey.c: Mode Y, Mode X and VESA direct (FH_VBD)
// fh_hires.c: high resolution VESA
#define FH_DIRECT
#if defined(FH_MODEY) || defined(FH_HIRES)
#define FH_PATCHDIRECT               // overlays go through FHY_DrawPatch
#endif
extern int fhy_sbmode;               // status bar overlays: anchor at the bottom
extern byte *fh_bordbuf;             // high res: border around a small view
extern byte *fh_planar;      // set: overlays go straight into this page
extern int fh_directframe;   // this frame's view was drawn into VRAM
void FHY_InitGraphics(void);
void R_FHY_InitTables(void);
void R_FHY_SetupFrame(void);
void R_FHY_SetDrawers(void);
void FHY_DrawPatch(int x, int y, patch_t *patch, int mode);
void FHY_NoteBox(int x, int y, patch_t *patch, int mode);
void FHY_OverlayOn(void);
void FHY_OverlayOff(void);
void FHY_Composite(void);
void FHY_ReadPage(void);
void FHY_Update(void);
int FHY_Direct(void);
#endif
#ifdef FH_VBR
// fh_vesa.c
extern byte *fhe_lfb;
extern int fhe_pitch;
int FHE_Init(int w, int h, int wantpages);
void FHE_CopyRect(byte *src, int srcpitch, int x, int y, int w, int h);
void FHE_CopyRows(byte *dest, byte *src, int y, int h);
#endif

#endif
