#ifndef __SOUND__
#define __SOUND__

#define SND_TICRATE             140             // tic rate for updating sound
#define SND_MAXSONGS    40              // max number of songs in game
#define SND_SAMPLERATE  11025   // sample rate of sound effects

typedef enum
{
  snd_none,
  snd_PC,
  snd_Adlib,
  snd_SB,
  snd_PAS,
  snd_GUS,
  snd_MPU,
  snd_MPU2,
  snd_MPU3,
  snd_AWE,
  snd_ENSONIQ,
  snd_DISNEY,
  snd_TANDY,
  snd_PC1BIT,
  snd_LPTDAC,
  snd_SBDirect,
  snd_PCPWM,
  snd_CMS,
  snd_OPL2LPT,
  snd_OPL3LPT,
  snd_CD,
  snd_WAV,
  snd_SBMIDI,
  snd_RS232MIDI,
  snd_LPTMIDI,
  snd_WSS,
  snd_IMFC,
  snd_ESFM,
  snd_GOLD,
  NUM_SCARDS
} cardenum_t;

#endif
