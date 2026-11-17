//
// fhdmx.c : FastHeretic sound, the DMX calls Heretic makes, played by
// FastDoom's sound library (ns_*.c, from the Apogee Sound System).
//
// Based on FastDoom's dmx.c (Alexey Khokholov, Victor Nieto) and APODMX.
// Adds back DMX's pitch shifting, which Heretic uses and FastDoom dropped.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "fdscompat.h"
#include "dmx.h"
#include "ns_fxm.h"
#include "ns_music.h"
#include "ns_task.h"
#include "ns_multi.h"
#include "ns_muldf.h"
#include "ns_pcfx.h"
#include "ns_scape.h"
#include "ns_sb.h"
#include "ns_sbmus.h"
#include "ns_cd.h"
#include "ns_cards.h"
#include "mus2mid.h"
#include "i_sound.h"

// settings the sound library reads (set from HERETIC.CFG / command line)
int ignoreSoundChecks;
int numChannels = 8;
int reverseStereo;
char *sbkfile = "SYNTHGS.SBK";
int snd_Rate = 2;               // 11025 Hz mixing
int snd_PCMRate = 0;            // WAV music: 11025 Hz
int snd_GoldIrq = 5;
int snd_GoldDma = 1;
int snd_MidiDevice;

int dmx_mus_port = 0;
int dmx_snd_port = 0;

static fx_blaster_config dmx_blaster;
static int fhs_mdev = snd_none;     // FastDoom card numbers
static int fhs_sdev = snd_none;
static int fhs_fxok;

static void *mus_data;
static char *mid_data;
static int mus_loop;
static int songnum = -1;            // for CD and WAV music

//--------------------------------------------------------------------------
// Errors and the bits of FastDoom's system code the library calls
//--------------------------------------------------------------------------

#undef I_Error
void I_Error(char *error, ...);
byte *I_AllocLow(int length);

void FDS_Error(int code, ...)
{
	switch (code)
	{
	case 7: case 8:
		I_Error("No CD-ROM drive with audio tracks found (MSCDEX).");
	case 12: case 13:
		I_Error("Sound Blaster not found. Check the BLASTER variable.");
	case 14:
		I_Error("Music card not responding.");
	case 15: case 16:
		I_Error("Sound effects card not responding.");
	case 29: case 30:
		I_Error("Can't load the AWE32 sound bank %s.", sbkfile);
	default:
		I_Error("Sound error %d.", code);
	}
}

char *I_LoadTextProgram(int number)
{
	return "";
}

unsigned char *I_ReadBinaryStatic(char *file, int size)
{
	return NULL;
}

void *I_DosMemAlloc(unsigned long size)
{
	return I_AllocLow(size);
}

//--------------------------------------------------------------------------
// Music
//--------------------------------------------------------------------------

void FHS_SetSongNum(int num)
{
	songnum = num;
}

static void MusicCD(void)
{
	int track;

	if (songnum < 0 || !CD_Cdrom_data.High_audio)
		return;
	track = songnum + 2;            // track 1 is the data track
	if (track > CD_Cdrom_data.High_audio)
		track = track % CD_Cdrom_data.High_audio + 1;
	CD_StopAudio();
	CD_PlayAudio(TrackBeginPosition[track], TrackLength[track]);
}

static unsigned char *wavptr;
static int wavhandle = -1;
static int wavvolume = 127;

static void MusicWAV(void)
{
	char name[64];
	FILE *f;
	long size;
	static const unsigned rates[3] = {11025, 22050, 44100};

	if (wavhandle >= 0 && MV_VoicePlaying(wavhandle))
		MV_Kill(wavhandle);
	wavhandle = -1;
	if (wavptr)
		Z_Free(wavptr);
	wavptr = NULL;
	if (songnum < 0)
		return;
	sprintf(name, "MUSIC\\HERETIC\\MUS_%d.RAW", songnum);
	f = fopen(name, "rb");
	if (!f)
		return;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	wavptr = Z_Malloc(size, PU_STATIC, NULL);
	fread(wavptr, 1, size, f);
	fclose(f);
	wavhandle = MV_PlayRaw(wavptr, size, rates[snd_PCMRate % 3],
		wavvolume * 2, wavvolume * 2, wavvolume * 2, 0);
}

