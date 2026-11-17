FastQuake
=========

FastQuake is DOS Quake (id Software, 1996) tuned for 486 PCs, in the
spirit of FastDoom. It is built from id's GPL source release with a
modern DJGPP compiler. It is an LFTech project.

You need your own copy of Quake (shareware or registered). Copy the
FastQuake files into your QUAKE directory, next to the ID1 folder that
holds PAK0.PAK (and PAK1.PAK). No game data is included here.

Executables
-----------

  FQUAKE.EXE   built for 486 / 5x86 (use this one on a 486)
  FQUAKE5.EXE  built for Pentium (keeps id's Pentium span drawer)
  FQUAKE3.EXE  built for 386 + 387 (or 386-class CPUs with an FPU)
  CWSDPMI.EXE  DPMI host from id's release (needed unless you run under
               Windows or another DPMI host)

All the usual Quake options work (-nosound, -nocdaudio, -mem, ...).

FastQuake options
-----------------

Options > FastQuake Options in the menu, or type them in the console.
They are saved in CONFIG.CFG.

  Detail          fq_detail      0 High (normal)
                                 1 Low: the 3D view is drawn at half
                                   height (320x100 in 320x200) and each
                                   line is shown twice. Halves the span
                                   setup work, the biggest cost on a 486.
                                 2 Potato: half height and half width
                                 3 Low 160x200: half width only (helps
                                   much less than Low on a 486)
  Texture spans   fq_spans       0 id's drawer: perspective every 16
                                   pixels (tuned for Pentium)
                                 1 486 drawer, perspective every 32 pixels
                                 2 486 drawer, perspective every 64 pixels
  Texture detail  d_mipcap       0 Full .. 3 Eighth (smaller textures)
  Lighting        fq_light       0 Normal
                                 1 Flat blocks (one light level per
                                   16x16 lightmap block)
                                 2 Fullbright (no lightmaps at all)
  Show weapon     r_drawviewmodel
  Sky             fq_sky         0 Normal, 1 flat colour
  Water           fq_water       0 Warped, 1 flat colour
  Particles       fq_particles   0 Normal, 1 single pixels, 2 none
  Show FPS        fq_fps         0 Off, 1 On

Benchmark
---------

In the console:  timedemo demo1

BENCH.BAT runs five timedemos (stock settings, 486 spans, Low, Low + fast
options, Potato + fast options) and appends the results to FQBENCH.TXT:

  BENCH                 uses FQUAKE.EXE
  BENCH FQUAKE5         another exe
  BENCH FQUAKE -nosound extra options for every run

The settings for each run are in ID1\FQB1.CFG .. FQB5.CFG; your own
CONFIG.CFG is left alone. Any run of FastQuake with -fqbench quits after
the timedemo and appends its result to FQBENCH.TXT; -fqprof adds a
breakdown of where the frame time went.

Speed in 86Box (ASUS PCI/I-486SP3G, 16MB, PCI ET4000/W32p, FQUAKE.EXE,
timedemo demo1, viewsize 100, -nosound, shareware PAK0), frames per second:

                                  DX-33   DX2-66   5x86-133
  FQB1 original settings           4.7     9.3      18.2
  FQB2 486 spans                    -      9.5        -
  FQB3 Low detail (320x100)        6.5    12.8      25.0
  FQB4 Low + fullbright, flat
       sky/water, pixel particles,
       no weapon                   7.5    14.9      28.9
  FQB5 Potato + the same           8.3    16.3      31.6

86Box does not emulate the 486's cache misses, so a real machine may
differ; please send numbers from real hardware.

What was changed
----------------

  src/fq.c, fq.h       options, detail modes, benchmark and profiler
  src/fq_span.c        span drawer selection
  src/fq_draw.s        486 span drawers (made by tools/genspan.py)
  src/r_surf.c         flat-block and fullbright surface builders
  src/menu.c           FastQuake Options menu
  src/d_edge.c, r_main.c, r_part.c, r_sky.c, screen.c, cl_demo.c, host.c
                       hooks for the above

Building
--------

Needs a DJGPP cross compiler (tested with gcc 12.2 from
andrewwutw/build-djgpp). Set DJGPP to its folder and run ./build.sh;
the executables and release files land in dist/.

License
-------

GNU GPL version 2, see LICENSE.TXT. Quake source code by id Software.
CWSDPMI by Charles W. Sandmann.
