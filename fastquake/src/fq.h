// fq.h: FastQuake options (LFTech)
#ifndef FQ_H
#define FQ_H
#define FQ_VERSION "0.1"
#ifndef FQ_CPUNAME
#define FQ_CPUNAME "486"
#endif
extern cvar_t fq_detail;	// 0 high, 1 low (half height), 2 potato (half both), 3 half width
extern cvar_t fq_spans;		// 0 stock (16px), 1 486 32px, 2 486 64px
extern cvar_t fq_sky;		// 0 normal, 1 flat colour
extern cvar_t fq_water;		// 0 warped, 1 flat colour
extern cvar_t fq_particles;	// 0 normal, 1 single pixel, 2 none
extern cvar_t fq_fps;
extern cvar_t fq_light;		// 0 normal, 1 flat per lightmap block, 2 fullbright		// show fps counter
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
// profiler (-fqprof): PIT ticks spent in each phase
enum {FQP_RENDER, FQP_SURFS, FQP_CACHE, FQP_SPANS, FQP_ZSPANS, FQP_SKYTURB,
	FQP_ENTS, FQP_VIEWMODEL, FQP_PARTICLES, FQP_SCREEN, FQP_VIDUPDATE, FQP_NUM};
extern int fq_prof;
extern int fq_benchquit;
extern unsigned fq_nspans, fq_npix;
extern unsigned fq_proft[FQP_NUM];
unsigned FQ_Ticks (void);
#define FQP_BEGIN(n) unsigned _fqp##n = fq_prof ? FQ_Ticks () : 0
#define FQP_END(n) if (fq_prof) fq_proft[n] += FQ_Ticks () - _fqp##n
#endif
