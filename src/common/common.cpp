
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
#include <signal.h> 
#include <sys/param.h> 
#include <sys/ipc.h> 
#include <sys/sem.h> 
#include <sys/shm.h> 
#include <pthread.h>
#include <syslog.h> 

#include "common.hpp"


using namespace std;

const char *forWard = "ǰ��";
const char *backOff = "����";
const char *turnLeft = "��ת";
const char *turnRight = "��ת";
const char *stop = "��ͣ";
const char *track = "����";
const char *audioOut = "���ҽ���";


// ���ź���sem_id����Ϊinit_value
int initSem(int sem_id,int init_value) 
{
	union semun sem_union;
   	sem_union.val=init_value;
	
    if (semctl(sem_id,0,SETVAL,sem_union)==-1) {
       	perror("Sem init");
        exit(1);
    }
	
    return 0;
}

// ɾ��sem_id�ź���
int delSem(int sem_id) {
     union semun sem_union;
     if (semctl(sem_id,0,IPC_RMID,sem_union)==-1){
         perror("Sem delete");
         exit(1);
     }
	 
     return 0;
 }
 
// ��sem_idִ��p����
int semP(int sem_id) 
{
     struct sembuf sem_buf;
     sem_buf.sem_num=0;//�ź������
     sem_buf.sem_op=-1;//P����
     sem_buf.sem_flg=SEM_UNDO;//ϵͳ�˳�ǰδ�ͷ��ź�����ϵͳ�Զ��ͷ�
     if (semop(sem_id,&sem_buf,1)==-1) {
         perror("Sem P operation");
         exit(1);
     }
     return 0;
 }

// ��sem_idִ��V����
int semV(int sem_id) {
  	struct sembuf sem_buf;
    sem_buf.sem_num=0;
    sem_buf.sem_op=1;//V����
    sem_buf.sem_flg=SEM_UNDO;
    if (semop(sem_id,&sem_buf,1)==-1) {
        perror("Sem V operation");
        exit(1);
    }
	
    return 0;
}


//��̨�ػ����̴���
void initDaemon() 
{ 
	int pid; 
	int i; 
	
	if(pid=fork()) 
		exit(0);	//�Ǹ����̣����������� 
	else if(pid< 0) 
		exit(1);	//forkʧ�ܣ��˳� 
		
	//��һ�ӽ��̳�Ϊ�µĻỰ�鳤�ͽ����鳤 
	setsid();

	//��������ն˷��� 
	if(pid=fork()){ 
		exit(0);//�ǵ�һ�ӽ��̣�������һ�ӽ��� 
	}
	else if(pid< 0){
		exit(1);//forkʧ�ܣ��˳� 
	}
		
	//�ڶ��ӽ��̲����ǻỰ�鳤 
	//�رմ򿪵��ļ������� 
	for(i=0;i<NOFILE;++i)
		close(i); 
	
	//�ı乤��Ŀ¼��/tmp 
	if (chdir(WORKDIR)<0){
        exit(1);
   	} 
	
	//�����ļ�������ģ 
	umask(0);
	
	return; 
} 


