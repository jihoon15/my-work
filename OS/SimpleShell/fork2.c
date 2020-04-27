#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
  char buf[BUFSIZE];
  size_t readlen, writelen, slen;
  pid_t cpid, mypid;

  int status;

  pid_t pid = getpid();         /* get current processes PID */
  printf("Parent pid: %d\n", pid);
  cpid = fork();
  if (cpid > 0) {		          /* Parent Process */
   mypid = getpid();
   printf("[%d] parent of [%d]\n", mypid, cpid);
   wait(&status);
   printf("the end\n");
  }  else if (cpid == 0) {	 /* Child Process */
   mypid = getpid();
   for(int i=0;i<5;i++)
   {
   printf("[%d] child\n", mypid);
   sleep(1);
   }
   exit(3);
  } else {
    perror("Fork failed");
    exit(0);
  }
}

