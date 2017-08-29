//============================================================================================
//  ����ʶ��Automatic Speech Recognition�������ܹ���������ʶ����ض�������ʻ����ģʽ��
//2017-4-8
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
#include <sys/param.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <iostream>
#include <signal.h> 
#include <syslog.h> 
#include <string.h>

#include <alsa/asoundlib.h> 

#include "ccl/ccl.hpp"
#include "common.hpp"
#include "asr/audioCapture.hpp"

#include "asr/qisr.h"
#include "asr/msp_cmn.h"
#include "asr/msp_errors.h"

using namespace std;


#define	BUFFER_SIZE 2048
#define HINTS_SIZE  100
#define GRAMID_LEN	128
#define FRAME_LEN	640 

/* wav��Ƶͷ����ʽ */
typedef struct _wavePcmHdr
{
	char            riff[4];                // = "RIFF"
	int		size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int		fmt_size;		// = ��һ���ṹ��Ĵ�С : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = ͨ���� : 1
	int		samples_per_sec;        // = ������ : 8000 | 6000 | 11025 | 16000
	int		avg_bytes_per_sec;      // = ÿ���ֽ��� : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = ÿ�������ֽ��� : wBitsPerSample / 8
	short int       bits_per_sample;        // = ����������: 8 | 16

	char            data[4];                // = "data";
	int		data_size;              // = �����ݳ��� : FileSize - 44 
} wavePcmHdr;

/* Ĭ��wav��Ƶͷ������ */
wavePcmHdr defaultWavHdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0
};


 
int get_grammar_id(char* grammar_id, unsigned int id_len)
{
	FILE*			fp				=	NULL;
	char*			grammar			=	NULL;
	unsigned int	grammar_len		=	0;
	unsigned int	read_len		=	0;
	const char*		ret_id			=	NULL;
	unsigned int	ret_id_len		=	0;
	int				ret				=	-1;	

	if (NULL == grammar_id)
		goto grammar_exit;

	fp = fopen("/root/Ai/gm_continuous_digit.abnf", "rb");
	if (NULL == fp)
	{   
		printf("\nopen grammar file failed!\n");
		goto grammar_exit;
	}
	
	fseek(fp, 0, SEEK_END);
	grammar_len = ftell(fp); //��ȡ�﷨�ļ���С 
	fseek(fp, 0, SEEK_SET); 

	grammar = (char*)malloc(grammar_len + 1);
	if (NULL == grammar)
	{
		printf("\nout of memory!\n");
		goto grammar_exit;
	}

	read_len = fread((void *)grammar, 1, grammar_len, fp); //��ȡ�﷨����
	if (read_len != grammar_len)
	{
		printf("\nread grammar error!\n");
		goto grammar_exit;
	}
	grammar[grammar_len] = '\0';

	ret_id = MSPUploadData("usergram", grammar, grammar_len, "dtt = abnf, sub = asr", &ret); //�ϴ��﷨
	if (MSP_SUCCESS != ret)
	{
		printf("\nMSPUploadData failed, error code: %d.\n", ret);
		goto grammar_exit;
	}

	ret_id_len = strlen(ret_id);
	if (ret_id_len >= id_len)
	{
		printf("\nno enough buffer for grammar_id!\n");
		goto grammar_exit;
	}
	strncpy(grammar_id, ret_id, ret_id_len);
	printf("grammar_id: \"%s\" \n", grammar_id); //�´ο���ֱ��ʹ�ø�ID�������ظ��ϴ��﷨��

grammar_exit:
	if (NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}
	if (NULL!= grammar)
	{
		free(grammar);
		grammar = NULL;
	}
	return ret;
}

char rec_result[BUFFER_SIZE] = {'\0'};

