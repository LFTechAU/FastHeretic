#!/bin/bash
# FastHeretic build script (OpenWatcom 2 on Linux)
# Usage: ./build.sh [cpu]   cpu = 386 (default) | 486 | 586
set -e

CPU=${1:-386}
case "$CPU" in
  386) CPUOPT="-3r"; EXE=fhtic.exe ;;
  486) CPUOPT="-4r"; EXE=fhtic4.exe ;;
  586) CPUOPT="-5r"; EXE=fhtic5.exe ;;
  *) echo "unknown cpu $CPU"; exit 1 ;;
esac

export INCLUDE="$WATCOM/h"
ROOT=$(cd "$(dirname "$0")" && pwd)
OUT="$ROOT/build/$CPU"
mkdir -p "$OUT"
cd "$OUT"
find . -maxdepth 1 -name '*.obj' -delete

CCOPTS="-dAPPVER_EXEDEF=TIC13 ${CCEXTRA} -omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos -i=$ROOT/heretic -i=$ROOT/apodmx"

HERETIC_C="am_map ct_chat d_main d_net f_finale g_game info in_lude i_cyber i_ibm i_sound \
 mn_menu m_misc p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj \
 p_plats p_pspr p_setup p_sight p_spec p_switch p_telept p_tick p_user r_bsp r_data r_draw \
 r_main r_plane r_segs r_things sb_bar sounds tables v_video w_wad z_zone i_text r_fhdraw"
APODMX_C="dmx dpmiapi mus2mid tsmapi usrhooks"

for f in $HERETIC_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/heretic/$f.c"; done
for f in $APODMX_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/apodmx/$f.c"; done
for f in i_ibm_a linear; do wasm -zq -fo=$f.obj "$ROOT/heretic/$f.asm"; done
nasm -f obj -o fhasm.obj "$ROOT/heretic/fhasm.asm"

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
  echo "option stub=$ROOT/dos32a/STUB32A.EXE"
  for f in $HERETIC_C $APODMX_C i_ibm_a linear fhasm; do echo "file $f.obj"; done
} > link.lnk

wlink @link.lnk
echo "Built $OUT/$EXE"