// Called every tic: restart a looping CD or WAV song that ended
void FHS_Update(void)
{
	if (!mus_loop)
		return;
	if (fhs_mdev == snd_CD)
	{
		CD_GetAudioStatus();
		if (!(CD_Cdrom_data.Status & (1 << 9)))
			MusicCD();
	}
	else if (fhs_mdev == snd_WAV && wavptr && !MV_VoicePlaying(wavhandle))
		MusicWAV();
}

void MUS_PauseSong(int handle)
{
	if (fhs_mdev == snd_CD)
		CD_StopAudio();
	else if (fhs_mdev == snd_WAV)
		MV_Kill(wavhandle);
	else
		MUSIC_Pause();
}

void MUS_ResumeSong(int handle)
{
	if (fhs_mdev == snd_CD)
		CD_ResumeAudio();
	else if (fhs_mdev == snd_WAV)
		MusicWAV();
	else
		MUSIC_Continue();
}

void MUS_SetMasterVolume(int volume)
{
	if (volume > 127)
		volume = 127;
	if (fhs_mdev == snd_CD)
		CD_SetVolume(volume * 2);
	else if (fhs_mdev == snd_WAV)
		wavvolume = volume;
	else
		MUSIC_SetVolume(volume * 2);
}

int MUS_RegisterSong(void *data)
{
	FILE *mus, *mid;
	unsigned int midlen;
	unsigned short len;

	mus_data = NULL;
	if (fhs_mdev == snd_CD || fhs_mdev == snd_WAV || fhs_mdev == snd_none)
		return 0;
	len = ((unsigned short *)data)[2] + ((unsigned short *)data)[3];
	if (mid_data)
		Z_Free(mid_data);
	mid_data = NULL;
	if (!memcmp(data, "MThd", 4))
	{
		mus_data = data;
		return 0;
	}
	// MUS -> MIDI through temporary files, like DMX replacements do
	mus = fopen("temp.mus", "wb");
	if (!mus)
		return 0;
	fwrite(data, 1, len, mus);
	fclose(mus);
	mus = fopen("temp.mus", "rb");
	mid = fopen("temp.mid", "wb");
	if (!mus || !mid || mus2mid(mus, mid))
	{
		if (mus) fclose(mus);
		if (mid) fclose(mid);
		return 0;
	}
	fclose(mid);
	fclose(mus);
	mid = fopen("temp.mid", "rb");
	if (!mid)
		return 0;
	fseek(mid, 0, SEEK_END);
	midlen = ftell(mid);
	rewind(mid);
	mid_data = Z_Malloc(midlen, PU_STATIC, NULL);
	fread(mid_data, 1, midlen, mid);
	fclose(mid);
	mus_data = mid_data;
	remove("temp.mid");
	remove("temp.mus");
	return 0;
}

int MUS_UnregisterSong(int handle)
{
	return 0;
}

int MUS_QrySongPlaying(int handle)
{
	if (fhs_mdev == snd_WAV)
		return wavhandle >= 0 && MV_VoicePlaying(wavhandle);
	return MUSIC_SongPlaying();
}

int MUS_StopSong(int handle)
{
	if (fhs_mdev == snd_CD)
		CD_StopAudio();
	else if (fhs_mdev == snd_WAV)
	{
		if (wavhandle >= 0)
			MV_Kill(wavhandle);
		wavhandle = -1;
	}
	else
		MUSIC_StopSong();
	return 0;
}

int MUS_ChainSong(int handle, int next)
{
	mus_loop = (next == handle);
	return 0;
}

int MUS_PlaySong(int handle, int volume)
{
	if (fhs_mdev == snd_CD)
	{
		MusicCD();
		return 0;
	}
	if (fhs_mdev == snd_WAV)
	{
		MusicWAV();
		return 0;
	}
	if (mus_data == NULL)
		return 0;
	MUSIC_PlaySong((unsigned char *)mus_data, mus_loop);
	MUSIC_SetVolume(volume * 2);
	return 0;
}

//--------------------------------------------------------------------------
// Sound effects
//--------------------------------------------------------------------------

