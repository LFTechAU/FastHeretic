/* drawtest.c : checks the per-CPU drawers (fhcpu.asm, fhasmy.asm) against
   reference versions. Runs natively on x86 Linux: see run.sh. */
typedef unsigned char byte;
#define G(t,n) t n __asm__("_" #n)
#define A(t,n,z) t n[z] __asm__("_" #n)
G(byte*, dc_colormap); G(int, dc_x); G(int, dc_yl); G(int, dc_yh); G(int, dc_iscale);
G(int, dc_texturemid); G(byte*, dc_source); G(int, centery); G(int, viewwindowy);
A(byte*,ylookup,1200); A(int,columnofs,1600);
G(int, ds_y); G(int, ds_x1); G(int, ds_x2); G(byte*, ds_colormap); G(int, ds_xfrac); G(int, ds_yfrac);
G(int, ds_xstep); G(int, ds_ystep); G(byte*, ds_source);
A(byte*,my_ylookup,200); A(int,my_ofs,320); A(byte,my_mask,320); G(byte, my_curmask);
G(byte*, ysp_dest); G(int, ysp_count); G(unsigned, ysp_pos); G(unsigned, ysp_step);
#define F(n) void n(void) __asm__(#n "_")
F(R_DrawColumnFastLEA); F(R_DrawColumnLowFastLEA); F(R_DrawColumnRoll); F(R_DrawColumnMMX);
F(R_DrawSpanRoll); F(R_DrawSpanMMX); F(R_DrawSpanPentium); F(R_DrawSpanLowPentium); F(R_DrawSpanPotatoPentium);
F(R_DrawColumnLowA); F(R_DrawSpanLowA); F(R_DrawSpanPotatoA); F(R_DrawColumnPotatoA);
F(R_DrawColumnYA); F(R_DrawColumnYLEA); F(R_DrawSpanYCore); F(R_DrawSpanYCoreSX);

static void sys_write(const char*s,int n){ __asm__ volatile("int $0x80"::"a"(4),"b"(1),"c"(s),"d"(n):"memory"); }
static void sys_exit(int c){ __asm__ volatile("int $0x80"::"a"(1),"b"(c)); }
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static void puts_(const char*s){sys_write(s,slen(s));}
static void putn(unsigned v){char b[12];int i=11;b[i]=0;do{b[--i]='0'+v%10;v/=10;}while(v);puts_(b+i);}
static unsigned rs=12345; static unsigned rnd(void){rs=rs*1103515245+12345;return rs>>8;}
static void mset(byte*p,int v,int n){while(n--)*p++=v;}
static int mcmp(byte*a,byte*b,int n){int i;for(i=0;i<n;i++)if(a[i]!=b[i])return i+1;return 0;}

static byte scr[320*200], ref[320*200];
static byte cmapbuf[512], tex[128], flat[4096];
static byte *cmap;

static void refcol(byte *dst){ int y; for(y=dc_yl;y<=dc_yh;y++){ unsigned f=(unsigned)dc_texturemid+(unsigned)(y-centery)*(unsigned)dc_iscale; dst[y*320+columnofs[dc_x]]=cmap[tex[(f>>16)&127]]; } }
static void refspan(byte *dst){ unsigned pos=((ds_xfrac<<10)&0xffff0000)|((ds_yfrac>>6)&0xffff), st=((ds_xstep<<10)&0xffff0000)|((ds_ystep>>6)&0xffff); int x; for(x=ds_x1;x<=ds_x2;x++){ int idx=((pos>>10)&63)*64+(pos>>26); dst[ds_y*320+columnofs[0]+x]=cmap[flat[idx]]; pos+=st; } }

static int fails;
static void check(const char*name,int r){ if(r){ fails++; puts_("FAIL "); puts_(name); puts_(" at "); putn(r-1); puts_("\n"); } }

