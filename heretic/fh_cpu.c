//
// fh_cpu.c : FastHeretic CPU detection and per-CPU drawers
//
// Like FastDoom, some drawers have versions tuned for particular CPUs.
// The CPU is found at startup (CPUID where there is one, the DPMI host's
// CPU type and a Cyrix check where there isn't), or set with the same
// command line options as FastDoom (-386sx, -cy5x86, -k6, -pentium...),
// in the FASTHERETIC menu, or with fh_cpu in HERETIC.CFG.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <string.h>
#include <conio.h>
#include <i86.h>
#include "doomdef.h"
#include "r_local.h"
#include "fh_cpu.h"

int fh_cpu = CPU_AUTO;          // from the config / menu
int fh_cpusel = CPU_I486;       // what the drawers are picked for
int fh_cpudetected = CPU_I486;
int fh_hasmmx;
int fh_usedmmx;                 // an MMX drawer may have run this frame

const char *fh_cpunames[NUM_CPU] = {
	"INTEL 386SX", "INTEL 386DX", "INTEL 486", "PENTIUM P5/P54C",
	"PENTIUM P54CS", "PENTIUM MMX", "PENTIUM II", "AMD K5", "AMD K6",
	"CYRIX 386DLC", "CYRIX 486", "CYRIX 5X86", "CYRIX 6X86",
	"CYRIX 6X86MX", "UMC 486", "IDT WINCHIP", "RISE MP6"
};

static const struct { char *parm; int cpu; } cpuparms[] = {
	{"-386sx", CPU_I386SX}, {"-386dx", CPU_I386DX}, {"-i486", CPU_I486},
	{"-pentium", CPU_P5}, {"-pentiumP54CS", CPU_P54CS},
	{"-pentiumMMX", CPU_PMMX}, {"-pentiumII", CPU_PII}, {"-k5", CPU_K5},
	{"-k6", CPU_K6}, {"-cy386", CPU_CY386}, {"-cy486", CPU_CY486},
	{"-cy5x86", CPU_CY5X86}, {"-cy6x86", CPU_CY6X86},
	{"-cy6x86mx", CPU_CY6X86MX}, {"-umc486", CPU_UMC486},
	{"-winchip", CPU_WINCHIP}, {"-mp6", CPU_MP6},
};

//--------------------------------------------------------------------------
// Detection
//--------------------------------------------------------------------------

// 1 when EFLAGS bit 21 can be flipped, so CPUID is there
int FH_HasCPUID(void);
#pragma aux FH_HasCPUID = \
	"pushfd" \
	"pop eax" \
	"mov ecx,eax" \
	"xor eax,200000h" \
	"push eax" \
	"popfd" \
	"pushfd" \
	"pop eax" \
	"push ecx" \
	"popfd" \
	"xor eax,ecx" \
	"shr eax,21" \
	"and eax,1" \
	value [eax] modify [ecx];

static void FH_CPUID(int leaf, unsigned *regs);
#pragma aux FH_CPUID = \
	"push esi" \
	"mov esi,edx" \
	0x0f 0xa2 /* cpuid */ \
	"mov [esi],eax" \
	"mov [esi+4],ebx" \
	"mov [esi+8],ecx" \
	"mov [esi+12],edx" \
	"pop esi" \
	parm [eax] [edx] modify [eax ebx ecx edx];

// Cyrix chips leave the flags alone in DIV; Intel and AMD change them
int FH_IsCyrix(void);
#pragma aux FH_IsCyrix = \
	"xor eax,eax" \
	"sahf" \
	"mov eax,5" \
	"mov bl,2" \
	"div bl" \
	"lahf" \
	"cmp ah,2" \
	"sete al" \
	"movzx eax,al" \
	value [eax] modify [ebx];

static int DPMICPU(void)
{
	union REGS r;

	memset(&r, 0, sizeof(r));
	r.w.ax = 0x0400;
	int386(0x31, &r, &r);
	return r.h.cl;              // 3, 4, 5, 6
}

static int CyrixReg(int index)
{
	int v;

	_disable();
	outp(0x22, index);
	v = inp(0x23);
	_enable();
	return v;
}

