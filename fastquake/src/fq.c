/*
FastQuake (LFTech): speed options for 386/486 class PCs.
GPL v2, based on id Software's Quake source.
*/
#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"
#include "fq.h"

cvar_t fq_detail = {"fq_detail", "0", true};
cvar_t fq_spans = {"fq_spans", "2", true};
cvar_t fq_sky = {"fq_sky", "0", true};
cvar_t fq_water = {"fq_water", "0", true};
cvar_t fq_particles = {"fq_particles", "0", true};
cvar_t fq_fps = {"fq_fps", "0", true};

int fq_curdetail;
int fq_skycolor;
vrect_t fq_fullvrect;

void FQ_Init (void)
{
	Cvar_RegisterVariable (&fq_detail);
	Cvar_RegisterVariable (&fq_spans);
	Cvar_RegisterVariable (&fq_sky);
	Cvar_RegisterVariable (&fq_water);
	Cvar_RegisterVariable (&fq_particles);
	Cvar_RegisterVariable (&fq_fps);
}

/*
FQ_ExpandView: the view was rendered into the top-left quarter/half of
fq_fullvrect; stretch it in place to fill the whole rect.
*/
void FQ_ExpandView (void)
{
	int x0 = fq_fullvrect.x, y0 = fq_fullvrect.y;
	int w = fq_fullvrect.width, h = fq_fullvrect.height;
	int hw = r_refdef.vrect.width, hh = r_refdef.vrect.height;
	int rb = vid.rowbytes;
	int r, i;
	byte *src, *dst;

	if (fq_curdetail == 1)
	{
		for (r = 0 ; r < h ; r++)
		{
			src = vid.buffer + (y0 + r) * rb + x0;
			for (i = hw - 1 ; i >= 0 ; i--)
			{
				byte c = src[i];
				if (2*i+1 < w)
					src[2*i+1] = c;
				src[2*i] = c;
			}
		}
	}
	else if (fq_curdetail == 2)
	{
		for (r = hh - 1 ; r >= 0 ; r--)
		{
			src = vid.buffer + (y0 + r) * rb + x0;
			dst = vid.buffer + (y0 + 2*r) * rb + x0;
			if (2*r >= h)
				continue;
			for (i = hw - 1 ; i >= 0 ; i--)
			{
				byte c = src[i];
				if (2*i+1 < w)
					dst[2*i+1] = c;
				dst[2*i] = c;
			}
			if (2*r+1 < h)
				memcpy (dst + rb, dst, w);
		}
	}
}

/*
Average colour of a texture's first mip, cached in the texture.
*/
int FQ_TextureColor (texture_t *tx)
{
	int i, n, r = 0, g = 0, b = 0, best = 0, bestd = 0x7fffffff;
	byte *p;

	if (tx->fq_color)
		return tx->fq_color - 1;
	p = (byte *)tx + tx->offsets[0];
	n = tx->width * tx->height;
	for (i = 0 ; i < n ; i++)
	{
		byte *c = host_basepal + p[i]*3;
		r += c[0]; g += c[1]; b += c[2];
	}
	if (n) { r /= n; g /= n; b /= n; }
	for (i = 0 ; i < 224 ; i++)	// skip fullbrights
	{
		byte *c = host_basepal + i*3;
		int d = (c[0]-r)*(c[0]-r) + (c[1]-g)*(c[1]-g) + (c[2]-b)*(c[2]-b);
		if (d < bestd) { bestd = d; best = i; }
	}
	tx->fq_color = best + 1;
	return best;
}

/*
Single pixel particles
*/
void FQ_DrawParticlePixel (particle_t *pparticle)
{
	vec3_t	local, transformed;
	float	zi;
	short	*pz;
	int		izi, u, v;

	VectorSubtract (pparticle->org, r_origin, local);
	transformed[2] = DotProduct(local, r_ppn);
	if (transformed[2] < PARTICLE_Z_CLIP)
		return;
	transformed[0] = DotProduct(local, r_pright);
	transformed[1] = DotProduct(local, r_pup);
	zi = 1.0 / transformed[2];
	u = (int)(xcenter + zi * transformed[0] + 0.5);
	v = (int)(ycenter - zi * transformed[1] + 0.5);
	if ((v > d_vrectbottom_particle) || (u > d_vrectright_particle) ||
		(v < d_vrecty) || (u < d_vrectx))
		return;
	pz = d_pzbuffer + (d_zwidth * v) + u;
	izi = (int)(zi * 0x8000);
	if (pz[0] <= izi)
	{
		pz[0] = izi;
		((byte *)d_viewbuffer)[d_scantable[v] + u] = pparticle->color;
	}
}

/*
FPS counter (top right of the view)
*/
void FQ_DrawFPS (void)
{
	static double lasttime;
	static int frames;
	static float fps;
	double t;
	char st[16];
	int x;

	t = Sys_FloatTime ();
	frames++;
	if (t - lasttime >= 1.0)
	{
		fps = frames / (t - lasttime);
		frames = 0;
		lasttime = t;
	}
	if (!fq_fps.value)
		return;
	sprintf (st, "%4.1f", fps);
	x = scr_vrect.x + scr_vrect.width - strlen(st)*8 - 2;
	Draw_String (x, scr_vrect.y + 2, st);
}

/*
Timedemo benchmark hook: -fqbench appends a result line to FQBENCH.TXT
and quits.
*/
void FQ_BenchDone (int frames, float time)
{
	FILE *f;
	int i;

	if (!COM_CheckParm ("-fqbench"))
		return;
	f = fopen ("FQBENCH.TXT", "a");
	if (f)
	{
		fprintf (f, "%5.1f fps %d frames %5.1f s |", frames / time, frames, time);
		for (i = 1 ; i < com_argc ; i++)
			if (com_argv[i])
				fprintf (f, " %s", com_argv[i]);
		fprintf (f, " | vid %dx%d detail %d spans %d sky %d water %d part %d\n",
			vid.width, vid.height, (int)fq_detail.value, (int)fq_spans.value,
			(int)fq_sky.value, (int)fq_water.value, (int)fq_particles.value);
		fclose (f);
	}
	Cbuf_AddText ("quit\n");
}