static void setcol(int h){ dc_x=rnd()%(320>>h); dc_yl=rnd()%200; dc_yh=dc_yl+rnd()%(200-dc_yl); if(rnd()%8==0) dc_yh=dc_yl-1; dc_iscale=0x800+rnd()%0x40000; dc_texturemid=rnd()*7; centery=rnd()%200; }
static void setspan(int w){ ds_y=rnd()%200; ds_x1=rnd()%w; ds_x2=ds_x1+rnd()%(w-ds_x1); ds_xfrac=rnd()*3; ds_yfrac=rnd()*5; ds_xstep=rnd()%0x40000; ds_ystep=rnd()%0x40000; if(rnd()&1)ds_xstep=-ds_xstep; }

void _start(void){
  int i,t;
  cmap=(byte*)(((unsigned)cmapbuf+255)&~255);
  for(i=0;i<256;i++)cmap[i]=rnd(); for(i=0;i<128;i++)tex[i]=rnd(); for(i=0;i<4096;i++)flat[i]=rnd();
  for(i=0;i<200;i++)ylookup[i]=scr+i*320; for(i=0;i<320;i++)columnofs[i]=i;
  dc_colormap=cmap; dc_source=tex; ds_colormap=cmap; ds_source=flat;
  for(t=0;t<3000;t++){
    /* high columns: reference C */
    void (*cols[3])(void)={R_DrawColumnFastLEA,R_DrawColumnRoll,R_DrawColumnMMX}; const char*cn[3]={"FastLEA","Roll","MMX"};
    setcol(0);
    for(i=0;i<3;i++){ mset(scr,0,64000); mset(ref,0,64000); cols[i](); refcol(ref); check(cn[i],mcmp(scr,ref,64000)); }
    /* high spans */
    { void (*sp[3])(void)={R_DrawSpanRoll,R_DrawSpanMMX,R_DrawSpanPentium}; const char*sn[3]={"SpanRoll","SpanMMX","SpanPentium"};
      setspan(320);
      for(i=0;i<3;i++){ mset(scr,0,64000); mset(ref,0,64000); sp[i](); refspan(ref); check(sn[i],mcmp(scr,ref,64000)); } }
    /* low column vs fhasm */
    setcol(1);
    mset(scr,0,64000); R_DrawColumnLowA(); for(i=0;i<64000;i++)ref[i]=scr[i];
    mset(scr,0,64000); R_DrawColumnLowFastLEA(); check("LowFastLEA",mcmp(scr,ref,64000));
    setspan(160);
    mset(scr,0,64000); R_DrawSpanLowA(); for(i=0;i<64000;i++)ref[i]=scr[i];
    mset(scr,0,64000); R_DrawSpanLowPentium(); check("LowPentium",mcmp(scr,ref,64000));
    setspan(80);
    mset(scr,0,64000); R_DrawSpanPotatoA(); for(i=0;i<64000;i++)ref[i]=scr[i];
    mset(scr,0,64000); R_DrawSpanPotatoPentium(); check("PotatoPentium",mcmp(scr,ref,64000));
    /* Mode Y */
    for(i=0;i<200;i++)my_ylookup[i]=scr+i*80; for(i=0;i<320;i++){my_ofs[i]=i>>2;my_mask[i]=1;} my_curmask=1;
    setcol(0);
    mset(scr,0,64000); R_DrawColumnYA(); for(i=0;i<64000;i++)ref[i]=scr[i];
    mset(scr,0,64000); R_DrawColumnYLEA(); check("YLEA",mcmp(scr,ref,64000));
    ysp_dest=scr+(rnd()%100)*80+rnd()%10; ysp_count=1+rnd()%70; ysp_pos=rnd()*11; ysp_step=rnd()*13;
    mset(scr,0,64000); R_DrawSpanYCore(); for(i=0;i<64000;i++)ref[i]=scr[i];
    mset(scr,0,64000); R_DrawSpanYCoreSX(); check("YCoreSX",mcmp(scr,ref,64000));
    for(i=0;i<200;i++)ylookup[i]=scr+i*320;
    if(fails>20)break;
  }
  puts_(fails?"FAILED\n":"all drawers match\n");
  sys_exit(fails!=0);
}
