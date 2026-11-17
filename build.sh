#!/bin/bash
# FastHeretic build script (OpenWatcom 2 on Linux)
# Usage: ./build.sh [cpu] [mode]
#   cpu  = 386 (default) | 486 | 586 | stock
#   mode = y (default, Mode Y straight to video memory) | 13h | yh (half height) | vbr vbd (VESA)
#          | cga bwc c16 512 cah cvb ega hgc inc pcp 400 (FastDoom modes)
#          | t1 t12 t25 t43 t50 mda cda vt (text modes)
#          | x (Mode X 320x240) | m240r m240d ... m1200r m1200d (high resolution VESA)
# "stock" = plain Heretic v1.3 settings (original compiler flags, DOS/4GW,
# FastHeretic options off) with only the -timedemo fix: a fair baseline.
set -e

CPU=${1:-386}
MODE=${2:-y}
[ "$CPU" = stock ] && MODE=13h
case "$CPU" in
  386) CPUOPT="-3r"; SUF="" ;;
  486) CPUOPT="-4r"; SUF="4" ;;
  586) CPUOPT="-5r"; SUF="5" ;;
  stock) CPUOPT="-4r" ;;
  *) echo "unknown cpu $CPU"; exit 1 ;;
esac
EXTRAOBJ=""
RES=""
FDC=""      # FastDoom video mode C file (heretic/fd)
FDASM=""    # FastDoom video mode asm file (heretic/fd)
fdmode() { MODEDEF="-dFH_BACKBUF -d$1"; NASMDEF="-D$1"; FDC=$2; FDASM=$3; EXE=fhtic$4$SUF.exe; }
case "$MODE" in
  y)   MODEDEF="-dFH_MODEY"; EXE=fhtic$SUF.exe; EXTRAOBJ="fhasmy" ;;
  13h) MODEDEF=""; EXE=fhtic13h$SUF.exe ;;
  yh)  MODEDEF="-dFH_MODEY -dFH_MODEYH"; EXE=fhtich$SUF.exe; EXTRAOBJ="fhasmy" ;;
  vbr) MODEDEF="-dFH_VBR"; EXE=fhticvbr$SUF.exe ;;
  vbd) MODEDEF="-dFH_VBD"; EXE=fhticvbd$SUF.exe ;;
  x)   MODEDEF="-dFH_MODEY -dFH_MODEX"; EXE=fhticx$SUF.exe; EXTRAOBJ="fhasmy"; RES=320x240 ;;
  m*r|m*d) # high resolution VESA: m480r = 640x480 copied, m480d = direct
       R=${MODE:1:${#MODE}-2}; K=${MODE: -1}
       case $R in 240) RES=320x240;; 300) RES=400x300;; 384) RES=512x384;;
         400) RES=640x400;; 480) RES=640x480;; 600) RES=800x600;; 768) RES=1024x768;;
         800) RES=1280x800;; 1024) RES=1280x1024;; 1200) RES=1600x1200;;
         *) echo "unknown resolution $R"; exit 1;; esac
       [ $K = r ] && MODEDEF="-dFH_VBR" || MODEDEF="-dFH_VBD"
       EXE=fhm$R$K.exe ;;
  cga) fdmode MODE_CGA     i_cga4    cga4   cga ;;
  bwc) fdmode MODE_CGA_BW  i_cgabw   cgabw  bwc ;;
  c16) fdmode MODE_CGA16   i_cga16   cga16  c16 ;;
  512) fdmode MODE_CGA512  i_cga512  cga512 512 ;;
  cah) fdmode MODE_CGA_AFH i_cgaafh  cgaafh cah ;;
  cvb) fdmode MODE_CVB     i_cgacvb  cgacvb cvb ;;
  ega) fdmode MODE_EGA     i_ega320  ega320 ega ;;
  hgc) fdmode MODE_HERC    i_herc    herc   hgc ;;
  inc) fdmode MODE_INCOLOR i_incolor ""     inc ;;
  pcp) fdmode MODE_PCP     i_pcp     pcp    pcp ;;
  400) fdmode MODE_SIGMA   i_sigma   sigma  400 ;;
  t1)  fdmode MODE_T4025   "" "" t1 ;;
  t12) fdmode MODE_T4050   "" "" t12 ;;
  t25) fdmode MODE_T8025   "" "" t25 ;;
  t43) fdmode MODE_T8043   "" "" t43 ;;
  t50) fdmode MODE_T8050   "" "" t50 ;;
  mda) fdmode MODE_MDA     "" "" mda ;;
  cda) fdmode MODE_CMDA    "" "" cda ;;
  vt)  fdmode MODE_VT100   "" "" vt ;;
  *) echo "unknown mode $MODE"; exit 1 ;;
