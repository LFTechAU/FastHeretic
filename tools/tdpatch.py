#!/usr/bin/env python3
"""Patch the -timedemo bug in original Heretic / Hexen DOS executables.

G_TimeDemo reads skill/episode/map but never skips the per-player bytes
that follow in the demo header (4 bytes in Heretic, 8 in Hexen), so every
ticcmd is read out of step. This rewrites G_TimeDemo's header parsing in
place, keeping every relocated operand where it was.
"""
import re, sys

def patch(src, dst, skip):
    d = bytearray(open(src, 'rb').read())
    # mov edx,1 / call W_CacheLumpName / mov [demobuffer],eax /
    # inc eax / mov [demo_p],eax / inc eax / mov [demo_p],eax / inc eax /
    # xor ecx,ecx / xor ebx,ebx / mov cl,[eax-3] / xor edx,edx /
    # mov bl,[eax-1] / mov dl,[eax-2] / mov [demo_p],eax / mov eax,ecx
    pat = re.compile(rb'\xba\x01\x00\x00\x00\xe8....\xa3....'
                     rb'(\x40\xa3....\x40\xa3....\x40'
                     rb'\x31\xc9\x31\xdb\x8a\x48\xfd\x31\xd2\x8a\x58\xff\x8a\x50\xfe)'
                     rb'\xa3....\x89\xc8', re.S)
    hits = list(pat.finditer(d))
    if len(hits) != 1:
        sys.exit(f'{src}: expected one G_TimeDemo match, found {len(hits)}')
    r = hits[0].start(1)
    old = bytes(d[r:r + 28])
    sk = 3 + skip
    new = bytearray(old)
    new[0] = 0x90          # nop
    new[1] = 0x3d          # cmp eax,imm32 (relocated operand kept, harmless)
    new[6] = 0x90
    new[7] = 0x3d
    new[12] = 0x90
    new[13:16] = bytes([0x83, 0xc0, sk])                  # add eax,3+skip
    new[16:20] = bytes([0x0f, 0xb6, 0x48, (-sk) & 0xff])  # movzx ecx,[eax-sk]   skill
    new[20:24] = bytes([0x0f, 0xb6, 0x58, (2 - sk) & 0xff])  # movzx ebx,[eax+2-sk] map
    new[24:28] = bytes([0x0f, 0xb6, 0x50, (1 - sk) & 0xff])  # movzx edx,[eax+1-sk] episode
    d[r:r + 28] = new
    open(dst, 'wb').write(d)
    print(f'{src}: patched at file offset {r:#x}')
    print('  old', old.hex())
    print('  new', bytes(new).hex())

if __name__ == '__main__':
    patch(sys.argv[1], sys.argv[2], int(sys.argv[3]))
