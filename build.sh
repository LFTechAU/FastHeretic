#!/bin/bash
# FastHeretic build script (OpenWatcom 2 on Linux)
# Usage: ./build.sh [cpu] [mode]
#   cpu  = 386 (default) | 486 | 586 | stock
#   mode = y (default, Mode Y straight to video memory) | 13h
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
case "$MODE" in
  y)   MODEDEF="-dFH_MODEY"; EXE=fhtic$SUF.exe; EXTRAOBJ="fhasmy" ;;
  13h) MODEDEF=""; EXE=fhtic13h$SUF.exe ;;
  *) echo "unknown mode $MODE"; exit 1 ;;
esac
[ "$CPU" = stock ] && EXE=htic13f.exe

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
 r_main r_plane r_segs r_things sb_bar sounds tables v_video w_wad z_zone i_text r_fhdraw fh_modey"
APODMX_C="dmx dpmiapi mus2mid tsmapi usrhooks"

for f in $HERETIC_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/heretic/$f.c"; done
for f in $APODMX_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/apodmx/$f.c"; done
for f in i_ibm_a linear; do wasm -zq -fo=$f.obj "$ROOT/heretic/$f.asm"; done
nasm -f obj -o fhasm.obj "$ROOT/heretic/fhasm.asm"
for f in $EXTRAOBJ; do nasm -f obj -o $f.obj "$ROOT/heretic/$f.asm"; done

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
