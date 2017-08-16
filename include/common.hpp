#ifndef COMMON_H
#define COMMON_H 1

#define SHMKEY  		181008 
#define ASRSEMKEY  		181994 

#define WORKDIR "/root/aiMultiProcess/asr"

#define MOTORSHMOFF (1*1024)
#define TTSSHMOFF (10*1024)
#define ASRSHMOFF (10*1024)
#define TRACKERSHMOFF (1*1024)
#define AUDIOCAPSHMOFF (320*1024)

#define MOTORSHMSTART (0)
#define TTSSHMSTART (MOTORSHMSTART+MOTORSHMOFF)
#define ASRSHMSTART (TTSSHMSTART+TTSSHMOFF)
#define TRACKERSHMSTART (ASRSHMSTART+ASRSHMOFF)
#define AUDIOCAPSHMSTART (TRACKERSHMSTART+TRACKERSHMOFF)
#define VIDEOCAPSHMSTART (AUDIOCAPSHMSTART+AUDIOCAPSHMOFF)

#define TTSTEXTOFF 16
#define ASRTEXTOFF 32


union semun {
 	int val;
  	struct semid_ds *buf;
   	unsigned short *array;
};

struct motorControl{
	int infoFlag;
	int command;
	int speedL;
	int speedR;
	int time;
	
	int reserved0;
	int reserved1;
	int reserved2;
};

struct ttsShm{
	int infoFlag;
	int textLen;
	int reserved1;
	int reserved2;
};

struct asrShm{
	int infoFlag;
	int command;
	int textLen;
	int asrPID;
	int mainControlPID;
	int reserved2;
};


typedef enum{
	GO_FORWARD = 0,
	BACK_OFF = 1,
	TURN_LEFT = 2,
	TURN_RIGHT = 3,
	STOP = 4,
	NOCOMMAND = 5,
}moveCommand;

typedef enum{
	SPEECH_NOCOM   = -1,
	SPEECH_FORWARD = 0,
	SPEECH_BACK    = 1,
	SPEECH_TURNL   = 2,
	SPEECH_TURNR   = 3,
	SPEECH_STOP    = 4,
	SPEECH_TRACK   = 5,
	SPEECH_OUT     = 6,
}speechCommand;


extern const char *forWard;
extern const char *backOff;
extern const char *turnLeft;
extern const char *turnRight;
extern const char *stop;
extern const char *track;
extern const char *audioOut;


extern int initSem(int sem_id,int init_value);
extern int delSem(int sem_id);
extern int semP(int sem_id);
extern int semV(int sem_id);
extern void initDaemon();



#endif



