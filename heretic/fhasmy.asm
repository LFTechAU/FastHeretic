;
; fhasmy.asm : FastHeretic Mode Y drawers (NASM)
;
; Column and span inner loops for drawing straight into a planar
; Mode Y page (80 bytes per row). Adapted from FastDoom's planar
; drawers (Victor Nieto, GPL) and Raven Software's linear.asm.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;

BITS 32

%define PLANEWIDTH    80
%define SCREENHEIGHT  200
%define MAXSPAN       80

extern _dc_colormap
extern _dc_x
extern _dc_yl
extern _dc_yh
extern _dc_iscale
extern _dc_texturemid
extern _dc_source
extern _centery
extern _viewwindowy

extern _my_ylookup
extern _my_ofs
extern _my_mask
extern _my_curmask

extern _ds_colormap
extern _ds_source
extern _ysp_dest
extern _ysp_count
extern _ysp_pos
extern _ysp_step

segment _DATA public align=4 class=DATA USE32

align 4

yscalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    dd yvscale %+ LINE
  %assign LINE LINE+1
  %endrep

ymapcalls:
  %assign LINE 0
  %rep MAXSPAN+1
    dd ymap %+ LINE
  %assign LINE LINE+1
  %endrep

segment _TEXT public align=4 class=CODE USE32

;============================================================================
; R_DrawColumnYA : one byte per row, map mask picks the plane(s)
;============================================================================

global R_DrawColumnYA_
R_DrawColumnYA_:
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  mov   ebx,[_dc_x]
  mov   al,[_my_mask+ebx]
  cmp   al,[_my_curmask]
  je    .masked
  mov   [_my_curmask],al
  mov   dx,0x3c5
  out   dx,al
.masked:
  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_my_ylookup+ebp*4]
  sub   ebp,eax                 ; ebp = pixel count - 1
  js    .done
  add   edi,[_my_ofs+ebx*4]

  mov   ecx,[_dc_iscale]
  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9                   ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9
  mov   eax,[_dc_colormap]

  jmp   [yscalecalls+4+ebp*4]

.done:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
yvscale %+ LINE:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]            ; get source pixel
  add   edx,ecx                 ; next location
  mov   bl,[eax]                ; translate the colour
  mov   [edi-(LINE-1)*PLANEWIDTH],bl
  %assign LINE LINE-1
%endrep

yvscale1:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]
  mov   bl,[eax]
  mov   [edi],bl
yvscale0:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret

;============================================================================
; R_DrawColumnYHA : half height Mode Y, only rows on even screen lines,
; one page row per 2 screen rows (shares the unrolled loop above)
;============================================================================

global R_DrawColumnYHA_
R_DrawColumnYHA_:
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  mov   ebx,[_dc_x]
  mov   al,[_my_mask+ebx]
  cmp   al,[_my_curmask]
  je    .masked
  mov   [_my_curmask],al
  mov   dx,0x3c5
  out   dx,al
.masked:
  mov   eax,[_dc_yl]
  mov   ecx,[_viewwindowy]
  lea   edx,[eax+ecx]
  and   edx,1
  add   eax,edx                 ; first row on an even screen line
  mov   ebp,[_dc_yh]
  lea   edx,[ebp+ecx]
  and   edx,1
  sub   ebp,edx                 ; last row on an even screen line
  mov   edi,[_my_ylookup+ebp*4]
  sub   ebp,eax
  js    .done
  shr   ebp,1                   ; ebp = page rows - 1
  add   edi,[_my_ofs+ebx*4]

  mov   ecx,[_dc_iscale]
  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  add   ecx,ecx                 ; two screen rows per page row
  shl   ecx,9
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9
  mov   eax,[_dc_colormap]

  jmp   [yscalecalls+4+ebp*4]

.done:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret

;============================================================================
; R_DrawSpanYCore : ysp_count bytes from ysp_dest, Heretic 6.10 composite
; position ysp_pos and step ysp_step (map mask already set)
;============================================================================

global R_DrawSpanYCore_
R_DrawSpanYCore_:
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  mov   ecx,[_ysp_pos]
  mov   ebp,[_ysp_step]
  mov   esi,[_ds_source]
  mov   eax,[_ds_colormap]
  mov   edx,[_ysp_count]
  mov   edi,[_ysp_dest]
  add   edi,edx                 ; ymapN writes [edi-N]

  shld  ebx,ecx,22              ; shift y units in
  shld  ebx,ecx,6               ; shift x units in
  and   ebx,0x0fff              ; mask off slop bits
  add   ecx,ebp

  jmp   [ymapcalls+edx*4]

%assign LINE MAXSPAN
%rep MAXSPAN
ymap %+ LINE:
  mov   al,[esi+ebx]            ; get source pixel
  shld  ebx,ecx,22
  mov   dl,[eax]                ; translate colour
  shld  ebx,ecx,6
  mov   [edi-LINE],dl
  and   ebx,0x0fff
  add   ecx,ebp
  %assign LINE LINE-1
%endrep
ymap0:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret
