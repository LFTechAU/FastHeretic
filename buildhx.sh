#!/bin/bash
# FastHexen build script (OpenWatcom 2 on Linux)
# Usage: ./buildhx.sh [cpu] [mode]      cpu = 386 (default) | 486 | 586
#   mode = the same as build.sh (y, 13h, yh, x, vbr, vbd, m480r ..., cga ..., t25 ..., mda, vt)
set -e
ROOT=$(cd "$(dirname "$0")" && pwd)
# ./buildhx.sh setup : the FHXSETUP.EXE setup program
if [ "$1" = setup ]; then
  mkdir -p "$ROOT/build/hxsetup" && cd "$ROOT/build/hxsetup"
  export INCLUDE="$WATCOM/h"
  wcc386 -3r -ox -zq -bt=dos -dFH_HEXEN -fo=fhxsetup.obj "$ROOT/setup/fhsetup.c"
  wlink option quiet format os2 le option stub=$ROOT/dos32a/STUB32A.EXE \
    libpath %WATCOM%/lib386 libpath %WATCOM%/lib386/dos name fhxsetup file fhxsetup.obj
  echo "Built $ROOT/build/hxsetup/fhxsetup.exe"
  exit 0
fi

CPU=${1:-386}
MODE=${2:-y}
case "$CPU" in
  386) CPUOPT="-3r"; SUF="" ;;
  486) CPUOPT="-4r"; SUF="4" ;;
  586) CPUOPT="-5r"; SUF="5" ;;
  *) echo "unknown cpu $CPU"; exit 1 ;;
