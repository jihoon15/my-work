#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

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
	return 0;
}

