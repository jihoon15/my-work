#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	pid_t pid;

	for(int i=0;i<10;i++){

	pid=fork();

	if (pid == -1) {
		perror("fork error");
		return 0;
	}
	else if (pid == 0) {
		// child
		printf("%d child process with pid %d\n", 
			i,getpid());
			return 0;
	} else {
		// parent
		wait(0);
	}


	}
	return 0;
}

