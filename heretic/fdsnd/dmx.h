// dmx.h : the DMX calls FastHeretic's sound layer (fhdmx.c) provides
#ifndef _DMX_H_
#define _DMX_H_
#include "tsmapi.h"
extern int dmx_mus_port;
extern int dmx_snd_port;
void MUS_PauseSong(int handle);
void MUS_ResumeSong(int handle);
void MUS_SetMasterVolume(int volume);
int MUS_RegisterSong(void *data);
int MUS_UnregisterSong(int handle);
int MUS_QrySongPlaying(int handle);
int MUS_StopSong(int handle);
int MUS_ChainSong(int handle, int next);
int MUS_PlaySong(int handle, int volume);
int SFX_PlayPatch(void *vdata, int pitch, int sep, int vol, int flags, int priority);
void SFX_StopPatch(int handle);
int SFX_Playing(int handle);
void SFX_SetOrigin(int handle, int pitch, int sep, int vol);
void GF1_SetMap(void *data, int len);
int FHS_Init(int mdev, int sdev, int musport, int sndport, void *genmidi);
void FHS_DeInit(void);
void FHS_Update(void);
void FHS_SetSongNum(int num);
#endif
