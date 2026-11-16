#!/bin/bash
# Bind a built LE executable to the DOS/32A extender (runs SB/SS under DOSBox)
# Usage: ./bind.sh build/386/fhtic.exe
set -e
ROOT=$(cd "$(dirname "$0")" && pwd)
EXEPATH=$(cd "$(dirname "$1")" && pwd)
NAME=$(basename "$1" | tr a-z A-Z)
cd "$EXEPATH"
[ -f "$(basename "$1")" ] && [ "$(basename "$1")" != "$NAME" ] && mv "$(basename "$1")" "$NAME"
mkdir -p DOS32A/BINW
cp "$ROOT"/dos32a/*.EXE DOS32A/BINW/
cp "$ROOT"/dos32a/DOS32A.D32 .
printf '@echo off\r\nsb /R %s > BIND.LOG\r\nss %s DOS32A.D32 >> BIND.LOG\r\nexit\r\n' "$NAME" "$NAME" > BIND.BAT
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 dosbox -exit \
  -c "mount c ." -c "c:" -c "set DOS32A=C:\\DOS32A" -c "set PATH=%PATH%;C:\\DOS32A\\BINW" -c "BIND.BAT" >/dev/null 2>&1 || true
tr -d '\r' < BIND.LOG
grep -q "successfully configured" BIND.LOG
rm -rf DOS32A DOS32A.D32 BIND.BAT BIND.LOG *.le *.LE
echo "Bound $EXEPATH/$NAME"
