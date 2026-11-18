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
  FHT13H4.EXE  Mode 13h, built for 486 / 5x86
  FHTICH.EXE   Mode Y at half height (320x100, each line shown twice
               as tall): about a quarter faster again
  FHTICX.EXE   Mode X 320x240, square pixels, drawn straight into video
               memory with 3 pages (a taller view than 320x200)
  FHTICVBD.EXE VESA 320x200 linear frame buffer, drawn straight into
               video memory with 3 pages (VESA 2.0 cards, or UNIVBE)
  FHTICVBR.EXE VESA 320x200, copied from system memory each frame
               (-noLFB to use the 64 KB window instead of linear)

High resolution VESA (the 3D view at full resolution; menus and the
status bar are scaled up). D = straight into video memory, 3 pages (needs
a linear frame buffer); R = drawn in memory and copied (any VESA card):

  FHM240D/R    320x240      FHM600D/R    800x600
  FHM300D/R    400x300      FHM768D/R    1024x768
  FHM384D/R    512x384      FHM800D/R    1280x800
  FHM400D/R    640x400      FHM1024D/R   1280x1024
  FHM480D/R    640x480      FHM1200D/R   1600x1200

  Most cards from the 386/486 era only offer some of these sizes; a VESA
  driver such as UNIVBE adds the rest.

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
  FHTICCDA.EXE 80x25 colour text at the MDA address (colour MDA cards)
  FHTICVT.EXE  80x24 shaded ASCII, also sent to a VT100 terminal on a
               serial port: -com 1..4 (default 1), -baud N (default 38400)

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
Run FHSETUP.EXE to pick the sound cards (and a few display options),
and the original SETUP.EXE for the controls. Each one keeps the
other's settings in HERETIC.CFG.

Benchmark:  FHTIC -timedemo demo1

Benchmark mode runs a demo once for each line of a .BNC file (the same
files FastDoom uses) and shows all the results together:

  FHTIC -bench QUICK.BNC [demo] [-advanced]

  QUICK.BNC   high, low and potato detail
  NORMAL.BNC  each detail with textured, flat and flat lit floors
  SIZES.BNC   each detail at screen sizes 10 and 11
  CPU.BNC     the same settings with each CPU's drawers

-advanced also shows the 1% and 0.1% low frame rates. The table is saved
to FHBENCH.TXT and every run is added to FHBENCH.CSV with the program's
name, so BENCH.BAT (which runs a .BNC file with every FHTIC and FHM
program in the folder) gives one CSV comparing all the video modes.

Sound
-----

Sound and music use FastDoom's sound library, so every card it knows
works here. FHSETUP.EXE lists them with what each one needs:

  Effects  Sound Blaster (and direct), Pro Audio Spectrum, Gravis
           UltraSound, AWE32, Ensoniq Soundscape, Windows Sound System,
           Adlib Gold, Adlib (OPL), Disney Sound Source, Covox, Tandy,
           CMS, OPL2LPT, OPL3LPT, PC speaker (normal or PWM)
  Music    OPL (Adlib, Sound Blaster, PAS), General MIDI (MPU-401, SB
           MIDI, serial, LPT), GUS, AWE32 (needs SYNTHGS.SBK), Soundscape,
           IBM Music Feature, ESFM, CMS, OPL2LPT, OPL3LPT, CD audio
           (track 2 is the first song), WAV files (MUSIC\HERETIC\MUS_n.RAW)

The Sound Blaster port, IRQ and DMA from HERETIC.CFG override the
BLASTER variable; without BLASTER the card is used as a plain 8-bit one.
Heretic's own card numbers mean what they did, so old HERETIC.CFG files
still work.

  -sfxdev N / -musdev N    card number for this run (see HERETIC.CFG)
  -nosound -nosfx -nomusic
  -reverseStereo           swap left and right

Speed options
-------------

