FastHexen
=========

FastHexen is a DOS port of Hexen: Beyond Heretic (Raven Software, 1995)
tuned for 386 and 486 PCs, in the spirit of FastDoom. It is the sister
of FastHeretic and shares its renderer, video modes, sound library and
options. It is an LFTech project.

You need your own copy of Hexen: put FHEX.EXE in the same directory as
HEXEN.WAD. FastHexen is built from the Hexen v1.0 source and tested with
the v1.0 HEXEN.WAD. No game data is included here.

Executables
-----------

  FHEX.EXE     Mode Y, built for 386 (also the safest choice)
  FHEX4.EXE    Mode Y, built for 486 / 5x86
  FHEX5.EXE    Mode Y, built for Pentium
  FHEX13H.EXE  Mode 13h, built for 386 (like the original HEXEN.EXE)
  FHEXH.EXE    Mode Y at half height (320x100, each line shown twice
               as tall)
  FHEXX.EXE    Mode X 320x240, square pixels, 3 pages
  FHEXVBD.EXE  VESA 320x200 linear frame buffer, 3 pages
  FHEXVBR.EXE  VESA 320x200, copied from system memory each frame

High resolution VESA (the 3D view at full resolution; menus and the
status bar are scaled up). D = straight into video memory, 3 pages (needs
a linear frame buffer); R = drawn in memory and copied (any VESA card):

  FXM240D/R    320x240      FXM600D/R    800x600
  FXM300D/R    400x300      FXM768D/R    1024x768
  FXM384D/R    512x384      FXM800D/R    1280x800
  FXM400D/R    640x400      FXM1024D/R   1280x1024
  FXM480D/R    640x480      FXM1200D/R   1600x1200

Other video cards (all built for 386, from FastDoom's video code):

  FHEXEGA  EGA            FHEXCGA  CGA 4 colours    FHEXBWC  CGA mono
  FHEXC16  CGA 160x100    FHEXCAH  CGA ANSI from Hell
  FHEXCVB  CGA composite  FHEX512  CGA 512 colours  FHEXPCP  Plantronics
  FHEX400  Sigma Color 400                          FHEXHGC  Hercules
  FHEXINC  Hercules InColor
  FHEXT1 FHEXT12 FHEXT25 FHEXT43 FHEXT50   text modes 40x25 to 80x50
  FHEXMDA  MDA            FHEXCDA  colour MDA       FHEXVT   VT100 serial

These show the startup messages as text instead of Hexen's 640x480
picture, since the cards they are for cannot show it. -textstartup does
the same with any of the programs.

FHEX files use the DOS/32A extender, so DOS4GW.EXE is not needed. Run
FHXSETUP.EXE to pick the sound cards (and a few display options) and the
original SETUP.EXE for the controls. Each one keeps the other's settings
in HEXEN.CFG.

Benchmark:  FHEX -timedemo demo1

Vanilla Hexen's -timedemo forgot to read the players and classes from
the demo, so it fell out of step at once; FastHexen plays all three
demos through. -bench works as in FastHeretic, with the same .BNC files.

Sound
-----

FastDoom's sound library, with the same cards as FastHeretic. Hexen's
own CD music (card 10) is kept. FastDoom's extra cards are numbered one
higher than in HERETIC.CFG, so let FHXSETUP.EXE set them.

Speed options
-------------

Options > FASTHEXEN... in the game menu, or on the command line. These
are the same as FastHeretic's (see README.TXT there), except that F5 is
still Hexen's suicide key, so the detail level is in the menu or -high,
-low, -potato.

  DETAIL     HIGH, LOW (2-pixel columns) or POTATO (4-pixel columns)
  FLOORS     TEXTURED, FLAT LIT or FLAT
  SKY        NORMAL or FLAT (both of Hexen's sky layers)
  GHOSTS     SEE-THRU, SOLID or SATURN
  FPS        frame rate counter
  ITEMS      ALL or NEAR
  UNCAPPED   frames between tics, with smooth movement (single player)
  CPU        AUTO, or the CPU to tune the drawers for

Settings are saved to HEXEN.CFG (the original HEXEN.EXE ignores them).

Changes
-------

v0.1  First FastHexen: everything in FastHeretic v0.7 (detail levels,
      flat floors and sky, ghost options, per-CPU drawers, Mode Y, Mode X,
      VESA up to 1600x1200, FastDoom's CGA/EGA/Hercules/text modes,
      uncapped frame rate, benchmark mode, FastDoom's sound library and
      a setup program), plus the -timedemo fix.

Credits and licence
-------------------

  Hexen source: Raven Software / id Software, released under GPL v2
  Source restoration: NY00123's gamesrc-ver-recreation project
  FastDoom (ideas, video modes and sound library): Victor Nieto (viti95)
  and contributors, GPL
  DOS/32A extender: Narech K.

Licensed under the GNU GPL v2 (see hexen/license.txt).
