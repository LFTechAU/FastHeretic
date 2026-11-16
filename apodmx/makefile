# Apogee Sound System backed DMX wrapper makefile

# --------------------------------------------------------------------------
#
#      4r  use 80486 timings and register argument passing
#       c  compile only
#      d1  include line number debugging information
#      d2  include full sybolic debugging information
#      ei  force enums to be of type int
#       j  change char default from unsigned to signed
#      oa  relax aliasing checking
#      od  do not optimize
#  oe[=#]  expand functions inline, # = quads (default 20)
#      oi  use the inline library functions
#      om  generate inline 80x87 code for math functions
#      ot  optimize for time
#      ox  maximum optimization
#       s  remove stack overflow checks
#     zp1  align structures on bytes
#      zq  use quiet mode
#  /i=dir  add include directories
#
# --------------------------------------------------------------------------

!ifdef OLDAPI
apodmx_variant = OLDAPI
!else
apodmx_variant = NEWAPI
!endif

CCOPTS = /d2 /omaxet /zp1 /4r /ei /j /zq /dDMX_$(apodmx_variant)

GLOBOBJS = &
 dmx.obj &
 dpmiapi.obj &
 usrhooks.obj &
 mus2mid.obj &
 tsmapi.obj

$(apodmx_variant)\apodmx.lib : $(apodmx_variant) $(GLOBOBJS)
 cd $(apodmx_variant)
 wlib apodmx.lib /n /b $(GLOBOBJS)
# wlib $^@ /n /b $<
 cd..

.obj : $(apodmx_variant)

tsmapi.obj : tsmapi.c
 wcc386 $(CCOPTS) /zu $[* /fo=$(apodmx_variant)\$^&
 
.c.obj :
 wcc386 $(CCOPTS) $[* /fo=$(apodmx_variant)\$^&

clean : .SYMBOLIC
 del $(apodmx_variant)\*.obj
 del $(apodmx_variant)\*.lib
