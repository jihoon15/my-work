#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define MAX_PROC 3

int exec_time;

void time_tick(int signo);
void child_completed(int signo);
void do_child();
void child_sighandler(int signo);

struct pcb{
        int pid;
        int completed;
};

struct pcb *curr;
int global_counter;
struct pcb pcbs[MAX_PROC];      //배열은 좋지않다? 왜? 다른거 생각해보자.
                                //아마 링크드 리스트?

int main(int argc, char *arg[])
{

        exec_time = 10;



/*******setitiomer에서 오는 알람으로 시작********/
        //SET UP SIGNAL FOR SIGALRM
        struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = time_tick;          //sa_handler는 signal 발생 시 할   함수
        sigaction(SIGALRM, &new_sa, &old_sa);
/*******time_tick 함수로 이동           *********/



/*******child_sighandler에서 SIGUSR2 알람********/
        //SET UP SIGNAL FOR SIGALRM2
        struct sigaction old_sa_siguser, new_sa_siguser;
        memset(&new_sa_siguser, 0, sizeof(new_sa_siguser));
        new_sa_siguser.sa_handler = child_completed;
        sigaction(SIGUSR2, &new_sa_siguser, &old_sa_siguser);
/*******child_completed 함수로 이동     *********/






        for(int i = 0; i < MAX_PROC; i++){

                int pid = fork();
                if(pid < 0){
                        //error
                        perror("fork");
                        break;
                }
                if(pid == 0){
                        //child
                        do_child();
                        exit(0);
                }
                else{
                        //parent
                        pcbs[i].pid = pid;
                //      pcbs[i].completed = 0;//**
                }

        }




        struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
        new_timer.it_interval.tv_sec = 1;       //몇초 간격으로 알람할지
        new_timer.it_interval.tv_usec = 0;
        new_timer.it_value.tv_sec = 1;          //처음 알람 시작을 몇초 뒤에 할  지
        new_timer.it_value.tv_usec = 0;

        //설정할 타이머 종류, 설정할 타이머 정보 구조체, 이전 타이머 정보 구조체
        setitimer(ITIMER_REAL, &new_timer, &old_timer);

        curr = &pcbs[0];        //시작은 첫 번째 배열부터 - 첫번째 자식프로세스

        while(1){
                sleep(1);
        }

        return 0;

}


/*******첫번째 sigaction에서 호출********/
//called when timer expires
void time_tick(int signo)
{
        //sending SIGUSER1 to child자
        global_counter++;       //일단 임시로 전체 카운터를 해놓은거고 나중엔 in  terval과 value에 맞춰 바꾸
        kill(curr->pid, SIGUSR1);
        printf("At %d : sending SIGUSR1 to (%d)\n", global_counter, curr->pid);

}
/*******curr->pid(자식프로세스)로 SIGUSR1을 보냄(do_child 함수의 sigaction 활성  화)*******/


/*******두번째 sigaction에서 호출********/
//called when the child is completed
void child_completed(int signo)
{

        int all_completed = 0;

        //change curr state
        curr->completed = 1;
        //change curr
        //search the array untill you find the uncompleted process
        for(int i = 0; i < MAX_PROC; i++){
                if(pcbs[i].completed == 0){
                        curr = &pcbs[i];
                        break;
                }
                else{
                        all_completed++;
                }
        }

        if(all_completed == MAX_PROC){
                exit(0);
        }
        //check the terminal cond
}
/*******모든 자식 프로세스 끝난거 확인 후 부모 프로세스도 종료  *********/


/*******자식 프로세스가 시작되면 같이 시작됐다가,
        time_tick에서 SIGUSR1이 넘어 올때까지 대기*******/
void do_child()
{
        //SET UP SIGNAL FOR SIGALRM2
        struct sigaction old_sa_siguser, new_sa_siguser;
        memset(&new_sa_siguser, 0, sizeof(new_sa_siguser));
        new_sa_siguser.sa_handler = child_sighandler;
        sigaction(SIGUSR1, &new_sa_siguser, &old_sa_siguser);

        while(1){
                sleep(1);
        }
}
/*******child_sighandler 함수로 이동            *********/


/*******do_chlid에서 불러옴                     *********/
void child_sighandler(int signo)
{
        exec_time--;
        printf("exec_time (%d): %d\n", getpid(), exec_time);

        if(exec_time == 0){
                //completed
                kill(getppid(), SIGUSR2);
                printf("completed %d\n", getpid());
                exit(0);
        }
}
/*******getppid(부모 프로세스)로 SIGUSR2 알림   *********/