void run_asr(char *buf,const char* params, char* grammar_id,unsigned int total_len)
{
	const char*		session_id				= NULL;
	//char			rec_result[BUFFER_SIZE]	= {'\0'};	
	char			hints[HINTS_SIZE]		= {'\0'}; //hintsΪ�������λỰ��ԭ�����������û��Զ���
	//unsigned int	total_len				= 0;
	int 			aud_stat 				= MSP_AUDIO_SAMPLE_CONTINUE;		//��Ƶ״̬
	int 			ep_stat 				= MSP_EP_LOOKING_FOR_SPEECH;		//�˵���
	int 			rec_stat 				= MSP_REC_STATUS_SUCCESS;			//ʶ��״̬	
	int 			errcode 				= MSP_SUCCESS;

	//FILE*			f_pcm 					= NULL;
	//char*			p_pcm 					= NULL;
	long 			pcm_count 				= 0;
	long 			pcm_size 				= total_len;
	//long			read_size				= 0;

	//printf("\n begin recognition.....\n");
	session_id = QISRSessionBegin(grammar_id, params, &errcode);
	//printf("\n session_id = %s !\n",session_id);
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRSessionBegin failed, error code:%d\n", errcode);
		goto asr_exit;
	}

	unsigned int len ; 
	int ret;

	//wav file header
	char tempBuf[256];
	len = sizeof(wavePcmHdr); 
	//printf("wav header len=%d\n",len);
	memcpy(tempBuf,&(defaultWavHdr.riff[0]),len);
	ret = QISRAudioWrite(session_id,(const void *)tempBuf,len,aud_stat,&ep_stat,&rec_stat);

	//printf("wav total len=%d\n",total_len);
	if (MSP_SUCCESS != ret)
	{
		printf("\nQISRAudioWrite header failed, error code:%d\n",ret);
		goto asr_exit;
	}
	
	while (1) 
	{		
		len = 10 * FRAME_LEN; // ÿ��д��200ms��Ƶ(16k��16bit)��1֡��Ƶ20ms��10֡=200ms��16k�����ʵ�16λ��Ƶ��һ֡�Ĵ�СΪ640Byte
		
		//printf("\n len=%d, pcm_size=%d,pcm_count=%d\n",len,pcm_size,pcm_count);

		if (pcm_size < 2 * len) 
			len = pcm_size;
		if (len <= 0)
			break;
		
		aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if (0 == pcm_count){
			aud_stat = MSP_AUDIO_SAMPLE_FIRST;
		}
		
		//printf(">===\n");
		ret = QISRAudioWrite(session_id,(const void *)&buf[pcm_count],len,aud_stat,&ep_stat,&rec_stat);
		//ret = QISRAudioWrite(session_id, (const void *)buf,size,aud_stat,&ep_stat,&rec_stat);

		if (MSP_SUCCESS != ret)
		{
			printf("\nQISRAudioWrite failed0, error code:%d\n",ret);
			goto asr_exit;
		}
			
		pcm_count += (long)len;
		pcm_size  -= (long)len;
		
		if (MSP_EP_AFTER_SPEECH == ep_stat)//add ||pcm_size==0 //bug
			break;
		//usleep(200*1000); //ģ����˵��ʱ���϶��10֡����Ƶ����Ϊ200ms
		usleep(1000);
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRAudioWrite failed1, error code:%d\n",errcode);
		goto asr_exit;	
	}

	unsigned int recLen;
	recLen=0;
	while (MSP_REC_STATUS_COMPLETE != rec_stat) 
	{
		const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			printf("\nQISRGetResult failed, error code: %d\n", errcode);
			goto asr_exit;
		}
		if (NULL != rslt)
		{
			unsigned int rslt_len = strlen(rslt);
			recLen += rslt_len;
			if (recLen >= BUFFER_SIZE)
			{
				printf("\nno enough buffer for rec_result !\n");
				goto asr_exit;
			}
			strncat(rec_result, rslt, rslt_len);
		}
		usleep(1*1000); //��ֹƵ��ռ��CPU
	}
	printf("\n����ʶ����� end!\n");
	//printf("=============================================================\n");
	//printf("rec in asr =%s\n",rec_result);
	//printf("=============================================================\n");

asr_exit:
	
	//if (NULL != f_pcm)
	//{
	//	fclose(f_pcm);
	//	f_pcm = NULL;
	//}
	//if (NULL != p_pcm)
	//{	
	//	free(p_pcm);
	//	p_pcm = NULL;
	//}

	QISRSessionEnd(session_id, hints);
}





