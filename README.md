FastHeretic
===========

FastHeretic is a DOS port of Heretic (Raven Software, 1994) tuned for
386 and 486 PCs, in the spirit of FastDoom. It is an LFTech project.

You need your own copy of Heretic: put FHTIC.EXE in the same directory
as HERETIC.WAD (registered or Shadow of the Serpent Riders) or
HERETIC1.WAD (shareware). No game data is included here.

Executables
-----------

  FHTIC.EXE    built for 386 (also the safest choice)
  FHTIC4.EXE   built for 486 / 5x86
  FHTIC5.EXE   built for Pentium

All use the DOS/32A extender, so DOS4GW.EXE is not needed.
Use the original SETUP.EXE to configure sound and controls.

Benchmark:  FHTIC -timedemo demo1

Status
------

v0.1: stock Heretic v1.3 code built with OpenWatcom 2, the Apogee Sound
System based DMX replacement, and DOS/32A. Renderer speedups from FastDoom
are being ported next.

Credits and licence
-------------------

  Heretic source: Raven Software / id Software, released under GPL v2
  Source restoration: NY00123's gamesrc-ver-recreation project
  DMX replacement (APODMX): Nuke.YKT and NY00123
  Apogee Sound System: Jim Dose / 3D Realms, GPL
  FastDoom (ideas and code being ported): Victor Nieto (viti95), GPL
  DOS/32A extender: Narech K.

Licensed under the GNU GPL v2 (see heretic/license.txt).
