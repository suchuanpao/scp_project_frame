
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

const char *forWard = "前进";
const char *backOff = "后退";
const char *turnLeft = "左转";
const char *turnRight = "右转";
const char *stop = "暂停";
const char *track = "跟随";
const char *audioOut = "自我介绍";


// 将信号量sem_id设置为init_value
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

// 删除sem_id信号量
int delSem(int sem_id) {
     union semun sem_union;
     if (semctl(sem_id,0,IPC_RMID,sem_union)==-1){
         perror("Sem delete");
         exit(1);
     }
	 
     return 0;
 }
 
// 对sem_id执行p操作
int semP(int sem_id) 
{
     struct sembuf sem_buf;
     sem_buf.sem_num=0;//信号量编号
     sem_buf.sem_op=-1;//P操作
     sem_buf.sem_flg=SEM_UNDO;//系统退出前未释放信号量，系统自动释放
     if (semop(sem_id,&sem_buf,1)==-1) {
         perror("Sem P operation");
         exit(1);
     }
     return 0;
 }

// 对sem_id执行V操作
int semV(int sem_id) {
  	struct sembuf sem_buf;
    sem_buf.sem_num=0;
    sem_buf.sem_op=1;//V操作
    sem_buf.sem_flg=SEM_UNDO;
    if (semop(sem_id,&sem_buf,1)==-1) {
        perror("Sem V operation");
        exit(1);
    }
	
    return 0;
}


//后台守护进程创建
void initDaemon() 
{ 
	int pid; 
	int i; 
	
	if(pid=fork()) 
		exit(0);	//是父进程，结束父进程 
	else if(pid< 0) 
		exit(1);	//fork失败，退出 
		
	//第一子进程成为新的会话组长和进程组长 
	setsid();

	//并与控制终端分离 
	if(pid=fork()){ 
		exit(0);//是第一子进程，结束第一子进程 
	}
	else if(pid< 0){
		exit(1);//fork失败，退出 
	}
		
	//第二子进程不再是会话组长 
	//关闭打开的文件描述符 
	for(i=0;i<NOFILE;++i)
		close(i); 
	
	//改变工作目录到/tmp 
	if (chdir(WORKDIR)<0){
        exit(1);
   	} 
	
	//重设文件创建掩模 
	umask(0);
	
	return; 
} 