esac
EXTRAOBJ=""
RES=""
FDC=""      # FastDoom video mode C file (heretic/fd)
FDASM=""    # FastDoom video mode asm file (heretic/fd)
fdmode() { MODEDEF="-dFH_BACKBUF -d$1"; NASMDEF="-D$1"; FDC=$2; FDASM=$3; EXE=fhex$4$SUF.exe; }
case "$MODE" in
  y)   MODEDEF="-dFH_MODEY"; EXE=fhex$SUF.exe; EXTRAOBJ="fhasmy" ;;
  13h) MODEDEF=""; EXE=fhex13h.exe; [ -n "$SUF" ] && EXE=fhx13h$SUF.exe ;;  # 8.3 names
  yh)  MODEDEF="-dFH_MODEY -dFH_MODEYH"; EXE=fhexh$SUF.exe; EXTRAOBJ="fhasmy" ;;
  vbr) MODEDEF="-dFH_VBR"; EXE=fhexvbr$SUF.exe ;;
  vbd) MODEDEF="-dFH_VBD"; EXE=fhexvbd$SUF.exe ;;
  x)   MODEDEF="-dFH_MODEY -dFH_MODEX"; EXE=fhexx$SUF.exe; EXTRAOBJ="fhasmy"; RES=320x240 ;;
  m*r|m*d) # high resolution VESA: m480r = 640x480 copied, m480d = direct
       R=${MODE:1:${#MODE}-2}; K=${MODE: -1}
       case $R in 240) RES=320x240;; 300) RES=400x300;; 384) RES=512x384;;
         400) RES=640x400;; 480) RES=640x480;; 600) RES=800x600;; 768) RES=1024x768;;
         800) RES=1280x800;; 1024) RES=1280x1024;; 1200) RES=1600x1200;;
         *) echo "unknown resolution $R"; exit 1;; esac
       [ $K = r ] && MODEDEF="-dFH_VBR" || MODEDEF="-dFH_VBD"
       EXE=fxm$R$K.exe ;;
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
WASMDEF=""; NASMRES=""
if [ -n "$RES" ]; then
  RW=${RES%x*}; RH=${RES#*x}
  MODEDEF="$MODEDEF -dFH_RW=$RW -dFH_RH=$RH"
  WASMDEF="-dFHRW=$RW -dFHRH=$RH"
  NASMRES="-DSCREENWIDTH=$RW -DSCREENHEIGHT=$RH"
fi

OUT="$ROOT/build/hx$CPU-$MODE"
mkdir -p "$OUT"
cd "$OUT"
find . -maxdepth 1 -name '*.obj' -delete
export INCLUDE="$WATCOM/h"
CCOPTS="-dAPPVER_EXEDEF=HEX10B -dFH_FDSOUND $MODEDEF -omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos -i=$ROOT/hexen -i=$ROOT/heretic -i=$ROOT/heretic/fdsnd"
FDOPTS="-omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos"
HEXEN_C="am_map a_action ct_chat d_net f_finale g_game h2_main info in_lude mn_menu m_misc \
 p_acs p_anim p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats \
 p_pspr p_setup p_sight p_spec p_switch p_telept p_tick p_things p_user po_man r_bsp r_data \
 r_draw r_main r_plane r_segs r_things sb_bar sc_man sn_sonix st_start sv_save sounds tables \
 v_video w_wad z_zone i_cdmus i_cyber i_ibm i_sound"
SHARED_C="r_fhdraw fh_cpu fh_uncap fh_bench i_text fh_modey fhvideo fh_vesa fh_hires"   # shared with FastHeretic (heretic/)
FDSND_C="ns_adbfx ns_awe32 ns_cd ns_cms ns_dma ns_dpmi ns_dsney ns_esfm ns_fxm ns_gold ns_gus ns_gusau \
 ns_gusmi ns_imfc ns_irq ns_llm ns_lpt ns_lptmusic ns_midi ns_mpu401 ns_multi ns_music ns_pas16 ns_pcfx \
 ns_pwm ns_rs232 ns_sb ns_sbdm ns_sbmid ns_sbmus ns_scape ns_speak ns_tandy ns_task ns_wss \
 mus2mid fhdmx tsmapi dpmiapi"
for f in $HEXEN_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/hexen/$f.c"; done
for f in $SHARED_C; do cp "$ROOT/heretic/$f.c" "_$f.c"; wcc386 $CCOPTS -fo=$f.obj "_$f.c"; done   # copied: wcc looks in the source folder first
for f in $FDSND_C; do wcc386 $FDOPTS -i=$ROOT/heretic/fdsnd/compat -i=$ROOT/heretic/fdsnd -fo=$f.obj "$ROOT/heretic/fdsnd/$f.c"; done
nasm -f obj -i "$ROOT/heretic/fdsnd/" -o ns_mix.obj "$ROOT/heretic/fdsnd/ns_mix.asm"
FDOBJ=""
if [ -n "$FDC" ]; then
  wcc386 $CCOPTS -i=$ROOT/heretic/fd/compat -i=$ROOT/heretic/fd -fo=$FDC.obj "$ROOT/heretic/fd/$FDC.c"
  FDOBJ="$FDOBJ $FDC"
fi
if [ -n "$FDASM" ]; then
  nasm -f obj $NASMDEF -i "$ROOT/heretic/fd/" -o $FDASM.obj "$ROOT/heretic/fd/$FDASM.asm"
  FDOBJ="$FDOBJ $FDASM"
fi
for f in i_ibm_a linear; do wasm -zq $WASMDEF -fo=$f.obj "$ROOT/hexen/$f.asm"; done
for f in fhasm fhcpu $EXTRAOBJ; do nasm -f obj $NASMRES -o $f.obj "$ROOT/heretic/$f.asm"; done
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
  echo "libpath $ROOT/heretic/fdsnd"
  echo "lib gf1_osf.lib"
  echo "lib pawe32.lib"
  echo "option stub=$ROOT/dos32a/STUB32A.EXE"
  for f in $HEXEN_C $SHARED_C $FDSND_C ns_mix i_ibm_a linear fhasm fhcpu $EXTRAOBJ $FDOBJ; do echo "file $f.obj"; done
} > link.lnk
wlink @link.lnk
echo "Built $OUT/$EXE"
