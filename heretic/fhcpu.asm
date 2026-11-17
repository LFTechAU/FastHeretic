;
; fhcpu.asm : FastHeretic drawers tuned for particular CPUs (NASM)
;
; Adapted from FastDoom's linearh.asm, lineahKN.asm, lineahPE.asm,
; linealKN.asm, linealPE.asm and lineapPE.asm (Victor Nieto, GPL), which
; come from id Software's and Raven Software's linear.asm.
;
;   FastLEA  unrolled columns that step two fractions with LEA, for CPUs
;            where LEA is cheap (UMC, Cyrix 5x86/6x86, AMD K5/K6, Rise)
;   Roll     plain loops, for CPUs with good branch prediction and small
;            caches (Pentium P54CS, AMD K5, Cyrix 6x86)
;   Pentium  unrolled spans that pair well in the Pentium's two pipes
;   MMX      two pixels per pass using MMX registers (Pentium MMX and up)
;
; Heretic moves centery when the player looks up or down, so these read
; centery on every call instead of patching it into the code.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;

BITS 32

%ifndef SCREENWIDTH
%define SCREENWIDTH   320
%endif
%ifndef SCREENHEIGHT
%define SCREENHEIGHT  200
%endif

OP_RET      equ 0xC3
OP_MOVAL    equ 0x8A

extern _dc_colormap
extern _dc_x
extern _dc_yl
extern _dc_yh
extern _dc_iscale
extern _dc_texturemid
extern _dc_source
extern _centery

extern _ylookup
extern _columnofs

extern _ds_y
extern _ds_x1
extern _ds_x2
extern _ds_colormap
extern _ds_xfrac
extern _ds_yfrac
extern _ds_xstep
extern _ds_ystep
extern _ds_source

segment _DATA public align=4 class=DATA USE32

align 4

leacalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    dd kvscale %+ LINE
  %assign LINE LINE+1
  %endrep

lowleacalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    dd klvscale %+ LINE
  %assign LINE LINE+1
  %endrep

pentcalls:
  %assign LINE 0
  %rep SCREENWIDTH+1
    dd pehmap %+ LINE
  %assign LINE LINE+1
  %endrep

lowpentcalls:
  %assign LINE 0
  %rep (SCREENWIDTH/2)+1
    dd plhmap %+ LINE
  %assign LINE LINE+1
  %endrep

potpentcalls:
  %assign LINE 0
  %rep (SCREENWIDTH/4)+1
    dd pphmap %+ LINE
  %assign LINE LINE+1
  %endrep

pcallpoint   dd 0
ljump        dd 0
preturnpoint dd 0

segment _TEXT public align=4 class=CODE USE32

%macro PUSHALL 0
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp
%endmacro

%macro POPALL 0
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
%endmacro

; ecx = composite 6.10 x / 6.10 y position, %1 = composite step
; (uses eax)
%macro COMPOSITE 1
  mov   ecx,[_ds_xfrac]
  shl   ecx,10
  and   ecx,0xffff0000
  mov   eax,[_ds_yfrac]
  shr   eax,6
  and   eax,0xffff
  or    ecx,eax
  mov   %1,[_ds_xstep]
  shl   %1,10
  and   %1,0xffff0000
  mov   eax,[_ds_ystep]
  shr   eax,6
  and   eax,0xffff
  or    %1,eax
%endmacro

;============================================================================
; FastLEA columns, %1 = call table, %2 = detail shift
;   eax colormap, ecx step, esi source, edi bottom pixel
;   ebx / edx take turns holding the next fraction and the current index
;============================================================================

%macro LEA_ENTRY 2
  PUSHALL
  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_ylookup+ebp*4]
  sub   ebp,eax                 ; ebp = pixel count - 1
  js    %%done
  mov   ebx,[_dc_x]
  mov   ecx,[_dc_iscale]
%if %2 > 0
  shl   ebx,%2