Options > FASTHERETIC... in the game menu, or on the command line:

  F5 / DETAIL     HIGH, LOW (2-pixel columns) or POTATO (4-pixel columns)
  FLOORS          TEXTURED or FLAT (one colour per floor, still lit)
  SKY             NORMAL or FLAT (one colour)
  GHOSTS          SEE-THRU or SOLID (faster translucent sprites)
  FPS             frame rate counter, top right
  UNCAPPED        ON draws as many frames as the PC can manage, with
                  smooth movement between the game's 35 tics a second
  CPU             AUTO, or the CPU to tune the drawers for

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
  -uncapped / -capped      frame rate above 35 (single player)
  -386sx -386dx -i486 -cy386 -cy486 -umc486 -cy5x86 -cy6x86 -cy6x86mx
  -k5 -k6 -pentium -pentiumP54CS -pentiumMMX -pentiumII -winchip -mp6
                           tune the drawers for this CPU (as FastDoom)
  -vsync / -novsync        wait for vertical retrace
  -size N                  screen size 3 to 11
  -csv                     add -timedemo results to FHBENCH.CSV
  -fixDAC                  wait for retrace before palette changes (old
                           VGA cards that show snow); vanilla always did
  -debugCard2 / -debugCard4  show the frame rate on a POST code card
  -debugPort hex           POST card port (default 80)
  -limitram KB             use at most this much memory for the game
  -freeram KB              leave this much memory free (for TSRs)
  -xt                      keyboard fix for PC/XT style keyboard ports
  -noLFB                   VESA builds: use the 64 KB window, not linear

The CPU is found at startup and shown on the startup screen: CPUID
where there is one, otherwise the DPMI host's CPU type, with a check
for Cyrix chips. Whether a tuned drawer is faster depends on the whole
machine, so compare with -timedemo demo1 and the CPU options.

Settings are saved to HERETIC.CFG (the original HERETIC.EXE ignores them).

Changes
-------

v0.7.3 Sound effects at shifted pitches (nearly all of them) were mixed
      with an unset rate: sounds stalled, repeated, clicked or came in
      late, and the game could freeze or crash with a divide by zero.
      Fixed.

v0.7.2 Music played at a fraction of the menu volume at the start of
      every song; fixed, and the volume follows the original game's
      curve. Closing the FASTHERETIC menu left its last lines over the
      status bar; fixed. New Text startup option (FHSETUP, or
      -textstartup) shows Doom-style startup lines instead of Raven's
      loading screen.

v0.7.1 Fixed the Mode 13h and VESA (VBR) programs at low and potato
      detail with a view smaller than full screen: only the left half or
      quarter of the view reached the screen. Added FHT13H4.EXE (Mode
      13h for 486).

v0.7  FastDoom's sound library with all its cards (above), and the
      FHSETUP.EXE setup program. Drawers tuned per CPU like FastDoom
      (LEA, loop, Pentium and MMX versions; 2-pixel Mode Y spans for
      386-class chips), with the CPU found at startup or picked in the
      menu. Benchmark mode: -bench with FastDoom's .BNC files, -advanced
      for 1% and 0.1% lows, BENCH.BAT for every program at once.

v0.6  Uncapped frame rate with smooth movement (-uncapped, or the menu).
      Mode X 320x240 (FHTICX). High resolution VESA builds from 320x240
      to 1600x1200 (FHM...D/R). Colour MDA (FHTICCDA) and VT100 serial
      terminal (FHTICVT) text modes.
v0.5  VESA 320x200 builds: FHTICVBD (direct, 3 pages) and FHTICVBR
      (copied). Both fall back to Mode 13h when there is no VESA mode.
      FastDoom options -debugCard2/-debugCard4, -limitram, -freeram, -xt.

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
  FastDoom sound library: Victor Nieto (viti95) and contributors, GPL
  FastDoom (ideas and code being ported): Victor Nieto (viti95), GPL
  DOS/32A extender: Narech K.

Licensed under the GNU GPL v2 (see heretic/license.txt).
