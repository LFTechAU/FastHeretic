Video mode code from FastDoom 1.3.0 (https://github.com/viti95/FastDoom),
by Victor Nieto and contributors, GPL v2.

Changes for FastHeretic:
- I_ProcessPalette converts one palette at a time (FD_NUMPAL) and the
  palette-index I_SetPalette is renamed FD_SetPaletteIdx, because
  Heretic passes palettes by pointer (fhvideo.c rebuilds the table).
- ega320.asm: clear ecx before use (its upper half leaked into the
  latch table address).
- cgaafh.asm: save and restore ebp.
- i_sigma.c: reset the VRAM pointer instead of writing through it.
- compat/ holds small headers standing in for FastDoom's own.