void run_asr0(const char* audio_file, const char* params, char* grammar_id)
{
	const char*		session_id				= NULL;
	char			rec_result[BUFFER_SIZE]	= {'\0'};	
	char			hints[HINTS_SIZE]		= {'\0'}; //hintsΪ�������λỰ��ԭ�����������û��Զ���
	unsigned int	total_len				= 0;
	int 			aud_stat 				= MSP_AUDIO_SAMPLE_CONTINUE;		//��Ƶ״̬
	int 			ep_stat 				= MSP_EP_LOOKING_FOR_SPEECH;		//�˵���
	int 			rec_stat 				= MSP_REC_STATUS_SUCCESS;			//ʶ��״̬	
	int 			errcode 				= MSP_SUCCESS;

	FILE*			f_pcm 					= NULL;
	char*			p_pcm 					= NULL;
	long 			pcm_count 				= 0;
	long 			pcm_size 				= 0;
	long			read_size				= 0;

	if (NULL == audio_file)
		goto asr_exit;

	f_pcm = fopen(audio_file, "rb");
	
	if (NULL == f_pcm) 
	{
		printf("\nopen [%s] failed!\n", audio_file);
		goto asr_exit;
	}
	
	fseek(f_pcm, 0, SEEK_END);
	pcm_size = ftell(f_pcm); //��ȡ��Ƶ�ļ���С 
	fseek(f_pcm, 0, SEEK_SET);		

	p_pcm = (char*)malloc(pcm_size);
	if (NULL == p_pcm)
	{
		printf("\nout of memory!\n");
		goto asr_exit;
	}

	read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm); //��ȡ��Ƶ�ļ�����
	if (read_size != pcm_size)
	{
		printf("\nread [%s] failed!\n", audio_file);
		goto asr_exit;
	}
	
	printf("\n begin recognition.....\n");
	session_id = QISRSessionBegin(grammar_id, params, &errcode);
	printf("\n session_id = %s !\n",session_id);
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRSessionBegin failed, error code:%d\n", errcode);
		goto asr_exit;
	}

	while (1) 
	{
		unsigned int len = 10 * FRAME_LEN; // ÿ��д��200ms��Ƶ(16k��16bit)��1֡��Ƶ20ms��10֡=200ms��16k�����ʵ�16λ��Ƶ��һ֡�Ĵ�СΪ640Byte
		int ret = 0;

		printf("\n len=%d, pcm_size=%d,pcm_count=%d\n",len,pcm_size,pcm_count);

		if (pcm_size < 2 * len) 
			len = pcm_size;
		if (len <= 0)
			break;
		
		aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
		if (0 == pcm_count)
			aud_stat = MSP_AUDIO_SAMPLE_FIRST;
		
		printf(">===\n");
		ret = QISRAudioWrite(session_id, (const void *)&p_pcm[pcm_count], len, aud_stat, &ep_stat, &rec_stat);
		if (MSP_SUCCESS != ret)
		{
			printf("\nQISRAudioWrite failed0, error code:%d\n",ret);
			goto asr_exit;
		}
			
		pcm_count += (long)len;
		pcm_size  -= (long)len;
		
		if (MSP_EP_AFTER_SPEECH == ep_stat)//add ||pcm_size==0 //bug
			break;
		usleep(200*1000); //ģ����˵��ʱ���϶��10֡����Ƶ����Ϊ200ms
	}
	errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
	if (MSP_SUCCESS != errcode)
	{
		printf("\nQISRAudioWrite failed1, error code:%d\n",errcode);
		goto asr_exit;	
	}

	while (MSP_REC_STATUS_COMPLETE != rec_stat) 
	{
		const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
		if (MSP_SUCCESS != errcode)
		{
			printf("\nQISRGetResult failed, error code: %d\n", errcode);
			goto asr_exit;
		}
		if (NULL != rslt)
		{
			unsigned int rslt_len = strlen(rslt);
			total_len += rslt_len;
			if (total_len >= BUFFER_SIZE)
			{
				printf("\nno enough buffer for rec_result !\n");
				goto asr_exit;
			}
			strncat(rec_result, rslt, rslt_len);
		}
		usleep(150*1000); //��ֹƵ��ռ��CPU
	}
	printf("\n����ʶ����� end!\n");
	printf("=============================================================\n");
	printf("%s",rec_result);
	printf("=============================================================\n");

