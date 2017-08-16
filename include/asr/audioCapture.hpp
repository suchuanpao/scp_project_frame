//============================================================================================
//audio data capture 
//2017-4-3
//============================================================================================

#ifndef _AUDIO_CAPTURE_H_
#define _AUDIO_CAPTURE_H_

extern snd_pcm_access_t micAccess;
extern snd_pcm_format_t micFormat;
extern unsigned int micSampleRate;
extern unsigned int micChannels;
extern int micPeriodSize ;

extern snd_pcm_t* micHwInit(snd_pcm_access_t access,snd_pcm_format_t format,\
					unsigned int sampleRate,unsigned int channels,\
					int *frameNum);
extern int readAudioData(snd_pcm_t *capture_handle,char*buf,snd_pcm_uframes_t frames,int size);

#endif





