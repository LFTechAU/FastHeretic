// fh_cpu.h : FastHeretic CPU detection and per-CPU drawers
#ifndef __FH_CPU_H__
#define __FH_CPU_H__

// Same numbers as FastDoom's selectedCPU
enum
{
	CPU_AUTO = -1,
	CPU_I386SX, CPU_I386DX, CPU_I486, CPU_P5, CPU_P54CS, CPU_PMMX, CPU_PII,
	CPU_K5, CPU_K6, CPU_CY386, CPU_CY486, CPU_CY5X86, CPU_CY6X86,
	CPU_CY6X86MX, CPU_UMC486, CPU_WINCHIP, CPU_MP6,
	NUM_CPU
};

enum { FHC_NORMAL, FHC_LEA, FHC_ROLL, FHC_PENTIUM, FHC_MMX };

extern int fh_cpu, fh_cpusel, fh_cpudetected, fh_hasmmx, fh_usedmmx;
extern const char *fh_cpunames[NUM_CPU];

void FH_InitCPU(void);
void FH_SelectCPU(void);
int FH_ColumnKind(void);
int FH_SpanKind(void);
int FH_LowColumnLEA(void);
int FH_LowSpanPentium(void);
int FH_SpanSX(void);

void FH_EMMS(void);
#pragma aux FH_EMMS = 0x0f 0x77;

// fhcpu.asm
void R_DrawColumnFastLEA(void);
void R_DrawColumnLowFastLEA(void);
void R_DrawColumnRoll(void);
void R_DrawColumnMMX(void);
void R_DrawSpanRoll(void);
void R_DrawSpanMMX(void);
void R_DrawSpanPentium(void);
void R_DrawSpanLowPentium(void);
void R_DrawSpanPotatoPentium(void);

#endif
