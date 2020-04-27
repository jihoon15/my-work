#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#define BUFSIZE 1024

#include "simple_shell.h"

void shell()
{
 char str[BUFSIZE];
 char *user = getenv("USER");
 pid_t pid;
 pid_t status;

  while(1)
   {
   //________________현재 디렉토리__________________
    char* pwd;
    pwd=getcwd(NULL,BUFSIZ);
    printf("%s$(shell) ",pwd);
    //__________________clock__________________________
    struct tm* clock;
    time_t get_time;
    time(&get_time);//현재시간을채움
    clock = localtime(&get_time);
    //_______________________________________________

        fgets(str,BUFSIZE,stdin);
	str[strlen(str)-1]='\0';

	 strtok(str," ");
	 char* arg1=strtok(NULL," ");
	 char* arg2=strtok(NULL," ");
	 char* arg3=strtok(NULL," ");
	 char* arg4=strtok(NULL," ");

	 if(strcmp(str,"quit")==0){break;}
	 //user
	 if(strcmp(str,"user")==0){
	 printf("%s\n",user);
	 continue;
	 }
	 //pwd
	 if(strcmp(str,"pwd")==0){
	 printf("%s\n",pwd);
	 continue;
	 }
	 //time
	 if(strcmp(str,"time")==0){
	  printf("%s",asctime(clock));
	  continue;
	  }
	 //cd
	 if(strcmp(str,"cd")==0){
	 chdir(arg1);
	 continue;
	 }

	 pid=fork();
	 //_______________________________________________

	     if(pid==-1) {
	       perror("fork error");
	       exit(1);
	       }

	     else if (pid == 0) {//자식
	       execlp(str,str,arg1,arg2,arg3,arg4,(char*)0);
	       exit(0); 
	      }
	     else {//부모
	         waitpid(pid,&status,0);
		}

	}

	return;

}





















