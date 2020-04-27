#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "msg.h"

int main()
{
	int msgq;
	int ret;
	int key = 0x22242;
	msgq = msgget( key, IPC_CREAT | 0666);//해당key로메시지큐 생성
	printf("msgq id: %d\n", msgq);

	struct msgbuf msg;//msg.h 에있는 것/ 메시지큐로 넘겨줄 데이터
	memset(&msg, 0, sizeof(msg));
	msg.mtype = 0;
	ret = msgsnd(msgq, &msg, sizeof(msg), NULL);//메시지 보냄(msgq id, size, msgflag)//flag 0: 큐에공간생기는거 긷자림//큐에공간없을시 복귀하는것도 있음
	printf("msgsnd ret: %d\n", ret);

	return 0;
}