%endif
  add   edi,[_columnofs+ebx*4]
  sub   eax,[_centery]
  imul  ecx
  mov   esi,[_dc_source]
  shl   ecx,9                   ; 7 significant bits, 25 frac
  test  ebp,1
  jne   %%odd
  mov   ebx,[_dc_texturemid]
  add   ebx,eax
  shl   ebx,9
  mov   eax,[_dc_colormap]
  lea   edx,[ebx+ecx]
  shr   ebx,25
  jmp   [%1+4+ebp*4]
%%odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9
  mov   eax,[_dc_colormap]
  lea   ebx,[edx+ecx]
  shr   edx,25
  jmp   [%1+4+ebp*4]
%%done:
  POPALL
  ret
%endmacro

global R_DrawColumnFastLEA_
R_DrawColumnFastLEA_:
  LEA_ENTRY leacalls, 0

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
kvscale %+ LINE:
  %if LINE % 2 = 0
  mov   al,[esi+edx]
  lea   edx,[ebx+ecx]
  mov   al,[eax]
  shr   ebx,25
  %else
  mov   al,[esi+ebx]
  lea   ebx,[edx+ecx]
  mov   al,[eax]
  shr   edx,25
  %endif
  mov   [edi-(LINE-1)*SCREENWIDTH],al
  %assign LINE LINE-1
%endrep
kvscale1:
  mov   al,[esi+ebx]
  mov   al,[eax]
  mov   [edi],al
kvscale0:
  POPALL
  ret

;------------------------------------------------------------------------
; Low detail, each pixel 2 wide: ebp / edx take turns, colour in bx
;------------------------------------------------------------------------

global R_DrawColumnLowFastLEA_
R_DrawColumnLowFastLEA_:
  PUSHALL
  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_ylookup+ebp*4]
  sub   ebp,eax                 ; ebp = pixel count - 1
  js    .done
  mov   ebx,[_dc_x]
  mov   esi,[lowleacalls+4+ebp*4]
  mov   ecx,[_dc_iscale]
  mov   [ljump],esi
  shl   ebx,1
  add   edi,[_columnofs+ebx*4]
  sub   eax,[_centery]
  imul  ecx
  mov   esi,[_dc_source]
  shl   ecx,9
  xor   ebx,ebx
  test  ebp,1
  jne   .odd
  mov   ebp,[_dc_texturemid]
  add   ebp,eax
  shl   ebp,9
  mov   eax,[_dc_colormap]
  lea   edx,[ebp+ecx]
  shr   ebp,25
  jmp   [ljump]
.odd:
  mov   edx,[_dc_texturemid]
  add   edx,eax
  shl   edx,9
  mov   eax,[_dc_colormap]
  lea   ebp,[edx+ecx]
  shr   edx,25
  jmp   [ljump]
.done:
  POPALL
  ret

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
klvscale %+ LINE:
  %if LINE % 2 = 0
  mov   al,[esi+edx]
  lea   edx,[ebp+ecx]
  mov   bl,[eax]
  shr   ebp,25
  %else
  mov   al,[esi+ebp]
  lea   ebp,[edx+ecx]
  mov   bl,[eax]
  shr   edx,25
  %endif
  mov   bh,bl
  mov   [edi-(LINE-1)*SCREENWIDTH],bx
  %assign LINE LINE-1
%endrep
klvscale1:
  mov   al,[esi+ebp]
  mov   bl,[eax]
  mov   bh,bl
  mov   [edi],bx
klvscale0:
  POPALL
  ret

;============================================================================
; Roll: plain loops
;============================================================================

global R_DrawColumnRoll_
R_DrawColumnRoll_:
  PUSHALL
  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_ylookup+eax*4]
  sub   ebp,eax                 ; ebp = pixel count - 1
  js    .done
  mov   ebx,[_dc_x]
  mov   ecx,[_dc_iscale]
  add   edi,[_columnofs+ebx*4]
  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9
  mov   eax,[_dc_colormap]
  mov   ebx,edx
  shr   ebx,25
.loop:
  add   edx,ecx
  mov   al,[esi+ebx]
  mov   ebx,edx
  mov   al,[eax]
  shr   ebx,25
  mov   [edi],al
  add   edi,SCREENWIDTH
  dec   ebp
  jns   .loop
.done:
  POPALL
  ret

