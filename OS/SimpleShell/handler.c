#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

void signal_callback_handler(int signum)
{
  printf("caught signal %d - phew!\n",signum);
  exit(1);

}

int main(int argc, char *argv[])
{
	pid_t pid;

	pid = fork();
	if (pid == -1) {
		perror("fork error");
		return 0;
	}
	else if (pid == 0) {
		// child
		printf("child process with pid %d\n", 
			getpid());
	} else {
		// parent
		printf("my pid is %d\n", getpid());
		wait(0);
	}
	signal(SIGALRM,signal_callback_handler);
	alarm(3);
        int i=1;
	while(1)
	{
	 
	 printf("count down %d\n",i);
	 sleep(1);
	 i++;

	}
	return 0;
}