// 2^(i/64) in 16.16: DMX pitch 128 is normal, +64 is an octave up
static const unsigned pitchtab[64] = {
	65536, 66250, 66971, 67700, 68438, 69183, 69936, 70698, 71468, 72246,
	73032, 73828, 74632, 75444, 76266, 77096, 77936, 78785, 79642, 80510,
	81386, 82273, 83169, 84074, 84990, 85915, 86851, 87796, 88752, 89719,
	90696, 91684, 92682, 93691, 94711, 95743, 96785, 97839, 98905, 99982,
	101070, 102171, 103283, 104408, 105545, 106694, 107856, 109031, 110218,
	111418, 112631, 113858, 115098, 116351, 117618, 118899, 120194, 121502,
	122825, 124163, 125515, 126882, 128263, 129660};

static unsigned PitchRate(unsigned rate, int pitch)
{
	int p = pitch - 128;
	int oct = p >> 6;              // floor
	unsigned r = (rate * pitchtab[p & 63]) >> 16;

	if (oct > 0)
		r <<= oct;
	else if (oct < 0)
		r >>= -oct;
	return r;
}

static int Div63i(int x)
{
	return x / 63;
}

int SFX_PlayPatch(void *vdata, int pitch, int sep, int vol, int flags,
	int priority)
{
	unsigned rate;
	unsigned long len;
	unsigned char *data = (unsigned char *)vdata;

	if (!fhs_fxok || (data[0] | (data[1] << 8)) != 3)
		return -1;
	rate = (data[3] << 8) | data[2];
	len = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];
	if (len <= 48)
		return -1;
	len -= 32;
	if (pitch != 128)
		rate = PitchRate(rate, pitch);
	return MV_PlayRaw(data + 24, len, rate, vol * 2,
		Div63i((254 - sep) * vol), Div63i(sep * vol), 127 - priority);
}

void SFX_StopPatch(int handle)
{
	if (fhs_fxok)
		MV_Kill(handle);
}

int SFX_Playing(int handle)
{
	return fhs_fxok && MV_VoicePlaying(handle);
}

void SFX_SetOrigin(int handle, int pitch, int sep, int vol)
{
	VoiceNode *voice;

	if (!fhs_fxok)
		return;
	voice = MV_GetVoice(handle);
	if (voice == NULL)
		return;
	MV_SetVoiceVolume(voice, vol * 2, Div63i((254 - sep) * vol),
		Div63i(sep * vol));
}

//--------------------------------------------------------------------------
// Setup
//--------------------------------------------------------------------------

// GENMIDI lump -> Apogee OPL timbre bank (from FastDoom's dmx.c)
static void SetTimbres(unsigned char *cdata)
{
	unsigned char *tmb;
	int i, j, o;

	tmb = Z_Malloc(13 * 256, PU_STATIC, NULL);
	memset(tmb, 0, 13 * 256);
	for (i = 0; i < 175; i++)
	{
		j = i < 128 ? i : i + 35;
		o = 8 + i * 36 + 4;
		tmb[j * 13 + 0] = cdata[o + 0];
		tmb[j * 13 + 1] = cdata[o + 7];
		tmb[j * 13 + 2] = cdata[o + 4] | cdata[o + 5];
		tmb[j * 13 + 3] = cdata[o + 11] & 192;
		tmb[j * 13 + 4] = cdata[o + 1];
		tmb[j * 13 + 5] = cdata[o + 8];
		tmb[j * 13 + 6] = cdata[o + 2];
		tmb[j * 13 + 7] = cdata[o + 9];
		tmb[j * 13 + 8] = cdata[o + 3];
		tmb[j * 13 + 9] = cdata[o + 10];
		tmb[j * 13 + 10] = cdata[o + 6];
		tmb[j * 13 + 11] = (i < 128 ? 0 : cdata[8 + i * 36 + 3])
			+ cdata[o + 14] + 12;
		tmb[j * 13 + 12] = 0;
	}
	AL_RegisterTimbreBank(tmb);
	Z_Free(tmb);
}