global R_DrawSpanRoll_
R_DrawSpanRoll_:
  PUSHALL
  COMPOSITE edx
  mov   eax,[_ds_x1]
  mov   ebp,[_ds_x2]
  mov   esi,[_ds_source]
  mov   edi,[_ds_y]
  sub   ebp,eax                 ; ebp = pixel count - 1
  mov   edi,[_ylookup+edi*4]
  add   edi,[_columnofs]
  add   edi,eax
  shld  ebx,ecx,22
  shld  ebx,ecx,6
  mov   eax,[_ds_colormap]
  and   ebx,0xfff
  add   ecx,edx
.loop:
  mov   al,[esi+ebx]
  shld  ebx,ecx,22
  mov   al,[eax]
  shld  ebx,ecx,6
  mov   [edi],al
  and   ebx,0xfff
  add   ecx,edx
  inc   edi
  dec   ebp
  jns   .loop
  POPALL
  ret

;============================================================================
; MMX: two pixels a pass. The caller runs EMMS once the view is drawn.
;============================================================================

global R_DrawColumnMMX_
R_DrawColumnMMX_:
  PUSHALL
  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_ylookup+eax*4]
  sub   ebp,eax
  js    .done
  mov   ebx,[_dc_x]
  mov   ecx,[_dc_iscale]
  add   edi,[_columnofs+ebx*4]
  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9
  mov   eax,[_dc_colormap]
  cmp   ebp,0
  je    .single
  test  ebp,1
  jnz   .even
  mov   ebx,edx                 ; odd count: one pixel first
  shr   ebx,25
  mov   al,[esi+ebx]
  add   edx,ecx
  mov   al,[eax]
  dec   ebp
  mov   [edi],al
  add   edi,SCREENWIDTH
.even:
  movd      mm0,edx
  movd      mm1,ecx
  mov       ecx,eax
  paddd     mm0,mm1
  movd      mm2,edx
  psllq     mm0,32
  punpckldq mm1,mm1
  por       mm0,mm2
  paddd     mm1,mm1
  movq      mm3,mm0
  psrld     mm3,25
.loop:
  movd      ebx,mm3
  paddd     mm0,mm1
  psrlq     mm3,32
  mov       al,[ebx+esi]
  movd      edx,mm3
  movq      mm3,mm0
  mov       al,[eax]
  mov       cl,[edx+esi]
  mov       [edi],al
  mov       cl,[ecx]
  psrld     mm3,25
  mov       [edi+SCREENWIDTH],cl
  add       edi,2*SCREENWIDTH
  sub       ebp,2
  jns       .loop
.done:
  POPALL
  ret
.single:
  shr   edx,25
  mov   al,[esi+edx]
  mov   al,[eax]
  mov   [edi],al
  POPALL
  ret

global R_DrawSpanMMX_
R_DrawSpanMMX_:
  PUSHALL
  COMPOSITE edx
  mov   eax,[_ds_x1]
  mov   ebp,[_ds_x2]
  mov   esi,[_ds_source]
  mov   edi,[_ds_y]
  sub   ebp,eax
  mov   edi,[_ylookup+edi*4]
  add   edi,[_columnofs]
  add   edi,eax
  push  edx
  mov   edx,ecx
  mov   ebx,ecx
  shr   edx,4
  shr   ebx,26
  and   edx,0xfc0
  or    ebx,edx
  pop   edx
  mov   eax,[_ds_colormap]
  cmp   ebp,0
  je    .single
  test  ebp,1
  jnz   .even
  mov   al,[esi+ebx]            ; odd count: one pixel first
  dec   ebp
  mov   al,[eax]
  mov   [edi],al
  inc   edi
  add   ecx,edx
.even:
  mov       ebx,eax
  movd      mm0,ecx
  movd      mm1,edx
  movq      mm2,mm0
  paddd     mm0,mm1
  mov       edx,0x00000fc0
  psllq     mm0,32
  movd      mm4,edx
  por       mm0,mm2
  punpckldq mm1,mm1
  movq      mm3,mm0
  movq      mm2,mm0
  psrld     mm3,4
  psrld     mm2,26
  punpckldq mm4,mm4
  pand      mm3,mm4
  por       mm2,mm3
  paddd     mm1,mm1