static int DetectCPU(void)
{
	unsigned r[4];
	char vendor[13];
	int family, model, stepping, dir0;

	if (FH_HasCPUID())
	{
		FH_CPUID(0, r);
		memcpy(vendor, &r[1], 4);
		memcpy(vendor + 4, &r[3], 4);
		memcpy(vendor + 8, &r[2], 4);
		vendor[12] = 0;
		FH_CPUID(1, r);
		family = (r[0] >> 8) & 15;
		model = (r[0] >> 4) & 15;
		stepping = r[0] & 15;
		fh_hasmmx = (r[3] >> 23) & 1;

		if (!strcmp(vendor, "GenuineIntel"))
		{
			if (family == 4)
				return CPU_I486;
			if (family == 5)
			{
				if (model == 4 || fh_hasmmx)
					return CPU_PMMX;
				if (model == 2 && stepping >= 0xb)
					return CPU_P54CS;
				return CPU_P5;
			}
			return family > 5 ? CPU_PII : CPU_I486;
		}
		if (!strcmp(vendor, "AuthenticAMD"))
		{
			if (family == 4)
				return CPU_I486;    // Am486, Am5x86
			if (family == 5)
				return model < 6 ? CPU_K5 : CPU_K6;
			return CPU_K6;
		}
		if (!strcmp(vendor, "CyrixInstead"))
		{
			if (family == 4)
				return CPU_CY5X86;  // 5x86, MediaGX
			if (family == 5)
				return CPU_CY6X86;
			return CPU_CY6X86MX;
		}
		if (!strcmp(vendor, "CentaurHauls"))
			return CPU_WINCHIP;
		if (!strcmp(vendor, "RiseRiseRise"))
			return CPU_MP6;
		if (!strcmp(vendor, "UMC UMC UMC "))
			return CPU_UMC486;
		if (family >= 6)
			return CPU_PII;
		return family == 5 ? CPU_P5 : CPU_I486;
	}

	// No CPUID: a 386, a 486 or a Cyrix with CPUID switched off
	// Some emulators also leave the flags alone, so check that the Cyrix
	// configuration registers answer too (nothing there reads FFh)
	dir0 = 0xff;
	if (FH_IsCyrix())
		dir0 = CyrixReg(0xfe);
	if (dir0 != 0xff || (FH_IsCyrix() && CyrixReg(0xc0) != 0xff))
	{
		if (dir0 >= 0x50 && dir0 < 0x60)
			return CPU_CY6X86MX;
		if (dir0 >= 0x30 && dir0 < 0x50)
			return CPU_CY6X86;
		if (dir0 >= 0x20 && dir0 < 0x30)
			return CPU_CY5X86;
		if (dir0 >= 0x10 && dir0 < 0x20)
			return CPU_CY486;
		return CPU_CY386;       // SLC/DLC/SRx/DRx, or no DIR0
	}
	return DPMICPU() <= 3 ? CPU_I386DX : CPU_I486;
}

void FH_InitCPU(void)
{
	int i;

	fh_cpudetected = DetectCPU();
	for (i = 0; i < sizeof(cpuparms) / sizeof(cpuparms[0]); i++)
		if (M_CheckParm(cpuparms[i].parm))
			fh_cpu = cpuparms[i].cpu;
	if (fh_cpu < CPU_AUTO || fh_cpu >= NUM_CPU)
		fh_cpu = CPU_AUTO;
	FH_SelectCPU();
}

void FH_SelectCPU(void)
{
	// the MMX drawers are only used when CPUID says MMX is there
	fh_cpusel = fh_cpu == CPU_AUTO ? fh_cpudetected : fh_cpu;
#ifdef FH_STOCK
	fh_cpusel = CPU_I486;       // the plain drawers, like vanilla
#endif
}

//--------------------------------------------------------------------------
// Which drawer for which CPU (FastDoom's choices)
//--------------------------------------------------------------------------

int FH_ColumnKind(void)
{
	switch (fh_cpusel)
	{
	case CPU_K5: case CPU_P54CS: case CPU_CY6X86: case CPU_CY6X86MX:
		return FHC_ROLL;
	case CPU_UMC486: case CPU_CY5X86: case CPU_K6:
		return FHC_LEA;
	case CPU_MP6: case CPU_PMMX: case CPU_PII:
		return fh_hasmmx ? FHC_MMX : FHC_NORMAL;
	}
	return FHC_NORMAL;
}

int FH_SpanKind(void)
{
	switch (fh_cpusel)
	{
	case CPU_CY6X86: case CPU_K5: case CPU_P54CS:
		return FHC_ROLL;
	case CPU_P5: case CPU_WINCHIP: case CPU_CY6X86MX:
		return FHC_PENTIUM;
	case CPU_MP6: case CPU_PMMX: case CPU_PII: case CPU_K6:
		return fh_hasmmx ? FHC_MMX : FHC_PENTIUM;
	}
	return FHC_NORMAL;
}

// Low detail columns use LEA on every chip with a fast one
int FH_LowColumnLEA(void)
{
	switch (fh_cpusel)
	{
	case CPU_UMC486: case CPU_CY5X86: case CPU_CY6X86: case CPU_CY6X86MX:
	case CPU_K5: case CPU_K6: case CPU_MP6:
		return 1;
	}
	return 0;
}

int FH_LowSpanPentium(void)
{
	switch (fh_cpusel)
	{
	case CPU_P5: case CPU_P54CS: case CPU_PMMX: case CPU_PII:
	case CPU_WINCHIP: case CPU_CY6X86: case CPU_CY6X86MX:
		return 1;
	}
	return 0;
}

// Mode Y: 2-pixel span stores for 386-class chips and the Cyrix 486
int FH_SpanSX(void)
{
	switch (fh_cpusel)
	{
	case CPU_I386SX: case CPU_I386DX: case CPU_CY386: case CPU_CY486:
		return 1;
	}
	return 0;
}
