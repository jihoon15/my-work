#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
  char buf[BUFSIZE];
  size_t readlen, writelen, slen;
  pid_t cpid, mypid;
  pid_t pid = getpid();         /* get current processes PID */
  printf("Parent pid: %d\n", pid);
 for(int k=0;k<3;k++){
  cpid = fork();
 }

  if (cpid > 0) {		          /* Parent Process */
   mypid = getpid();
	for(int i=0;i<10;i++){
   printf("[%d] parent of [%d]\n", mypid, cpid);
	sleep(1);
     }
  }  else if (cpid == 0) {	 /* Child Process */
   mypid = getpid();
	for(int j=0;j<10;j++){
   printf("[%d] child\n", mypid);
	sleep(1);
	}
  } else {
    perror("Fork failed");
    exit(1);
  }
  exit(0);
}

