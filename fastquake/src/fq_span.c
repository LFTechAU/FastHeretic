/*
fq_span.c: FastQuake span drawer dispatch (LFTech). GPL v2.
fq_spans 1 = 32 pixel segments, 2 = 64 pixel segments (fq_draw.s).
*/
#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"
#include "fq.h"

extern float fq_sdivzN, fq_tdivzN, fq_ziN;
void D_DrawSpansFQ32 (espan_t *pspan);
void D_DrawSpansFQ64 (espan_t *pspan);

void D_DrawSpansFQ (espan_t *pspan)
{
	float n = (fq_spans.value >= 2) ? 64 : 32;

	if (fq_prof)
	{
		espan_t *p;
		for (p = pspan ; p ; p = p->pnext)
		{
			fq_nspans++;
			fq_npix += p->count;
		}
	}

	fq_sdivzN = d_sdivzstepu * n;
	fq_tdivzN = d_tdivzstepu * n;
	fq_ziN = d_zistepu * n;
	if (n == 64)
		D_DrawSpansFQ64 (pspan);
	else
		D_DrawSpansFQ32 (pspan);
}
