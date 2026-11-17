# FastQuake DOS build (DJGPP cross compiler)
DJGPP ?= /tmp/claude-0/djgpp
CC = $(DJGPP)/bin/i586-pc-msdosdjgpp-gcc
STUBEDIT = $(DJGPP)/i586-pc-msdosdjgpp/bin/stubedit
B ?= build/stock
EXE ?= QUAKE.EXE
CPU ?= -march=i486 -mtune=i486
DEFS ?=
CFLAGS = $(CPU) -O2 -fno-strict-aliasing -ffast-math -fcommon -fno-pie -w $(DEFS) -Isrc
ASFLAGS = $(DEFS) -Isrc -x assembler-with-cpp

COBJS = chase cl_demo cl_input cl_main cl_parse cl_tent cmd common console crc cvar \
 draw d_edge d_fill d_init d_modech d_part d_polyse d_scan d_sky d_sprite d_surf \
 d_vars d_zpoint host host_cmd keys menu mathlib model net_dgrm net_loop net_main \
 net_vcr net_ser net_dos net_ipx net_mp net_bw mplib mplpc pr_cmds pr_edict \
 pr_exec r_aclip r_alias r_bsp r_light r_draw r_efrag r_edge r_misc r_main r_sky \
 r_sprite r_surf r_part r_vars screen sbar sv_main sv_phys sv_move sv_user zone view \
 wad world cd_audio dos_v2 vid_dos vid_vga vid_ext vregset snd_dos snd_gus snd_dma \
 snd_mem snd_mix in_dos sys_dos fq fq_span $(EXTRA_C)
SOBJS = d_copy d_draw d_draw16 d_parta d_polysa d_scana d_spr8 d_varsa math \
 r_aliasa r_drawa r_edgea r_varsa surf16 surf8 worlda r_aclipa snd_mixa sys_dosa \
 dosasm fq_spana $(EXTRA_S)

OBJS = $(addprefix $(B)/,$(addsuffix .o,$(COBJS) $(SOBJS)))

$(B)/$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) -lm
	$(STUBEDIT) $@ minstack=1048576 || true

$(B)/%.o: src/%.c | $(B)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/%.o: src/%.s | $(B)
	$(CC) $(ASFLAGS) -c -o $@ $<
$(B):
	mkdir -p $@
clean:
	rm -rf build