static int CardCode(int dev)
{
	switch (dev)
	{
	case snd_PC: return PC;
	case snd_Adlib: return Adlib;
	case snd_SB: return SoundBlaster;
	case snd_PAS: return ProAudioSpectrum;
	case snd_GUS: return UltraSound;
	case snd_MPU: return GenMidi;
	case snd_AWE: return Awe32;
	case snd_ENSONIQ: return SoundScape;
	case snd_DISNEY: return SoundSource;
	case snd_TANDY: return Tandy3Voice;
	case snd_PC1BIT: return PC1bit;
	case snd_PCPWM: return PCPWM;
	case snd_CMS: return CMS;
	case snd_LPTDAC: return LPTDAC;
	case snd_SBDirect: return SoundBlasterDirect;
	case snd_OPL2LPT: return OPL2LPT;
	case snd_OPL3LPT: return OPL3LPT;
	case snd_CD: return AudioCD;
	case snd_WAV: return FileWAV;
	case snd_SBMIDI: return SBMIDI;
	case snd_RS232MIDI: return RS232MIDI;
	case snd_LPTMIDI: return LPTMIDI;
	case snd_WSS: return WSS;
	case snd_IMFC: return IMFC;
	case snd_ESFM: return ESFM;
	case snd_GOLD: return GOLD;
	default: return -1;
	}
}

static int UsesBlaster(int dev)
{
	return dev == snd_SB || dev == snd_AWE || dev == snd_SBDirect
		|| dev == snd_SBMIDI;
}

// mdev, sdev: FastDoom card numbers (fd i_sound.h). genmidi: the GENMIDI
// lump for OPL music. Returns 0, or prints why a card failed.
int FHS_Init(int mdev, int sdev, int musport, int sndport, void *genmidi)
{
	static const unsigned rates[9] = {7000, 8000, 11025, 12000, 16000,
		22050, 24000, 32000, 44100};
	int status, voices;
	fx_device fxdev;

	fhs_mdev = mdev;
	fhs_sdev = sdev;
	dmx_mus_port = musport;
	dmx_snd_port = sndport;
	if (UsesBlaster(mdev) || UsesBlaster(sdev))
	{
		if (FX_GetBlasterSettings(&dmx_blaster) != FX_Ok)
			FDS_Error(12);
	}

	if (mdev != snd_none)
	{
		if (mdev == snd_CD)
		{
			if (CD_Init())
				FDS_Error(7);
		}
		else if (mdev != snd_WAV)
		{
			if (mdev == snd_SBMIDI
				&& FX_SetupSoundBlaster(dmx_blaster) != FX_Ok)
				FDS_Error(13);
			status = MUSIC_Init(CardCode(mdev), musport);
			if (status != MUSIC_Ok)
			{
				printf("Music card didn't start (%d), no music.\n", status);
				fhs_mdev = snd_none;
			}
			else
			{
				MUSIC_SetVolume(0);
				if (genmidi)
					SetTimbres(genmidi);
			}
		}
	}

	if (sdev != snd_none)
	{
		switch (CardCode(sdev))
		{
		case SoundBlaster:
		case Awe32:
		case SoundBlasterDirect:
			status = FX_SetupSoundBlaster(dmx_blaster);
			break;
		case SoundSource:
		case LPTDAC:
		case CMS:
		case Adlib:
		case OPL2LPT:
		case OPL3LPT:
			status = FX_SetupCard(CardCode(sdev), &fxdev, sndport);
			break;
		case GOLD:
			status = FX_SetupGold(snd_GoldIrq, snd_GoldDma);
			if (status == FX_Ok)
				status = FX_SetupCard(GOLD, &fxdev, -1);
			break;
		default:
			status = FX_SetupCard(CardCode(sdev), &fxdev, -1);
			break;
		}
		if (status == FX_Ok)
		{
			voices = numChannels + (mdev == snd_WAV);
			status = FX_Init(CardCode(sdev), voices, 2,
				rates[snd_Rate < 0 || snd_Rate > 8 ? 2 : snd_Rate]);
		}
		if (status != FX_Ok)
		{
			printf("Sound effects card didn't start (%d), no sound "
				"effects.\n", status);
			fhs_sdev = snd_none;
		}
		else
		{
			fhs_fxok = 1;
			FX_SetVolume(255);
			if (reverseStereo)
				MV_ReverseStereo();
		}
	}
	return 0;
}

void FHS_DeInit(void)
{
	if (fhs_mdev == snd_CD)
		CD_StopAudio();
	MUSIC_Shutdown();
	FX_Shutdown();
	remove("ULTRAMID.INI");
	if (mid_data)
		Z_Free(mid_data);
	mid_data = NULL;
}

// GUS: the patch map goes to ULTRAMID.INI for the GUS MIDI driver
void GF1_SetMap(void *data, int len)
{
	FILE *ini = fopen("ULTRAMID.INI", "wb");
	if (ini)
	{
		fwrite(data, 1, len, ini);
		fclose(ini);
	}
}
