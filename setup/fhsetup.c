//
// FHSETUP.EXE : FastHeretic setup
//
// A small text mode program that sets FastHeretic's sound cards, ports
// and display options in HERETIC.CFG. Only the lines it knows about are
// changed; everything else in the file (keys, mouse, joystick, chat
// macros) is kept as it is, so the original SETUP.EXE can still be used
// for the controls.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <i86.h>

#define CFGNAME "HERETIC.CFG"
#define VERSION "0.7"

//--------------------------------------------------------------------------
// Text screen
//--------------------------------------------------------------------------

static unsigned short *vram = (unsigned short *)0xB8000;
static int mono;

#define C_BACK   0x17   // white on blue
#define C_TITLE  0x4F   // bright white on red
#define C_ITEM   0x1F
#define C_VALUE  0x1E   // yellow on blue
#define C_SEL    0x70   // black on grey
#define C_HINT   0x30   // black on cyan
#define C_BOX    0x5F

static void Put(int x, int y, int attr, const char *s)
{
	unsigned short *p = vram + y * 80 + x;

	while (*s && x++ < 80)
		*p++ = (unsigned char)*s++ | (attr << 8);
}

static void Fill(int x, int y, int w, int h, int attr, int ch)
{
	int i, j;

	for (j = y; j < y + h; j++)
		for (i = x; i < x + w; i++)
			vram[j * 80 + i] = ch | (attr << 8);
}

static void PutC(int y, int attr, const char *s)
{
	Put((80 - (int)strlen(s)) / 2, y, attr, s);
}

static void SetMode(int mode)
{
	union REGS r;

	r.w.ax = mode;
	int386(0x10, &r, &r);
}

static void Cursor(int on)
{
	union REGS r;

	r.h.ah = 1;
	r.w.cx = on ? 0x0607 : 0x2000;
	int386(0x10, &r, &r);
}

// For testing: -keys "DDRE..." plays keys (U D L R = arrows, E = Enter,
// S = F10, X = Esc) and SCREEN.TXT gets the screen before each one
static char *testkeys;

static void DumpScreen(void)
{
	FILE *f = fopen("SCREEN.TXT", "a");
	int x, y;

	if (!f)
		return;
	for (y = 0; y < 25; y++)
	{
		for (x = 0; x < 80; x++)
			fputc(vram[y * 80 + x] & 0xFF, f);
		fputc('\n', f);
	}
	fputs("--------\n", f);
	fclose(f);
}

static int GetKey(void)
{
	int c;

	if (testkeys)
	{
		DumpScreen();
		switch (*testkeys ? *testkeys++ : 'X')
		{
		case 'U': return 0x148;
		case 'D': return 0x150;
		case 'L': return 0x14B;
		case 'R': return 0x14D;
		case 'E': return 13;
		case 'S': return 0x144;
		case 'Y': return 'y';
		default: return 27;
		}
	}
	c = getch();

	if (c == 0 || c == 0xE0)
		return 0x100 | getch();
	return c;
}

#define K_UP    0x148
#define K_DOWN  0x150
#define K_LEFT  0x14B
#define K_RIGHT 0x14D
#define K_HOME  0x147
#define K_END   0x14F
#define K_F10   0x144
#define K_ESC   27
#define K_ENTER 13

//--------------------------------------------------------------------------
// HERETIC.CFG
//--------------------------------------------------------------------------

#define MAXLINES 200

static char *lines[MAXLINES];
static int numlines;

static void LoadConfig(void)
{
	FILE *f = fopen(CFGNAME, "r");
	char buf[256];

	if (!f)
		return;
	while (numlines < MAXLINES && fgets(buf, sizeof(buf), f))
	{
		buf[strcspn(buf, "\r\n")] = 0;
		lines[numlines++] = strdup(buf);
	}
	fclose(f);
}

static int FindKey(const char *key)
{
	int i, n = strlen(key);

	for (i = 0; i < numlines; i++)
		if (!strncmp(lines[i], key, n) && (lines[i][n] == ' '
			|| lines[i][n] == '\t'))
			return i;
	return -1;
}

