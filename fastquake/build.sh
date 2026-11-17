#!/bin/sh
# Build every FastQuake executable into dist/
set -e
cd "$(dirname "$0")"
DJGPP=${DJGPP:-/tmp/claude-0/djgpp}
export DJGPP
make B=build/486 EXE=FQUAKE.EXE CPU="-march=i486 -mtune=i486" DEFS='-DFQ_CPUNAME=\"486\"' "$@"
make B=build/586 EXE=FQUAKE5.EXE CPU="-march=pentium -mtune=pentium" DEFS='-DFQ_CPUNAME=\"Pentium\" -DFQ_DEFSPANS=\"0\"' "$@"
make B=build/386 EXE=FQUAKE3.EXE CPU="-march=i386 -mtune=i386" DEFS='-DFQ_CPUNAME=\"386\"' "$@"
mkdir -p dist
cp build/486/FQUAKE.EXE build/586/FQUAKE5.EXE build/386/FQUAKE3.EXE dist/
cp src/cwsdpmi.exe dist/CWSDPMI.EXE
sed 's/$/\r/' README.md > dist/README.TXT
cp BENCH.BAT dist/BENCH.BAT
cp LICENSE.TXT dist/LICENSE.TXT
mkdir -p dist/ID1
cp bench/*.CFG dist/ID1/
