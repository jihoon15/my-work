#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "msg.h"

int main()
{
	int msgq;
	int ret;
	int key = 0x22242;
	msgq = msgget( key, IPC_CREAT | 0666);
	printf("msgq id: %d\n", msgq);

	struct msgbuf msg;
	memset(&msg, 0, sizeof(msg));
	ret = msgrcv(msgq, &msg, sizeof(msg), 0, 0);
	printf("msgsnd ret: %d\n", ret);
	printf("msg.mtype: %d\n", msg.mtype);
	

	return 0;
}
