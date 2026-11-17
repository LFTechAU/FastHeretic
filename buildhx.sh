#!/bin/bash
# FastHexen build script (OpenWatcom 2 on Linux)
# Usage: ./buildhx.sh [cpu] [mode]      cpu = 386 (default) | 486 | 586
set -e
ROOT=$(cd "$(dirname "$0")" && pwd)
CPU=${1:-386}
MODE=${2:-13h}
case "$CPU" in
  386) CPUOPT="-3r"; SUF="" ;;
  486) CPUOPT="-4r"; SUF="4" ;;
  586) CPUOPT="-5r"; SUF="5" ;;
  *) echo "unknown cpu $CPU"; exit 1 ;;
esac
case "$MODE" in
  13h) MODEDEF=""; EXE=fhex13h$SUF.exe ;;
  *) echo "unknown mode $MODE"; exit 1 ;;
esac
OUT="$ROOT/build/hx$CPU-$MODE"
mkdir -p "$OUT"
cd "$OUT"
find . -maxdepth 1 -name '*.obj' -delete
export INCLUDE="$WATCOM/h"
CCOPTS="-dAPPVER_EXEDEF=HEX10B -dFH_FDSOUND $MODEDEF -omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos -i=$ROOT/hexen -i=$ROOT/heretic/fdsnd"
FDOPTS="-omaxtnrih -ol+ -oe=32 -zp1 $CPUOPT -ei -j -zq -zc -bt=dos"
HEXEN_C="am_map a_action ct_chat d_net f_finale g_game h2_main info in_lude mn_menu m_misc \
 p_acs p_anim p_ceilng p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats \
 p_pspr p_setup p_sight p_spec p_switch p_telept p_tick p_things p_user po_man r_bsp r_data \
 r_draw r_main r_plane r_segs r_things sb_bar sc_man sn_sonix st_start sv_save sounds tables \
 v_video w_wad z_zone i_cdmus i_cyber i_ibm i_sound"
FDSND_C="ns_adbfx ns_awe32 ns_cd ns_cms ns_dma ns_dpmi ns_dsney ns_esfm ns_fxm ns_gold ns_gus ns_gusau \
 ns_gusmi ns_imfc ns_irq ns_llm ns_lpt ns_lptmusic ns_midi ns_mpu401 ns_multi ns_music ns_pas16 ns_pcfx \
 ns_pwm ns_rs232 ns_sb ns_sbdm ns_sbmid ns_sbmus ns_scape ns_speak ns_tandy ns_task ns_wss \
 mus2mid fhdmx tsmapi dpmiapi"
for f in $HEXEN_C; do wcc386 $CCOPTS -fo=$f.obj "$ROOT/hexen/$f.c"; done
for f in $FDSND_C; do wcc386 $FDOPTS -i=$ROOT/heretic/fdsnd/compat -i=$ROOT/heretic/fdsnd -fo=$f.obj "$ROOT/heretic/fdsnd/$f.c"; done
nasm -f obj -i "$ROOT/heretic/fdsnd/" -o ns_mix.obj "$ROOT/heretic/fdsnd/ns_mix.asm"
for f in i_ibm_a linear; do wasm -zq -fo=$f.obj "$ROOT/hexen/$f.asm"; done
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
  for f in $HEXEN_C $FDSND_C ns_mix i_ibm_a linear; do echo "file $f.obj"; done
} > link.lnk
wlink @link.lnk
echo "Built $OUT/$EXE"