.loop:
  movd      ecx,mm2
  psrlq     mm2,32
  mov       al,[esi+ecx]
  movd      ecx,mm2
  paddd     mm0,mm1
  mov       bl,[esi+ecx]
  movq      mm3,mm0
  movq      mm2,mm0
  mov       dl,[eax]
  psrld     mm3,4
  psrld     mm2,26
  mov       dh,[ebx]
  pand      mm3,mm4
  mov       [edi],dx
  por       mm2,mm3
  add       edi,2
  sub       ebp,2
  jns       .loop
  POPALL
  ret
.single:
  mov   al,[esi+ebx]
  mov   al,[eax]
  mov   [edi],al
  POPALL
  ret

;============================================================================
; Pentium spans: unrolled, index built with shifts that pair in the U and V
; pipes. %1 = call table
;   ecx position, ebp step, edx index, ebx y part, eax colormap
;============================================================================

%macro PENT_ENTRY 1
  PUSHALL
  COMPOSITE ebp
  mov   eax,[_ds_x1]
  mov   ebx,[_ds_x2]
  mov   eax,[%1+eax*4]
  mov   [pcallpoint],eax
  mov   eax,[%1+4+ebx*4]
  mov   [preturnpoint],eax
  mov   byte [eax],OP_RET
  mov   esi,[_ds_source]
  mov   edi,[_ds_y]
  mov   edi,[_ylookup+edi*4]
  add   edi,[_columnofs]
  mov   eax,[_ds_colormap]
  mov   ebx,ecx
  mov   edx,ecx
  shr   ebx,4
  shr   edx,26
  and   ebx,0xfc0
  add   ecx,ebp
  or    edx,ebx
  call  [pcallpoint]
  mov   ebx,[preturnpoint]
  mov   byte [ebx],OP_MOVAL     ; remove the ret patched in
  POPALL
  ret
%endmacro

global R_DrawSpanPentium_
R_DrawSpanPentium_:
  PENT_ENTRY pentcalls

%assign LINE 0
%rep SCREENWIDTH
pehmap %+ LINE:
  mov   al,[esi+edx]
  mov   bh,ch
  mov   edx,ecx
  shr   ebx,10
  mov   al,[eax]
  shl   ebx,6
  mov   [edi+LINE],al
  shr   edx,26
  or    edx,ebx
  add   ecx,ebp
  %assign LINE LINE+1
%endrep
pehmap %+ LINE:
  mov   al,[esi+edx]            ; overwritten by the patched ret
  ret

global R_DrawSpanLowPentium_
R_DrawSpanLowPentium_:
  PENT_ENTRY lowpentcalls

%assign LINE 0
%rep SCREENWIDTH/2
plhmap %+ LINE:
  mov   al,[esi+edx]
  mov   bh,ch
  mov   dl,[eax]
  shr   ebx,10
  mov   dh,dl
  shl   ebx,6
  mov   [edi+LINE*2],dx
  mov   edx,ecx
  shr   edx,26
  or    edx,ebx
  add   ecx,ebp
  %assign LINE LINE+1
%endrep
plhmap %+ LINE:
  mov   al,[esi+edx]            ; overwritten by the patched ret
  ret

global R_DrawSpanPotatoPentium_
R_DrawSpanPotatoPentium_:
  PENT_ENTRY potpentcalls

%assign LINE 0
%rep SCREENWIDTH/4
pphmap %+ LINE:
  mov   al,[esi+edx]
  mov   bh,ch
  mov   dl,[eax]
  shr   ebx,10
  mov   dh,dl
  shl   ebx,6
  mov   [edi+LINE*4],dx
  mov   [edi+LINE*4+2],dx
  mov   edx,ecx
  shr   edx,26
  or    edx,ebx
  add   ecx,ebp
  %assign LINE LINE+1
%endrep
pphmap %+ LINE:
  mov   al,[esi+edx]            ; overwritten by the patched ret
  ret