esac
[ "$CPU" = stock ] && EXE=htic13f.exe
WASMDEF=""; NASMRES=""
if [ -n "$RES" ]; then
  RW=${RES%x*}; RH=${RES#*x}
  MODEDEF="$MODEDEF -dFH_RW=$RW -dFH_RH=$RH"
  WASMDEF="-dFHRW=$RW -dFHRH=$RH"
  NASMRES="-DSCREENWIDTH=$RW -DSCREENHEIGHT=$RH"
fi

export INCLUDE="$WATCOM/h"
ROOT=$(cd "$(dirname "$0")" && pwd)
OUT="$ROOT/build/$CPU-$MODE"
mkdir -p "$OUT"
cd "$OUT"
find . -maxdepth 1 -name '*.obj' -delete

if [ "$CPU" = stock ]; then
  CCOPTS="-dAPPVER_EXEDEF=TIC13 -omaxet -zp1 -4r -ei -j -zq -bt=dos -i=$ROOT/heretic -i=$ROOT/apodmx"
  STUB="$WATCOM/binw/wstub.exe"
else
  STUB="$ROOT/dos32a/STUB32A.EXE"
CCOPTS="-dAPPVER_EXEDEF=TIC13 ${CCEXTRA} $MODEDEF -omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos -i=$ROOT/heretic -i=$ROOT/apodmx"
fi

HERETIC_C="am_map ct_chat d_main d_net f_finale g_game info in_lude i_cyber i_ibm i_sound \
 mn_menu m_misc p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj \
 p_plats p_pspr p_setup p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw \
 r_main r_plane r_segs r_things sb_bar sounds tables v_video w_wad z_zone i_text r_fhdraw fh_modey fhvideo fh_vesa fh_uncap fh_hires"
APODMX_C="dmx dpmiapi mus2mid tsmapi usrhooks"

for f in $HERETIC_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/heretic/$f.c"; done
for f in $APODMX_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/apodmx/$f.c"; done
for f in i_ibm_a linear; do wasm -zq $WASMDEF -fo=$f.obj "$ROOT/heretic/$f.asm"; done
nasm -f obj $NASMRES -o fhasm.obj "$ROOT/heretic/fhasm.asm"
for f in $EXTRAOBJ; do nasm -f obj $NASMRES -o $f.obj "$ROOT/heretic/$f.asm"; done
if [ -n "$FDC" ]; then
  wcc386 $CCOPTS -i=$ROOT/heretic/fd/compat -i=$ROOT/heretic/fd -fo=$FDC.obj "$ROOT/heretic/fd/$FDC.c"
  EXTRAOBJ="$EXTRAOBJ $FDC"
fi
if [ -n "$FDASM" ]; then
  nasm -f obj $NASMDEF -i "$ROOT/heretic/fd/" -o $FDASM.obj "$ROOT/heretic/fd/$FDASM.asm"
  EXTRAOBJ="$EXTRAOBJ $FDASM"
fi

{
  echo "option map"
  echo "option stack=65536"
  echo "option caseexact"
  echo "option quiet"
  echo "option eliminate"
  echo "format os2 le"
  echo "name ${EXE%.exe}"
  echo "libpath %WATCOM%/lib386"
  echo "libpath %WATCOM%/lib386/dos"
  echo "libpath $ROOT/audiolib"
  echo "lib audio_wf.lib"
  echo "lib gf1_osf.lib"
  echo "lib pawe32.lib"
  echo "option stub=$STUB"
  for f in $HERETIC_C $APODMX_C i_ibm_a linear fhasm $EXTRAOBJ; do echo "file $f.obj"; done
} > link.lnk

wlink @link.lnk
echo "Built $OUT/$EXE"
