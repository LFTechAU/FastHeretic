;
; fhasm.asm : FastHeretic low and potato detail drawers (NASM)
;
; Adapted from FastDoom's linearl.asm / linearp.asm (Victor Nieto, GPL)
; which in turn come from id Software's and Raven Software's linear.asm.
;
; Low detail draws each column 2 pixels wide, potato 4 pixels wide.
; The column drawers read centery every call, since Heretic moves it
; when the player looks up or down.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;

BITS 32

%define SCREENWIDTH   320
%define SCREENHEIGHT  200

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

lowscalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    dd lvscale %+ LINE
  %assign LINE LINE+1
  %endrep

potscalecalls:
  %assign LINE 0
  %rep SCREENHEIGHT+1
    dd pvscale %+ LINE
  %assign LINE LINE+1
  %endrep

lowmapcalls:
  %assign LINE 0
  %rep (SCREENWIDTH/2)+1
    dd lhmap %+ LINE
  %assign LINE LINE+1
  %endrep

potmapcalls:
  %assign LINE 0
  %rep (SCREENWIDTH/4)+1
    dd phmap %+ LINE
  %assign LINE LINE+1
  %endrep

callpoint   dd 0
returnpoint dd 0

segment _TEXT public align=4 class=CODE USE32

;============================================================================
; Column setup shared by both detail levels
;   %1 = scale call table, %2 = detail shift (1 low, 2 potato)
;============================================================================

%macro COLUMN_ENTRY 2
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  mov   ebp,[_dc_yh]
  mov   eax,[_dc_yl]
  mov   edi,[_ylookup+ebp*4]
  sub   ebp,eax                 ; ebp = pixel count - 1
  js    %%done

  mov   ebx,[_dc_x]
  mov   ecx,[_dc_iscale]
  shl   ebx,%2
  add   edi,[_columnofs+ebx*4]

  sub   eax,[_centery]
  imul  ecx
  mov   edx,[_dc_texturemid]
  shl   ecx,9                   ; 7 significant bits, 25 frac
  add   edx,eax
  mov   esi,[_dc_source]
  shl   edx,9                   ; 7 significant bits, 25 frac
  mov   eax,[_dc_colormap]

  jmp   [%1+4+ebp*4]

%%done:
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret
%endmacro

%macro COLUMN_EXIT 0
  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret
%endmacro

;============================================================================
; R_DrawColumnLowA
;============================================================================

global R_DrawColumnLowA_
R_DrawColumnLowA_:
  COLUMN_ENTRY lowscalecalls, 1

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
lvscale %+ LINE:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]            ; get source pixel
  add   edx,ecx                 ; next location
  mov   bl,[eax]                ; translate the colour
  mov   bh,bl
  mov   [edi-(LINE-1)*SCREENWIDTH],bx
  %assign LINE LINE-1
%endrep

lvscale1:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]
  mov   bl,[eax]
  mov   bh,bl
  mov   [edi],bx
lvscale0:
  COLUMN_EXIT

;============================================================================
; R_DrawColumnPotatoA
;============================================================================

global R_DrawColumnPotatoA_
R_DrawColumnPotatoA_:
  COLUMN_ENTRY potscalecalls, 2

%assign LINE SCREENHEIGHT
%rep SCREENHEIGHT-1
pvscale %+ LINE:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]
  add   edx,ecx
  mov   bl,[eax]
  mov   bh,bl
  mov   [edi-(LINE-1)*SCREENWIDTH],bx
  mov   [edi-(LINE-1)*SCREENWIDTH+2],bx
  %assign LINE LINE-1
%endrep

pvscale1:
  mov   ebp,edx
  shr   ebp,25
  mov   al,[esi+ebp]
  mov   bl,[eax]
  mov   bh,bl
  mov   [edi],bx
  mov   [edi+2],bx
pvscale0:
  COLUMN_EXIT

;============================================================================
; Span setup: build Heretic's composite 6.10 x / 6.10 y position and step
;   %1 = map call table
;============================================================================

%macro SPAN_ENTRY 1
  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  mov   eax,[_ds_x1]
  mov   ebx,[_ds_x2]
  mov   eax,[%1+eax*4]
  mov   [callpoint],eax         ; spot to jump into unwound code
  mov   eax,[%1+4+ebx*4]
  mov   [returnpoint],eax       ; spot to patch a ret at
  mov   byte [eax],OP_RET

  ; composite position
  mov   ecx,[_ds_xfrac]
  shl   ecx,10
  and   ecx,0xffff0000
  mov   eax,[_ds_yfrac]
  shr   eax,6
  and   eax,0xffff
  or    ecx,eax

  ; composite step
  mov   ebp,[_ds_xstep]
  shl   ebp,10
  and   ebp,0xffff0000
  mov   eax,[_ds_ystep]
  shr   eax,6
  and   eax,0xffff
  or    ebp,eax

  mov   esi,[_ds_source]
  mov   edi,[_ds_y]
  mov   edi,[_ylookup+edi*4]
  add   edi,[_columnofs]

  mov   eax,[_ds_colormap]

  ; feed the pipeline and jump in
  shld  ebx,ecx,22              ; shift y units in
  shld  ebx,ecx,6               ; shift x units in
  and   ebx,0x0fff              ; mask off slop bits
  add   ecx,ebp
  call  [callpoint]

  mov   ebx,[returnpoint]
  mov   byte [ebx],OP_MOVAL     ; remove the ret patched in

  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret
%endmacro

;============================================================================
; R_DrawSpanLowA
;============================================================================

global R_DrawSpanLowA_
R_DrawSpanLowA_:
  SPAN_ENTRY lowmapcalls

%assign LINE 0
%rep SCREENWIDTH/2
lhmap %+ LINE:
  mov   al,[esi+ebx]            ; get source pixel
  shld  ebx,ecx,22              ; shift y units in
  mov   dl,[eax]                ; translate colour
  shld  ebx,ecx,6               ; shift x units in
  mov   dh,dl
  mov   [edi+LINE*2],dx         ; write 2 pixels
  and   ebx,0x0fff              ; mask off slop bits
  add   ecx,ebp                 ; position += step
  %assign LINE LINE+1
%endrep
lhmap %+ LINE:
  mov   al,[esi+ebx]            ; overwritten by the patched ret
  ret

;============================================================================
; R_DrawSpanPotatoA
;============================================================================

global R_DrawSpanPotatoA_
R_DrawSpanPotatoA_:
  SPAN_ENTRY potmapcalls

%assign LINE 0
%rep SCREENWIDTH/4
phmap %+ LINE:
  mov   al,[esi+ebx]
  shld  ebx,ecx,22
  mov   dl,[eax]
  shld  ebx,ecx,6
  mov   dh,dl
  mov   [edi+LINE*4],dx         ; write 4 pixels
  mov   [edi+LINE*4+2],dx
  and   ebx,0x0fff
  add   ecx,ebp
  %assign LINE LINE+1
%endrep
phmap %+ LINE:
  mov   al,[esi+ebx]            ; overwritten by the patched ret
  ret
