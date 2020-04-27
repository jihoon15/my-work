#include <stdio.h>//
#include <stdlib.h>//
#include <sys/types.h>//
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>//
#include <sys/time.h>//
#include <signal.h>//
#include <string.h>//
#include "msg.h"

void enqueue();
void dequeue();
int msgq;
int ret;
#define key 0x03215

struct msgbuf msg;

int main(){
	msgq=msgget(key, IPC_CREAT | 0666);
	//int형으로 메시지큐생성하면서 아이디를 변수에 저장하는듯
	//0666말고 바꿔봤는데 오류남 찾아보니 자세히는 알수없고 key다음이 msgflag인데자세한사용법알기어렵고 불필요
	memset(&msg, 0, sizeof(msg));

	

          int pid = fork();
            if(pid < 0){
                //error
              perror("fork");
            }
            if(pid == 0){
               //child
                enqueue();
		printf("!!!!!!MESSAGE!!!!!!!!!!!!\n");
                exit(0);
            }
                else{
                        //parent
                        dequeue();
                //      pcbs[i].completed = 0;//**
                }

         return 0;

}



void enqueue()
{
        //printf("msgq id: %d\n", msgq);
//___________________________________
        msg.mtype = 0;
        msg.pid = getpid();
//넣어줄데이터 설정함
        ret = msgsnd(msgq, &msg, sizeof(msg), NULL);
//메시지 보냄(msgq id, size, msgflag)//flag 0: 큐에공간생기는거 긷자림//큐에공간없을시 복귀하는것도 있음
//이것도 마지막이
        printf("msgsnd ret: %d\n", ret);
	//성공여부 확인하는듯

        return;
}

void dequeue()
{
        //printf("msgq id: %d\n", msgq);

        ret = msgrcv(msgq, &msg, sizeof(msg), 0, NULL);
        printf("!!msgsnd ret: %d\n", ret);
        printf("msg.mtype: %d\n", msg.mtype);
        printf("msg.pid: %d\n", msg.pid);


        return;
}