static int GetInt(const char *key, int def)
{
	int i = FindKey(key);
	char *p;

	if (i < 0)
		return def;
	p = lines[i] + strlen(key);
	while (*p == ' ' || *p == '\t')
		p++;
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		return strtol(p + 2, NULL, 16);
	return atoi(p);
}

static void SetInt(const char *key, int v)
{
	char buf[80];
	int i = FindKey(key);

	sprintf(buf, "%s\t\t%d", key, v);
	if (i >= 0)
	{
		free(lines[i]);
		lines[i] = strdup(buf);
	}
	else if (numlines < MAXLINES)
		lines[numlines++] = strdup(buf);
}

static int SaveConfig(void)
{
	FILE *f = fopen(CFGNAME, "w");
	int i;

	if (!f)
		return 0;
	for (i = 0; i < numlines; i++)
		fprintf(f, "%s\n", lines[i]);
	return fclose(f) == 0;
}

//--------------------------------------------------------------------------
// The settings
//--------------------------------------------------------------------------

typedef struct
{
	int value;
	const char *name;
	const char *hint;
} choice_t;

// Card numbers as FastHeretic reads them (0-9 are Heretic's own)
static choice_t sfxcards[] = {
	{0, "None", "No sound effects."},
	{3, "Sound Blaster", "Any Sound Blaster or compatible. Uses the port, IRQ and DMA below."},
	{4, "Pro Audio Spectrum", "Media Vision Pro Audio Spectrum (needs MVSOUND.SYS)."},
	{5, "Gravis UltraSound", "Needs the ULTRASND variable (set by the GUS install)."},
	{9, "Sound Blaster AWE32", "Needs BLASTER with an E (EMU) setting."},
	{10, "Ensoniq Soundscape", "Needs the SNDSCAPE variable (set by the Soundscape install)."},
	{25, "Windows Sound System", "WSS / AD1848 / CS4231 codec cards."},
	{28, "Adlib Gold", "Adlib Gold 1000 sampled sound."},
	{2, "Adlib (OPL sampled)", "Plays the effects through the OPL2 chip. Rough, but works on a plain Adlib."},
	{11, "Disney Sound Source", "On the printer port below."},
	{14, "Covox / LPT DAC", "8-bit DAC on the printer port below."},
	{12, "Tandy 3 voice", "Tandy / PCjr sound chip."},
	{17, "Creative Music System", "Game Blaster / CMS at the Sound Blaster port."},
	{18, "OPL2LPT", "OPL2 on the printer port below."},
	{19, "OPL3LPT", "OPL3 on the printer port below."},
	{15, "Sound Blaster direct", "Sound Blaster without DMA or IRQ. Slower, for awkward setups."},
	{1, "PC speaker", "Plays the sampled effects through the PC speaker."},
	{16, "PC speaker (PWM)", "Better quality PC speaker. Needs a fast machine."},
};

static choice_t muscards[] = {
	{0, "None", "No music."},
	{3, "Sound Blaster (OPL)", "FM music on the Sound Blaster's OPL2/OPL3."},
	{2, "Adlib", "FM music on an Adlib (OPL2)."},
	{4, "Pro Audio Spectrum (OPL)", "FM music on the Pro Audio Spectrum."},
	{6, "General MIDI (MPU-401)", "Roland SC-55, MT-32, Yamaha, wavetable boards... Uses the MIDI port below."},
	{5, "Gravis UltraSound", "Needs the ULTRASND variable and the GUS patches."},
	{9, "Sound Blaster AWE32", "Needs SYNTHGS.SBK in the game folder and BLASTER with E."},
	{10, "Ensoniq Soundscape", "Needs the SNDSCAPE variable."},
	{22, "Sound Blaster MIDI port", "MIDI out of the Sound Blaster's game port."},
	{23, "Serial MIDI", "MIDI on a COM port. Set the MIDI port below to 3F8, 2F8, 3E8 or 2E8."},
	{24, "LPT MIDI", "MIDI on the printer port below."},
	{18, "OPL2LPT", "OPL2 on the printer port below."},
	{19, "OPL3LPT", "OPL3 on the printer port below."},
	{26, "IBM Music Feature", "IBM Music Feature Card."},
	{27, "ESS ESFM", "ESS AudioDrive native FM."},
	{17, "Creative Music System", "Game Blaster / CMS at the Sound Blaster port."},
	{20, "CD audio", "Plays CD tracks: track 2 is the first song. Needs MSCDEX."},
	{21, "WAV files", "MUSIC\\HERETIC\\MUS_n.RAW in the game folder (raw 8-bit mono)."},
};

