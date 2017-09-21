//============================================================================================
//audio data capture
//2017-4-3
//============================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <iostream>

#include <alsa/asoundlib.h>

//#include "../../AiMain/include/AIMain.hpp"
#include <alsa/pcm.h>
#include <alsa/mixer.h>
#include "asr/audioCapture.hpp"

using namespace std;

snd_pcm_access_t micAccess = SND_PCM_ACCESS_RW_INTERLEAVED ;
snd_pcm_format_t micFormat = SND_PCM_FORMAT_S16_LE;
unsigned int micSampleRate = 16000;
unsigned int micChannels = 1;
int micPeriodSize = 800;
long int g_leftVol=90,g_rightVol=90;
long int g_minVol=0, g_maxVol=100;

int volumeControl(long leftVol,long rightVol,long minVol,long maxVol)
{
	snd_mixer_t *mixerFd;
	snd_mixer_elem_t *elem;
	int result;

	if((result = snd_mixer_open( &mixerFd, 0)) < 0)
	{
		printf("snd_mixer_open error\n");
		mixerFd = NULL;
	}

	// Attach an HCTL to an opened mixer
	if((result = snd_mixer_attach( mixerFd, "default"))< 0)
	{
		printf("snd_mixer_attach error\n");
		snd_mixer_close(mixerFd);
		mixerFd = NULL;
	}

	// 注册混音器
	if ((result = snd_mixer_selem_register( mixerFd, NULL, NULL)) < 0)
	{
		printf("snd_mixer_selem_register error\n");
		snd_mixer_close(mixerFd);
		mixerFd = NULL;
	}

	// 加载混音器
	if ((result = snd_mixer_load( mixerFd)) < 0)
	{
		printf("snd_mixer_load error\n");
		snd_mixer_close(mixerFd);
		mixerFd = NULL;
	}

	if(leftVol>maxVol)
		leftVol = maxVol;
	if(leftVol<minVol)
		leftVol = minVol;

	if(rightVol>maxVol)
		rightVol = maxVol;
	if(rightVol<minVol)
		rightVol = minVol;

	//Ok, 到现在准备工作已经完成. 下面找到具体的混音器元素,就可以控制音量了, Alsa编程的确很简单.
	// 遍历混音器元素
	for(elem=snd_mixer_first_elem(mixerFd); elem; elem=snd_mixer_elem_next(elem))
	{
		if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE &&
				  snd_mixer_selem_is_active(elem)) // 找到可以用的, 激活的elem
		{
			printf(" name= %s\n",snd_mixer_selem_get_name(elem));
			//if(QString(snd_mixer_selem_get_name(elem)) == "Master")
			//if(!(strcmp("Master",snd_mixer_selem_get_name(elem))))
			//if(!(strcmp("Headphone",snd_mixer_selem_get_name(elem))))
			//{
			//	snd_mixer_selem_get_playback_volume_range(elem, &minVol, &maxVol);
			//	snd_mixer_selem_set_playback_volume_all(elem, leftVol); // 设置音量为50
			//}


			if(!(strcmp("AIF1 DAC timeslot 0 volume",snd_mixer_selem_get_name(elem))))
			{
				snd_mixer_selem_get_playback_volume_range(elem, &minVol, &maxVol);
				snd_mixer_selem_set_playback_volume_all(elem, leftVol); // 设置音量为50
			}
		}
	}

}