asr_exit:
	if (NULL != f_pcm)
	{
		fclose(f_pcm);
		f_pcm = NULL;
	}
	if (NULL != p_pcm)
	{	
		free(p_pcm);
		p_pcm = NULL;
	}

	QISRSessionEnd(session_id, hints);
}


//void *asrMain(void *ptr)
int main(int argc,char **argv)
{		
	//������̨�ػ�����
	//initDaemon();
	
	const char*		session_id				= NULL;	
	char			hints[HINTS_SIZE]		= {'\0'}; //hintsΪ�������λỰ��ԭ�����������û��Զ���
	int 			aud_stat = MSP_AUDIO_SAMPLE_FIRST;		//��Ƶ״̬
	int 			ep_stat = MSP_EP_LOOKING_FOR_SPEECH;		//�˵���
	int 			rec_stat = MSP_REC_STATUS_SUCCESS;			//ʶ��״̬	
	int 			errcode = MSP_SUCCESS;
	long 			pcm_count 	= 0;
	long 			pcm_size 	= 0;
	bool            sessionBegin=0;//must be 0
	unsigned int 	len ; 

	char*buf;
	unsigned int size;
	int bytes = 0;
	
	struct timeval lastTime, thisTime;
    int interval;

	
	//for speech recognition
	int 		ret 			=	MSP_SUCCESS;
	//const char* login_params		=	"appid = 588808f6, work_dir = ."; //��¼����,appid��msc���,��������Ķ�
	const char* login_params	=	"appid = 570ee863, work_dir = ."; //��¼����,appid��msc���,��������Ķ�
	/*
	* sub:             ����ҵ������
	* result_type:     ʶ������ʽ
	* result_encoding: ��������ʽ
	*
	* ��ϸ����˵������ġ�Ѷ��������MSC--API�ĵ���//GB2312
	*/
	const char*	session_begin_params	=	"sub = asr, domain=asr,aue=speex-wb;7,vad_bos = 500,vad_eos=500,result_type = plain, result_encoding = GB2312";
	char*		grammar_id				=	NULL;
	
		
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

	/* �û���¼ */
	ret = MSPLogin(NULL, NULL, login_params); //��һ���������û������ڶ������������룬����NULL���ɣ������������ǵ�¼����
	if (MSP_SUCCESS != ret)
	{
		printf("asr MSPLogin failed, error code: %d.\n",ret);
		goto exit; //��¼ʧ�ܣ��˳���¼
	}

	printf("\n##################################################\n");
	printf("## ����ʶ��Automatic Speech Recognition������ ##\n");
	printf("## �ܹ���������ʶ����ض�������ʻ����ģʽ��   ##\n");
	printf("##################################################\n\n");

	grammar_id = (char*)malloc(GRAMID_LEN);
	if (NULL == grammar_id)
	{
		printf("out of memory !\n");
		goto exit;
	}
	memset(grammar_id, 0, GRAMID_LEN);

	printf("�ϴ��﷨ ...\n");
	ret = get_grammar_id(grammar_id, GRAMID_LEN);
	if (MSP_SUCCESS != ret)
		goto exit;
	printf("�ϴ��﷨�ɹ�\n");

	
	//���̼�ͨ��
	int shmid;	
	int semidAsr;	
	
	shmid=shmget(SHMKEY,0,0); 
	while(shmid == -1){
		usleep(40*1000);	 
		shmid=shmget(SHMKEY,0,0);
	}
	
	printf("shm shmkey= %d,shmid=%d in asr\n",SHMKEY,shmid);
	if(shmid==-1)	
		printf("creat shmidMotor is fail\n");
	
	semidAsr=semget(ASRSEMKEY,1,0666|IPC_CREAT);
	printf("semkeyAsr= %d,semidAsr=%d\n",ASRSEMKEY,semidAsr);
	if(semidAsr==-1)	 
		printf("creat semidMotor is fail\n");
	//initSem(semidAsr,1);
	
	char *pShm;
	pShm = (char*)shmat(shmid,0,0);
	if(pShm == (char*)-1)	
		printf("shm shmat is fail in main\n");
	
	//for asr
	struct asrShm *pShmAsr; 
	pShmAsr = (struct asrShm*)(pShm + ASRSHMSTART);
	char *pShmAsrText;
	pShmAsrText = pShm+ASRSHMSTART+ASRTEXTOFF;
	
	while(1) 
	{	
		ret = readAudioData(capture_handle,buf,frameNum,size);
		
		if(ret==frameNum){
			
			if(sessionBegin == 0 ){
				session_id				= NULL;	
				rec_result[BUFFER_SIZE]	= {'\0'};
				hints[HINTS_SIZE]		= {'\0'}; //hintsΪ�������λỰ��ԭ�����������û��Զ���

				//aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
				aud_stat = MSP_AUDIO_SAMPLE_FIRST;		//��Ƶ״̬
				ep_stat  = MSP_EP_LOOKING_FOR_SPEECH;	//�˵���
				rec_stat = MSP_REC_STATUS_SUCCESS;		//ʶ��״̬	
				errcode  = MSP_SUCCESS;

				//printf("\n begin recognition.....\n");
				session_id = QISRSessionBegin(grammar_id, session_begin_params, &errcode);
				//printf("\n session_id = %s !\n",session_id);
				if (MSP_SUCCESS != errcode)
				{
					printf("\nQISRSessionBegin failed, error code:%d\n", errcode);
					//goto asr_exit;
					QISRSessionEnd(session_id, hints);
					continue;
				}

				//wav file header
				char tempBuf[256];
				len = sizeof(wavePcmHdr); 

				memcpy(tempBuf,&(defaultWavHdr.riff[0]),len);
				ret = QISRAudioWrite(session_id,(const void *)tempBuf,len,aud_stat,&ep_stat,&rec_stat);

				//printf("ret=%d,aud_stat=%d,ep_stat=%d,rec_stat=%d,\n",ret,aud_stat,ep_stat,rec_stat);
				if (MSP_SUCCESS != ret)
				{
					printf("\nQISRAudioWrite header failed, error code:%d\n",ret);
					//goto asr_exit;
					QISRSessionEnd(session_id, hints);
					continue;
				}

				sessionBegin=1;
				
			}


			pcm_count = 0;
			pcm_size  = size;
				
			while (1) 
			{	
				len = 10 * FRAME_LEN; // ÿ��д��200ms��Ƶ(16k��16bit)��1֡��Ƶ20ms��10֡=200ms��16k�����ʵ�16λ��Ƶ��һ֡�Ĵ�СΪ640Byte
				
				if (pcm_size < 2 * len) 
					len = pcm_size;
				if (len <= 0)
					break;
				
				aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
					
				ret = QISRAudioWrite(session_id,(const void *)&buf[pcm_count],len,aud_stat,&ep_stat,&rec_stat);

				//printf("ret=%d,aud_stat=%d,ep_stat=%d,rec_stat=%d,\n",ret,aud_stat,ep_stat,rec_stat);
				
				if (MSP_SUCCESS != ret)
				{
					printf("\nQISRAudioWrite failed0, error code:%d\n",ret);
					//goto asr_exit;
					sessionBegin=0;
					QISRSessionEnd(session_id, hints);
					break;
				}
						
				pcm_count += (long)len;
				pcm_size  -= (long)len;
					
				if (MSP_EP_AFTER_SPEECH == ep_stat)//add ||pcm_size==0 //bug
				{	
					break;
				}
				
				//2017-4-11
				if((ep_stat==MSP_EP_TIMEOUT)||(ep_stat==MSP_EP_ERROR)\
					||(ep_stat==MSP_EP_MAX_SPEECH)/*||(ep_stat==MSP_EP_IDLE)*/)
				{
					//goto asr_exit;
					sessionBegin=0;
					QISRSessionEnd(session_id, hints);
					break;
				}
				//usleep(200*1000); //ģ����˵��ʱ���϶��10֡����Ƶ����Ϊ200ms
				usleep(10*1000);
				
			}

			if (MSP_EP_AFTER_SPEECH == ep_stat)
			{	
					
				errcode = QISRAudioWrite(session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &ep_stat, &rec_stat);
				if (MSP_SUCCESS != errcode)
				{
					printf("\nQISRAudioWrite failed1, error code:%d\n",errcode);
					goto asr_exit;
				}

				unsigned int recLen;
				recLen=0;
				while (MSP_REC_STATUS_COMPLETE != rec_stat) 
				{
					const char *rslt = QISRGetResult(session_id, &rec_stat, 0, &errcode);
					if (MSP_SUCCESS != errcode)
					{
						printf("\nQISRGetResult failed, error code: %d\n", errcode);
						goto asr_exit;
					}
					
					if (NULL != rslt)
					{
						unsigned int rslt_len = strlen(rslt);
						recLen += rslt_len;
						if (recLen >= BUFFER_SIZE)
						{
							printf("\nno enough buffer for rec_result !\n");
							goto asr_exit;
						}
						strncat(rec_result, rslt, rslt_len);
					}
					usleep(5*1000); //��ֹƵ��ռ��CPU
				}

				printf("�����=%s \n",rec_result);
				char *temp;
				temp = rec_result;
				while((temp[0]!='i')||(temp[1]!='n')||(temp[2]!='p')||\
						(temp[3]!='u')||(temp[4]!='t')){
					temp++;
				}

				//=====================================
				temp += 6;
				speechCommand speechCom;
				if((forWard[0]==temp[0])&&(forWard[1]==temp[1])\
					&&(forWard[2]==temp[2])&&(forWard[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 0;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
					
				}else if((backOff[0]==temp[0])&&(backOff[1]==temp[1])\
					&&(backOff[2]==temp[2])&&(backOff[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 1;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
				}else if((turnLeft[0]==temp[0])&&(turnLeft[1]==temp[1])\
					&&(turnLeft[2]==temp[2])&&(turnLeft[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 2;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
					
				}else if((turnRight[0]==temp[0])&&(turnRight[1]==temp[1])\
					&&(turnRight[2]==temp[2])&&(turnRight[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 3;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
				}else if((stop[0]==temp[0])&&(stop[1]==temp[1])\
					&&(stop[2]==temp[2])&&(stop[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 4;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
				}else if((track[0]==temp[0])&&(track[1]==temp[1])\
					&&(track[2]==temp[2])&&(track[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 5;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
				}else if((audioOut[0]==temp[0])&&(audioOut[1]==temp[1])\
					&&(audioOut[2]==temp[2])&&(audioOut[3]==temp[3])){

					semP(semidAsr);
					pShmAsr->command = 6;
					pShmAsr->infoFlag = 1;
					semV(semidAsr);
					
					
				}else{
				
					semP(semidAsr);
					pShmAsr->command = -1;
					pShmAsr->infoFlag = 0;
					semV(semidAsr);
					
				}
				
				printf("�����=%c%c%c%c \n",temp[0],temp[1],temp[2],temp[3]);
				
				memset(rec_result,0,BUFFER_SIZE-1);

asr_exit:
				sessionBegin=0;
				QISRSessionEnd(session_id, hints);
				
			}

		}
	}
	
exit:
	if (NULL != grammar_id)
	{
		free(grammar_id);
		grammar_id = NULL;
	}
	printf("��������˳� ...\n");
	getchar();
	MSPLogout(); //�˳���¼

	//for micphone device
	snd_pcm_close (capture_handle); 
	free(buf);

	//return 0;
}


int main00(void) 
{
    pthread_t asrMainId;
	pthread_t speechCaptureId;
	
	int ret;
	
    //ret = pthread_create(&asrMainId, NULL, asrMain,NULL);
    //if(ret) {
    //    printf("Create pthread asrMain error!\n");
    //   return 1;
    //}

	//ret = pthread_create(&speechCaptureId, NULL, speechCaptureMain,NULL);
   // if(ret) {
    //    printf("Create pthread ttsMain error!\n");
     //   return 1;
    //}

    //pthread_join(speechCaptureId, NULL);
    //pthread_join(asrMainId, NULL);
	
    return 0;
		
}