static choice_t sbports[] = {
	{0x220, "220", 0}, {0x240, "240", 0}, {0x260, "260", 0}, {0x280, "280", 0},
	{0x210, "210", 0}, {0x230, "230", 0}, {0x250, "250", 0},
};
static choice_t sbirqs[] = {
	{-1, "From BLASTER", 0}, {2, "2", 0}, {3, "3", 0}, {5, "5", 0},
	{7, "7", 0}, {10, "10", 0},
};
static choice_t sbdmas[] = {
	{-1, "From BLASTER", 0}, {0, "0", 0}, {1, "1", 0}, {3, "3", 0},
};
static choice_t midiports[] = {
	{0x330, "330", 0}, {0x300, "300", 0}, {0x310, "310", 0}, {0x320, "320", 0},
	{0x332, "332", 0}, {0x334, "334", 0}, {0x336, "336", 0}, {0x340, "340", 0},
	{0x3F8, "3F8 (COM1)", 0}, {0x2F8, "2F8 (COM2)", 0}, {0x3E8, "3E8 (COM3)", 0},
	{0x2E8, "2E8 (COM4)", 0},
};
static choice_t lptports[] = {
	{0x378, "378 (LPT1)", 0}, {0x278, "278 (LPT2)", 0}, {0x3BC, "3BC (mono card)", 0},
};
static choice_t rates[] = {
	{0, "7000 Hz", 0}, {1, "8000 Hz", 0}, {2, "11025 Hz", 0}, {3, "12000 Hz", 0},
	{4, "16000 Hz", 0}, {5, "22050 Hz", 0}, {6, "24000 Hz", 0},
	{7, "32000 Hz", 0}, {8, "44100 Hz", 0},
};
static choice_t channels[] = {
	{1, "1", 0}, {2, "2", 0}, {3, "3", 0}, {4, "4", 0}, {5, "5", 0},
	{6, "6", 0}, {7, "7", 0}, {8, "8", 0},
};
static choice_t onoff[] = {{0, "Off", 0}, {1, "On", 0}};
static choice_t detail[] = {{0, "High", 0}, {1, "Low", 0}, {2, "Potato", 0}};

#define N(a) (sizeof(a) / sizeof(a[0]))

typedef struct
{
	const char *label;
	const char *key;
	int def;
	choice_t *choices;
	int numchoices;
	const char *hint;
	int value;
} item_t;

static item_t items[] = {
	{"Sound effects", "snd_sfxdevice", 0, sfxcards, N(sfxcards), 0},
	{"Music", "snd_musicdevice", 0, muscards, N(muscards), 0},
	{"Sound Blaster port", "snd_sbport", 0x220, sbports, N(sbports),
		"Also used for the Creative Music System."},
	{"Sound Blaster IRQ", "snd_sbirq", -1, sbirqs, N(sbirqs),
		"\"From BLASTER\" uses the BLASTER variable (IRQ 7 if there is none)."},
	{"Sound Blaster DMA", "snd_sbdma", -1, sbdmas, N(sbdmas),
		"8-bit DMA. The 16-bit one (H) is always taken from BLASTER."},
	{"MIDI port", "snd_mport", 0x330, midiports, N(midiports),
		"MPU-401 port, or the COM port for serial MIDI."},
	{"Printer port", "snd_lptport", 0x378, lptports, N(lptports),
		"For Disney, Covox, OPL2LPT, OPL3LPT and LPT MIDI."},
	{"Effects mixing rate", "snd_rate", 2, rates, N(rates),
		"Higher sounds better but costs speed. 11025 Hz is what the game's sounds are."},
	{"Effect channels", "snd_channels", 3, channels, N(channels),
		"How many effects can play at once. Fewer is faster."},
	{"Swap left and right", "snd_reversestereo", 0, onoff, N(onoff),
		"Turn on if sounds come from the wrong side (some SB Pro clones)."},
	{"Uncapped frame rate", "fh_uncapped", 0, onoff, N(onoff),
		"Draws frames between the game's 35 tics a second. Also in the Options menu."},
	{"Show frame rate", "fh_showfps", 0, onoff, N(onoff),
		"Frames per second in the corner of the screen."},
	{"Wait for retrace", "fh_vsync", 0, onoff, N(onoff),
		"Stops tearing, but caps the frame rate to the monitor."},
	{"Detail", "fh_detail", 0, detail, N(detail),
		"Low halves the columns, Potato quarters them. Also in the game's menu."},
};

