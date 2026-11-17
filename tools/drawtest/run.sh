#!/bin/bash
# Check the CPU-specific drawers give the same pixels as the reference ones.
# Needs nasm, gcc and ld on x86 Linux (no 32-bit libc needed).
set -e
H=$(cd "$(dirname "$0")/../../heretic" && pwd)
T=$(mktemp -d)
for f in fhcpu fhasm fhasmy; do
  sed -e 's/^segment _DATA.*/section .data/' -e 's/^segment _TEXT.*/section .text/' "$H/$f.asm" > "$T/$f.asm"
  nasm -f elf32 -o "$T/$f.o" "$T/$f.asm"
done
gcc -m32 -O1 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -c "$(dirname "$0")/drawtest.c" -o "$T/t.o"
ld -m elf_i386 -N -e _start -o "$T/drawtest" "$T"/*.o 2>/dev/null
"$T/drawtest"
rm -rf "$T"
