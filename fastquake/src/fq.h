// fq.h: FastQuake options (LFTech)
#ifndef FQ_H
#define FQ_H
extern cvar_t fq_detail;	// 0 high, 1 low (half width), 2 potato (half width+height)
extern cvar_t fq_spans;		// 0 stock asm (d_subdiv16), 1 FQ 16px, 2 FQ 32px, 3 FQ 64px, 4 affine
extern cvar_t fq_sky;		// 0 normal, 1 flat colour
extern cvar_t fq_water;		// 0 warped, 1 flat colour
extern cvar_t fq_particles;	// 0 normal, 1 single pixel, 2 none
extern cvar_t fq_fps;		// show fps counter
extern int fq_curdetail;
extern int fq_skycolor;
extern vrect_t fq_fullvrect;
void FQ_Init (void);
void FQ_ExpandView (void);
void FQ_DrawFPS (void);
struct espan_s;
void D_DrawSpansFQ (struct espan_s *pspan);
void FQ_DrawParticlePixel (particle_t *p);
int FQ_TextureColor (texture_t *tx);
void FQ_BenchDone (int frames, float time);
#endif
