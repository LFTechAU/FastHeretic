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

Speed options
-------------

Options > FASTHERETIC... in the game menu, or on the command line:

  F5 / DETAIL     HIGH, LOW (2-pixel columns) or POTATO (4-pixel columns)
  FLOORS          TEXTURED or FLAT (one colour per floor, still lit)
  SKY             NORMAL or FLAT (one colour)
  GHOSTS          SEE-THRU or SOLID (faster translucent sprites)
  FPS             frame rate counter, top right

  -high -low -potato -flatsurfaces -flatsky -notrans -fps

Settings are saved to HERETIC.CFG (the original HERETIC.EXE ignores them).

Changes
-------

v0.2.1 Fix -timedemo (vanilla Heretic misread every demo in timedemo mode).
v0.2  Low and Potato detail with hand-tuned assembly drawers (from
      FastDoom), flat floors/ceilings, flat sky, solid ghosts, FPS
      counter, FASTHERETIC options menu page, F5 detail key.
v0.1  Stock Heretic v1.3 code built with OpenWatcom 2, the Apogee Sound
      System based DMX replacement, and DOS/32A.

Credits and licence
-------------------

  Heretic source: Raven Software / id Software, released under GPL v2
  Source restoration: NY00123's gamesrc-ver-recreation project
  DMX replacement (APODMX): Nuke.YKT and NY00123
  Apogee Sound System: Jim Dose / 3D Realms, GPL
  FastDoom (ideas and code being ported): Victor Nieto (viti95), GPL
  DOS/32A extender: Narech K.

Licensed under the GNU GPL v2 (see heretic/license.txt).