snd_pcm_t* micHwInit(snd_pcm_access_t access,snd_pcm_format_t format,\
					unsigned int sampleRate,unsigned int channels,\
					int *frameNum)
{
	int err;
	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_uframes_t frames;
	int dir=0;
	int bytes = -1;
	unsigned int val, val2;

    if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    	fprintf (stderr, "cannot open audio device %s (%s)\n",
             "default",snd_strerror (err));
       	exit (1);
  	}

   	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
     	fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
                 snd_strerror (err));
      	exit (1);
 	}

    if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
 		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                 snd_strerror (err));
  		exit (1);
 	}

	if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, access/*SND_PCM_ACCESS_RW_INTERLEAVED*/)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
                 snd_strerror (err));
 		exit (1);
	}

	//SND_PCM_FORMAT_U8 SND_PCM_FORMAT_S16_LE  SND_PCM_FORMAT_S24_LE
	if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params,format /*SND_PCM_FORMAT_S16_LE*/)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
                 snd_strerror (err));
 		exit (1);
	}

   	if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params,&sampleRate, 0)) < 0) {
    	fprintf (stderr, "cannot set sample rate (%s)\n",
                 snd_strerror (err));
    	exit (1);
   	}

   	if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, channels)) < 0) {
    	fprintf (stderr, "cannot set channel count (%s)\n",
                 snd_strerror (err));
  		exit (1);
   	}

	frames = (snd_pcm_uframes_t)frameNum[0];
	err = snd_pcm_hw_params_set_period_size_near(capture_handle, hw_params, &frames, &dir);
    if (err){
        fprintf(stderr, "Error setting period size: %s\n", snd_strerror(err));
        snd_pcm_close(capture_handle);
        //return err;
        exit (1);
    }

 	if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
     	fprintf (stderr, "cannot set parameters (%s)\n",
                 snd_strerror (err));
    	exit (1);
   	}

	//test mic hw info=============================================================
		printf("PCM handle name = '%s'\n",snd_pcm_name(capture_handle));
		printf("PCM state = %s\n",snd_pcm_state_name(snd_pcm_state(capture_handle)));

		snd_pcm_hw_params_get_access(hw_params,(snd_pcm_access_t *) &val);

		printf("access type = %s\n",snd_pcm_access_name((snd_pcm_access_t)val));

		snd_pcm_hw_params_get_format(hw_params,  (snd_pcm_format_t*)&val);
		printf("format = '%s' (%s)\n",snd_pcm_format_name((snd_pcm_format_t)val),
		snd_pcm_format_description((snd_pcm_format_t)val));

		snd_pcm_hw_params_get_subformat(hw_params,(snd_pcm_subformat_t *)&val);
		printf("subformat = '%s' (%s)\n",
				snd_pcm_subformat_name((snd_pcm_subformat_t)val),

		snd_pcm_subformat_description((snd_pcm_subformat_t)val));

		snd_pcm_hw_params_get_channels(hw_params, &val);
		printf("channels = %d\n", val);

		snd_pcm_hw_params_get_rate(hw_params, &val, &dir);
		printf("rate = %d bps\n", val);

		snd_pcm_hw_params_get_period_time(hw_params,&val, &dir);
		printf("period time = %d us\n", val);

		snd_pcm_hw_params_get_period_size(hw_params,&frames, &dir);
		printf("period size = %d frames\n", (int)frames);

		snd_pcm_hw_params_get_buffer_time(hw_params,&val, &dir);
		printf("buffer time = %d us\n", val);

		snd_pcm_hw_params_get_buffer_size(hw_params,(snd_pcm_uframes_t *) &val);
		printf("buffer size = %d frames\n", val);

		snd_pcm_hw_params_get_periods(hw_params, &val, &dir);
		printf("periods per buffer = %d frames\n", val);

		snd_pcm_hw_params_get_rate_numden(hw_params,&val, &val2);
		printf("exact rate = %d/%d bps\n", val, val2);

		val = snd_pcm_hw_params_get_sbits(hw_params);
		printf("significant bits = %d\n", val);

		snd_pcm_hw_params_get_tick_time(hw_params,&val, &dir);
		printf("tick time = %d us\n", val);

		val = snd_pcm_hw_params_is_batch(hw_params);
		printf("is batch = %d\n", val);

		val = snd_pcm_hw_params_is_block_transfer(hw_params);
		printf("is block transfer = %d\n", val);

		val = snd_pcm_hw_params_is_double(hw_params);
		printf("is double = %d\n", val);

		val = snd_pcm_hw_params_is_half_duplex(hw_params);
		printf("is half duplex = %d\n", val);

		val = snd_pcm_hw_params_is_joint_duplex(hw_params);
		printf("is joint duplex = %d\n", val);

		val = snd_pcm_hw_params_can_overrange(hw_params);
		printf("can overrange = %d\n", val);

		val = snd_pcm_hw_params_can_mmap_sample_resolution(hw_params);
		printf("can mmap = %d\n", val);

		val = snd_pcm_hw_params_can_pause(hw_params);
		printf("can pause = %d\n", val);

		val = snd_pcm_hw_params_can_resume(hw_params);
		printf("can resume = %d\n", val);

		val = snd_pcm_hw_params_can_sync_start(hw_params);
		printf("can sync start = %d\n", val);

	//=============================================================================

	err=snd_pcm_hw_params_get_period_size(hw_params,&frames, &dir); /*获取周期长度*/
	if(err<0)
	{
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}

	frameNum[0] = (int)frames;
	printf("frames=%d=========== \n",frames);

	snd_pcm_hw_params_free (hw_params);

    //snd_pcm_drop 和 snd_pcm_prepare 来暂停和继续了。
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
   		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                 snd_strerror (err));
    	exit (1);
 	}

	//volumeControl(g_leftVol,g_rightVol,g_minVol,g_maxVol);


	return capture_handle;

}