#define NUMITEMS N(items)
#define SAVEITEM NUMITEMS
#define QUITITEM (NUMITEMS + 1)

static int Find(item_t *it, int v)
{
	int i;

	for (i = 0; i < it->numchoices; i++)
		if (it->choices[i].value == v)
			return i;
	return -1;
}

//--------------------------------------------------------------------------
// Drawing
//--------------------------------------------------------------------------

#define TOP 3

static void DrawScreen(int sel, int changed)
{
	int i, y;
	item_t *it;
	const char *hint = "";
	char buf[81];

	Fill(0, 0, 80, 25, C_BACK, ' ');
	Fill(0, 0, 80, 1, C_TITLE, ' ');
	PutC(0, C_TITLE, "FastHeretic Setup " VERSION " by LFTech");

	for (i = 0, it = items; i < NUMITEMS; i++, it++)
	{
		int c = Find(it, it->value);

		y = TOP + i + (i >= 2) + (i >= 10);
		if (i == 2)
			Put(6, TOP + 2, C_BACK, "Ports and options");
		if (i == 10)
			Put(6, TOP + 11, C_BACK, "Display");
		Put(8, y, i == sel ? C_SEL : C_ITEM, " ");
		sprintf(buf, "%-22s", it->label);
		Put(9, y, i == sel ? C_SEL : C_ITEM, buf);
		if (c >= 0)
			sprintf(buf, "%s", it->choices[c].name);
		else
			sprintf(buf, "%d (not in list)", it->value);
		Put(34, y, i == sel ? C_SEL : C_VALUE, buf);
		if (i == sel)
		{
			hint = it->hint;
			if (c >= 0 && it->choices[c].hint)
				hint = it->choices[c].hint;
		}
	}
	y = TOP + NUMITEMS + 3;
	Put(9, y, sel == SAVEITEM ? C_SEL : C_ITEM, " Save settings and quit ");
	Put(9, y + 1, sel == QUITITEM ? C_SEL : C_ITEM, " Quit without saving ");
	if (sel == SAVEITEM)
		hint = changed ? "Writes " CFGNAME ". Your keys and other settings are kept."
			: "Nothing changed yet.";
	if (sel == QUITITEM)
		hint = "Leaves " CFGNAME " as it was.";

	Fill(0, 23, 80, 2, C_HINT, ' ');
	Put(1, 23, C_HINT, hint ? hint : "");
	Put(1, 24, C_HINT, "Up/Down: choose   Left/Right or Enter: change   F10: save   Esc: quit");
}

