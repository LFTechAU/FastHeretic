/*
fq_span.c: FastQuake span drawer with 16/32/64 pixel perspective
subdivision (LFTech). One FDIV per segment instead of per 16 pixels;
the per-pixel loop is FQ_SpanSeg in fq_spana.s. GPL v2.
*/
#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"
#include "fq.h"

extern int fq_advancetable[2];
extern int fq_tstepfrac;
void FQ_SpanSeg (byte *pdest, byte *psrc, int sfrac, int tfrac, int sstepfrac, int count);

static inline int FQ_ftol (float f)
{
	int i;
	__asm__ ("fistpl %0" : "=m" (i) : "t" (f) : "st");	// chop mode during refresh
	return i;
}

void D_DrawSpansFQ (espan_t *pspan)
{
	int			count, spancount, sh, N, cw = cachewidth;
	byte		*pbase = (byte *)cacheblock, *pdest;
	fixed16_t	s, t, snext, tnext, sstep, tstep;
	float		sdivz, tdivz, zi, z, du, dv;
	float		sdivzN, tdivzN, ziN;

	sh = (int)fq_spans.value + 3;	// 1 -> 16, 2 -> 32, 3 -> 64
	if (sh < 4) sh = 4;
	if (sh > 6) sh = 6;
	N = 1 << sh;
	sdivzN = d_sdivzstepu * N;
	tdivzN = d_tdivzstepu * N;
	ziN = d_zistepu * N;

	do
	{
		pdest = (byte *)d_viewbuffer + d_scantable[pspan->v] + pspan->u;
		count = pspan->count;

		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;

		s = FQ_ftol (sdivz * z) + sadjust;
		if (s > bbextents) s = bbextents;
		else if (s < 0) s = 0;
		t = FQ_ftol (tdivz * z) + tadjust;
		if (t > bbextentt) t = bbextentt;
		else if (t < 0) t = 0;

		do
		{
			spancount = count > N ? N : count;
			count -= spancount;

			if (count)
			{
				sdivz += sdivzN;
				tdivz += tdivzN;
				zi += ziN;
				z = (float)0x10000 / zi;

				snext = FQ_ftol (sdivz * z) + sadjust;
				if (snext > bbextents) snext = bbextents;
				else if (snext < N) snext = N;
				tnext = FQ_ftol (tdivz * z) + tadjust;
				if (tnext > bbextentt) tnext = bbextentt;
				else if (tnext < N) tnext = N;

				sstep = (snext - s) >> sh;
				tstep = (tnext - t) >> sh;
			}
			else if (spancount > 1)
			{
				float spancountminus1 = (float)(spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;

				snext = FQ_ftol (sdivz * z) + sadjust;
				if (snext > bbextents) snext = bbextents;
				else if (snext < N) snext = N;
				tnext = FQ_ftol (tdivz * z) + tadjust;
				if (tnext > bbextentt) tnext = bbextentt;
				else if (tnext < N) tnext = N;

				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}
			else
			{
				pdest[0] = pbase[(s >> 16) + (t >> 16) * cw];
				break;
			}

			fq_advancetable[1] = (tstep >> 16) * cw + (sstep >> 16);
			fq_advancetable[0] = fq_advancetable[1] + cw;
			fq_tstepfrac = tstep << 16;
			FQ_SpanSeg (pdest, pbase + (s >> 16) + (t >> 16) * cw,
				s << 16, t << 16, sstep << 16, spancount);
			pdest += spancount;
			s = snext;
			t = tnext;
		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}
