FastHeretic
===========

FastHeretic is a DOS port of Heretic (Raven Software, 1994) tuned for
386 and 486 PCs, in the spirit of FastDoom. It is an LFTech project.

You need your own copy of Heretic: put FHTIC.EXE in the same directory
as HERETIC.WAD (registered or Shadow of the Serpent Riders) or
HERETIC1.WAD (shareware). No game data is included here.

Executables
-----------

  FHTIC.EXE    Mode Y, built for 386 (also the safest choice)
  FHTIC4.EXE   Mode Y, built for 486 / 5x86
  FHTIC5.EXE   Mode Y, built for Pentium
  FHTIC13H.EXE Mode 13h, built for 386
  FHTICH.EXE   Mode Y at half height (320x100, each line shown twice
               as tall): about a quarter faster again

Other video cards (all built for 386, from FastDoom's video code):

  FHTICEGA.EXE EGA 320x200, 16 colours
  FHTICCGA.EXE CGA 320x200, 4 colours (-palette1 for cyan/magenta)
  FHTICBWC.EXE CGA 640x200, black and white
  FHTICC16.EXE CGA 160x100, 16 colours (text mode trick)
  FHTICCAH.EXE CGA "ANSI from Hell" 320x100, 16 colours
  FHTICCVB.EXE CGA composite 160x200, 16 colours
  FHTIC512.EXE CGA composite 80x100, 512 colours (-oldcga for an
               early IBM CGA)
  FHTICPCP.EXE Plantronics ColorPlus 320x200, 16 colours
  FHTIC400.EXE Sigma Color 400 320x200, 16 colours
  FHTICHGC.EXE Hercules 640x400, black and white
  FHTICINC.EXE Hercules InColor 320x200, 16 colours
  FHTICT1.EXE  40x25 text
  FHTICT12.EXE 40x25 text, 40x50 blocks
  FHTICT25.EXE 80x25 text, 80x50 blocks
  FHTICT43.EXE 80x43 text, 80x86 blocks (EGA)
  FHTICT50.EXE 80x50 text, 80x100 blocks (VGA)
  FHTICMDA.EXE MDA 80x25 text, shaded blocks

  -snow fixes snow on IBM CGA cards, -cga fixes the text modes on CGA
  cards. The text modes draw the whole picture first, so add -potato
  for speed.

Mode Y draws the 3D view straight into video memory and flips between
3 pages, like vanilla Doom and FastDoom's FDOOM.EXE. Mode 13h draws into
system memory and copies the view to the card every frame, like
vanilla Heretic. Which is faster depends on the video card and bus, so
try both on your machine with -timedemo demo1.

  HTIC13F.EXE  plain Heretic v1.3 with only the -timedemo fix, for
               comparing speeds (needs DOS4GW.EXE from your Heretic)

The FHTIC files use the DOS/32A extender, so DOS4GW.EXE is not needed.
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

  -high -low -potato       detail level
  -flatSpan                flat floors, no distance lighting (fastest)
  -flatterSpan             flat floors, lit by distance
  -defSpan                 textured floors
  -flatsky / -defSky       flat or normal sky
  -flatInv                 solid ghosts
  -saturn                  checkerboard ghosts (Sega Saturn style)
  -translucent / -defInv   normal see-through ghosts
  -near / -far             only draw nearby items and decorations
  -fps / -nofps            frame rate counter
  -vsync / -novsync        wait for vertical retrace
  -size N                  screen size 3 to 11
  -csv                     add -timedemo results to FHBENCH.CSV
  -fixDAC                  wait for retrace before palette changes (old
                           VGA cards that show snow); vanilla always did

Settings are saved to HERETIC.CFG (the original HERETIC.EXE ignores them).

Changes
-------

v0.4  FastDoom's other video modes: EGA, CGA (4 colour, mono, 16 colour,
      ANSI from Hell, composite, 512 colour), Plantronics, Sigma Color
      400, Hercules, InColor, and text modes 40x25 to 80x50 and MDA.
      Palette changes no longer wait for vertical retrace (-fixDAC).
v0.3  Mode Y renderer straight into video memory with 3-page flipping
      (FHTIC.EXE); Mode 13h kept as FHTIC13H.EXE. FastDoom options
      -flatSpan, -flatterSpan, -saturn, -near, -vsync, -size, -csv.
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