int readAudioData(snd_pcm_t *capture_handle,char *buf,snd_pcm_uframes_t frames,int size)
{
	int i;
	int err;

	memset(buf,0,size);
	err = snd_pcm_readi(capture_handle,buf,frames);

	if (err == -EPIPE) {
		/* EPIPE means overrun */
		fprintf(stderr, "overrun occurred\n");
		printf("overrun occurred\n");
		snd_pcm_prepare(capture_handle);
		return 0;
	} else if (err < 0) {
		fprintf(stderr, "error from read: %s/n",snd_strerror(err));
		printf("read return err<0/n");
		return -1;
	} else if (err != (int)frames) {
		fprintf(stderr, "short read, read %d frames/n", err);
		printf("read data<frames/n");
		return -2;
	}

	return frames;

}


void *speechCaptureMain(void *ptr)
{

	const char*		session_id				= NULL;
	//char			hints[HINTS_SIZE]		= {'\0'}; //hints为结束本次会话的原因描述，由用户自定义
	//int 			aud_stat = MSP_AUDIO_SAMPLE_FIRST;		//音频状态
	//int 			ep_stat = MSP_EP_LOOKING_FOR_SPEECH;		//端点检测
	//int 			rec_stat = MSP_REC_STATUS_SUCCESS;			//识别状态
	//int 			errcode = MSP_SUCCESS;
	long 			pcm_count 	= 0;
	long 			pcm_size 	= 0;
	bool            sessionBegin=0;
	unsigned int 	len ;
	int             ret ;

	char*buf;
	unsigned int size;
	int bytes = 0;

	struct timeval lastTime, thisTime;
    int interval;

	//init microphone=========================================================
	int frameNum;
	snd_pcm_t *capture_handle;

	frameNum = micPeriodSize;
	capture_handle = micHwInit(micAccess,micFormat,micSampleRate,micChannels,&frameNum);

	printf("frameNum = %d \n",frameNum);

	if(micFormat==SND_PCM_FORMAT_U8)
		bytes = 1;
	else if(micFormat==SND_PCM_FORMAT_S16_LE)
		bytes = 2;
	else if(micFormat==SND_PCM_FORMAT_S24_LE)
		bytes = 3;
	else{
		printf("format error in micHwInit \n");
		exit (1);
	}

	size=frameNum*(micChannels*bytes);
	buf = (char*)malloc(size);

	while (1)
	{
		ret = readAudioData(capture_handle,buf,frameNum,size);

	}

exit:

	snd_pcm_close (capture_handle);

}