// A pop-up list for the card choices
static int PickList(item_t *it)
{
	int n = it->numchoices, w = 0, i, x, y, h, cur, top = 0;
	int key;
	char buf[81];

	for (i = 0; i < n; i++)
		if ((int)strlen(it->choices[i].name) > w)
			w = strlen(it->choices[i].name);
	w += 4;
	h = n < 16 ? n : 16;
	x = 40 - w / 2;
	y = 12 - h / 2;
	cur = Find(it, it->value);
	if (cur < 0)
		cur = 0;
	for (;;)
	{
		if (cur < top)
			top = cur;
		if (cur >= top + h)
			top = cur - h + 1;
		Fill(x - 1, y - 1, w + 2, h + 2, C_BOX, ' ');
		Put(x, y - 1, C_BOX, it->label);
		for (i = 0; i < h; i++)
		{
			sprintf(buf, " %-*s ", w - 2, it->choices[top + i].name);
			Put(x, y + i, top + i == cur ? C_SEL : C_BOX, buf);
		}
		Fill(0, 23, 80, 1, C_HINT, ' ');
		Put(1, 23, C_HINT, it->choices[cur].hint ? it->choices[cur].hint : "");
		key = GetKey();
		if (key == K_UP)
			cur = (cur + n - 1) % n;
		else if (key == K_DOWN)
			cur = (cur + 1) % n;
		else if (key == K_HOME)
			cur = 0;
		else if (key == K_END)
			cur = n - 1;
		else if (key == K_ENTER)
			return it->choices[cur].value;
		else if (key == K_ESC)
			return it->value;
	}
}

static int Confirm(const char *q)
{
	int key;

	Fill(18, 10, 44, 4, C_BOX, ' ');
	PutC(11, C_BOX, q);
	PutC(12, C_BOX, "(Y/N)");
	for (;;)
	{
		key = GetKey();
		if (key == 'y' || key == 'Y')
			return 1;
		if (key == 'n' || key == 'N' || key == K_ESC)
			return 0;
	}
}

//--------------------------------------------------------------------------

int main(int argc, char **argv)
{
	int i, sel = 0, key, changed = 0, c, saved = 0;
	item_t *it;

	if (argc > 2 && !strcmp(argv[1], "-keys"))
		testkeys = argv[2];
	LoadConfig();
	for (i = 0, it = items; i < NUMITEMS; i++, it++)
	{
		it->value = GetInt(it->key, it->def);
		// the original setup writes 0 or -1 for "not set"
		if (it->choices == sbports && it->value <= 0)
			it->value = 0x220;
		if (it->choices == midiports && it->value <= 0)
			it->value = 0x330;
		if (it->choices == sfxcards && (it->value == 7 || it->value == 8))
			it->value = 6;
		if (it->choices == muscards && (it->value == 7 || it->value == 8))
			it->value = 6;
	}
	{
		union REGS r;
		r.h.ah = 0x0F;
		int386(0x10, &r, &r);
		mono = r.h.al == 7;
	}
	if (mono)
		vram = (unsigned short *)0xB0000;
	else
		SetMode(3);
	Cursor(0);

	for (;;)
	{
		DrawScreen(sel, changed);
		key = GetKey();
		if (key == K_UP)
			sel = (sel + QUITITEM) % (QUITITEM + 1);
		else if (key == K_DOWN)
			sel = (sel + 1) % (QUITITEM + 1);
		else if (key == K_HOME)
			sel = 0;
		else if (key == K_END)
			sel = QUITITEM;
		else if (key == K_F10 || (key == K_ENTER && sel == SAVEITEM))
		{
			saved = 1;
			break;
		}
		else if (key == K_ESC || (key == K_ENTER && sel == QUITITEM))
		{
			if (!changed || key == K_ENTER || Confirm("Quit without saving?"))
				break;
		}
		else if (sel < NUMITEMS && (key == K_LEFT || key == K_RIGHT
			|| key == K_ENTER || key == ' '))
		{
			it = &items[sel];
			if (key == K_ENTER && it->numchoices > 4)
				i = PickList(it);
			else
			{
				c = Find(it, it->value);
				if (c < 0)
					c = 0;
				else if (key == K_LEFT)
					c = (c + it->numchoices - 1) % it->numchoices;
				else
					c = (c + 1) % it->numchoices;
				i = it->choices[c].value;
			}
			if (i != it->value)
			{
				it->value = i;
				changed = 1;
			}
		}
	}

	Cursor(1);
	if (!mono)
		SetMode(3);
	if (saved)
	{
		for (i = 0, it = items; i < NUMITEMS; i++, it++)
			SetInt(it->key, it->value);
		if (!SaveConfig())
		{
			printf("Couldn't write " CFGNAME ".\n");
			return 1;
		}
		printf("Settings saved to " CFGNAME ".\n");
	}
	return 0;
}
