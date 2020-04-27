#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "msg.h"

#include <time.h>

#define MAX_PROC 10  
#define MAX_TIME 1000
//*****메시지큐 사용에 필요*******
void enqueue(int input);
void dequeue();
int msgq;
int ret;
struct msgbuf msg;
#define key 0x03215
//*********************
int time_q;

void time_tick(int signo);
void IO_REQ(int signo);
void do_child();
void get_cputime(int signo);
void change(int signo);
void CNT_WAIT();

typedef struct pcb{
        struct pcb* next;
        int pid;
	int io_burst;
	int cpu_burst;
	int cnt_wait;
	int cnt_cpu;
	int cnt_io;

	int throuput;//
	int for_turnaround;
	int turnaround;
	int for_response;
	int response;
	int response_flag;
	int for_rand;
}pcb;

int cpu_burst;
int io_burst;
int for_random;

typedef struct L{
        pcb *head;
        pcb *tail;
}pcb_L;
pcb_L* r_pcb;
pcb_L* w_pcb;

pcb_L* create_pcb();
void push_ready(pcb* newpcb);
void pop_ready();
void push_wait(pcb* newpcb);
void pop_wait();
void reduce_IO();

void ready_init(int pid);

int global_counter;
void PRINT();
void give_random(int level);
void give_random_init(int order);

int ready_count;
int wait_count;

int PtoC[2];
int CtoP[2];
int flag;

int timeslice();
void check_end();

int total_waiting_time;
float total_response;
float total_turnaround;
int total_throuput;



int main(int argc, char *arg[])
{
	
	srand(time(NULL));
	int a;
	//키에 해당하는 메시지큐 생
	msgq = msgget(key, IPC_CREAT | 0666);
	memset(&msg, 0, sizeof(msg));

	ready_count=0;
	wait_count=0;

	flag=0;

        r_pcb = create_pcb();
	w_pcb = create_pcb();
        
	struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = time_tick;          //sa_handler는 signal 발생 시 할 함수
        sigaction(SIGALRM, &new_sa, &old_sa);

        struct sigaction old_sa_siguser2, new_sa_siguser2;
        memset(&new_sa_siguser2, 0, sizeof(new_sa_siguser2));
        new_sa_siguser2.sa_handler = change;
        sigaction(SIGUSR2, &new_sa_siguser2, &old_sa_siguser2);

        struct sigaction old_sa_siguser3, new_sa_siguser3;
        memset(&new_sa_siguser3, 0, sizeof(new_sa_siguser3));
        new_sa_siguser3.sa_handler = IO_REQ;
        sigaction(SIGUSR1, &new_sa_siguser3, &old_sa_siguser3);



        for(int i = 0; i < MAX_PROC; i++){
                int pid = fork();
                if(pid < 0){
                        perror("fork");
                        break;
                }
                if(pid == 0){
                        //child
			a=rand()%10 +1;
			give_random_init(i);
			
                        do_child();
                        exit(0);
                }
                else{
                        //parent
			time_q=timeslice();
                        ready_init(pid);
			ready_count++;
                }
        }


        struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
	new_timer.it_interval.tv_sec = 0;
   	new_timer.it_interval.tv_usec = 100000;
   	new_timer.it_value.tv_sec = 0;
   	new_timer.it_value.tv_usec = 10000;
        setitimer(ITIMER_REAL, &new_timer, &old_timer);


        while(1){
                sleep(1);
        }

        return 0;

}

void time_tick(int signo)
{
	PRINT();
	global_counter++;
	check_end();//

	if(wait_count!=0){
	reduce_IO();
	}
	CNT_WAIT();
	if(ready_count==0)
	{
	 printf("running queue is empty...\n");
	}
	else{
	if(flag==0){
	enqueue(time_q);
	flag=1;
	}
	 if(r_pcb->head->response_flag==0){//처음실행시킬 경우
	 r_pcb->head->response_flag=1;//1로바꿔줌
	 r_pcb->head->response+=r_pcb->head->for_response;
	 r_pcb->head->for_response=0;
	 }
	r_pcb->head->cnt_cpu++;
	r_pcb->head->for_turnaround++;
        kill(r_pcb->head->pid, SIGUSR1);
        printf("At %d : sending SIGUSR1 to (%d)\n", global_counter, r_pcb->head->pid);
	}
}
//called when the child is completed
void IO_REQ(int signo)
{		
        pop_ready();
	flag=0;
}

void change(int signo)
{
        r_pcb->head = r_pcb->head->next;
	r_pcb->tail = r_pcb->tail->next;
	flag=0;
}

void do_child()
{
        //SET UP SIGNAL FOR SIGALRM2
        struct sigaction old_sa_siguser, new_sa_siguser;
        memset(&new_sa_siguser, 0, sizeof(new_sa_siguser));
        new_sa_siguser.sa_handler = get_cputime;
        sigaction(SIGUSR1, &new_sa_siguser, &old_sa_siguser);

        while(1){
                sleep(1);
        }
}
void get_cputime(int signo)
{
	
	if(flag==0){
	dequeue();
	time_q=msg.data;
	flag=1;
	}
        cpu_burst--;
        time_q--;
        printf("exec_time (%d): %d\n", getpid(), cpu_burst);

        if(cpu_burst == 0){
		give_random(for_random);
		flag=0;
		enqueue(io_burst);
                kill(getppid(), SIGUSR1);
        }
        else if(time_q == 0){
                flag=0;
                kill(getppid(), SIGUSR2);
        }
}
/*******getppid(부모 프로세스)로 SIGUSR2 알림   *********/
pcb_L* create_pcb(){
        pcb_L *L;
        L = (pcb_L*)malloc(sizeof(pcb_L));
        L->head = NULL;
        L->tail = NULL;
        return L;
}
void pop_ready()
{

        pcb* temp;
        if(r_pcb->head == NULL){
                printf("현재 node는 empty\n");  //
                return;
        }
	temp= r_pcb->head;

	if(r_pcb->head==r_pcb->tail){
	ready_count--;
	dequeue();
	temp->io_burst=msg.data;
	r_pcb->head=NULL;
	r_pcb->tail=NULL;
	push_wait(temp);
	return;
	}
	else{
	ready_count--;
	r_pcb->head = r_pcb->head->next;
	r_pcb->tail->next = r_pcb->head;
	dequeue();
	temp->io_burst=msg.data;
	push_wait(temp);
        return;
	}
}
void pop_wait(){
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        if(w_pcb->head ==NULL ){
                printf("현재 node는 empty \n");  
                return;
        }
        temp = w_pcb->head;
	temp->throuput++;
	temp->turnaround+=temp->for_turnaround;
	temp->for_turnaround=0;
        w_pcb->head = w_pcb->head->next;
	temp->response_flag=0;//
	push_ready(temp);
	wait_count--;

        return ;
}
void push_ready(pcb* newpcb){
	if(r_pcb->head==NULL){
	r_pcb->head=newpcb;
	}
	else{
	r_pcb->tail->next=newpcb;
	}

	r_pcb->tail=newpcb;
	r_pcb->tail->next=r_pcb->head;	

	ready_count++;
}//확인
void push_wait(pcb* newpcb){
	int a;
	int b;
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        temp = w_pcb->head;    
	if(w_pcb->head==NULL){
	w_pcb->head=newpcb;
	newpcb->next=NULL;
	wait_count++;
	return;
	}
	 if(newpcb->io_burst<=temp->io_burst)
	 {
	   w_pcb->head=newpcb;
	   newpcb->next=temp;
           wait_count++;
	   return;
	 }	
	while(temp->next!=NULL)//
        {
		a=newpcb->io_burst;
		b=temp->next->io_burst;
		if(a<=b){
		break;
		}

                temp = temp->next;
        }
        newpcb->next = temp->next;
        temp->next = newpcb;
	wait_count++;
}
void ready_init(int pid)
{
        pcb *newpcb;
        newpcb = (pcb*)malloc(sizeof(pcb));
        newpcb->pid = pid;

        newpcb->next = NULL;

        if(r_pcb->head == NULL){
                r_pcb->head = newpcb;
                newpcb->next = newpcb;
                r_pcb->tail = newpcb;
        }
        else{
        r_pcb->tail->next = newpcb;
        r_pcb->tail = newpcb;
        newpcb->next = r_pcb->head;
        }

}
void reduce_IO()
{
        pcb* temp;
        temp= w_pcb->head;
        for(int i=0;i<wait_count;i++)
        {
	temp->io_burst--;
	temp->for_turnaround++;
	temp->cnt_io++;
	if(temp->io_burst==0){
	pop_wait();
	}
	temp=temp->next;
        }
	return;
}
void CNT_WAIT(){
 pcb* temp;
 if(ready_count<2){
 return;
 }
 else{
 temp=r_pcb->head->next;
  while(temp!=r_pcb->head){
   temp->cnt_wait++;
   temp->for_turnaround++;
   if(temp->response_flag==0){
   temp->for_response++;
   }
   temp=temp->next;
  }
 }
 return;
}

void PRINT()
{
pcb* temp;
temp=r_pcb->head;
printf("-----------------------READY_QUEUE---------------------------\n");
for(int k=0;k<ready_count;k++)
{
//printf(" %d",temp->pid%10);
printf("<%d>  wait: %d  io_burst:  %d  cpu_burst:  %d cpu 점유율: %f  \n",temp->pid, temp->cnt_wait, temp->cnt_io, temp->cnt_cpu,(float)(temp->cnt_cpu)/global_counter*100);
temp=temp->next;
}
printf("\n");
printf("-------------------------------------------------------------\n");
temp=w_pcb->head;
printf("-----------------------WAIT_QUEUE----------------------------\n");
for(int k=0;k<wait_count;k++)
{
//printf(" %d",temp->pid%10);
printf("<%d>  wait: %d  io_burst:  %d  cpu_burst:  %d cpu 점유율: %f \n",temp->pid, temp->cnt_wait, temp->cnt_io, temp->cnt_cpu,(float)(temp->cnt_cpu)/global_counter*100);
temp=temp->next;
}
printf("\n");
printf("-------------------------------------------------------------\n");
printf("\n\n");
}

int timeslice(){
return 4;
}
void give_random(int level){//level에 for_random 이들어감
int a= rand()%10 +1;
int b= rand()%10 +1;
int c= rand()%10 +1;
int d= rand()%10 +1;

  switch(level){
	case 1:
		cpu_burst=a%3 +1;
		io_burst=d%3 +8;
	break;

	case 2:
		cpu_burst=b%3 +3;
		io_burst=c%3 + 6;
	break;

	case 3:
		cpu_burst=c%3 + 4;
		io_burst=b%3 + 4;
	break;

	case 4:
		cpu_burst=d%3 + 6;
		io_burst=a%3 +3;
	break;

	case 5:
		cpu_burst=b%3 + 8;
		io_burst=c%3 + 1;
	break;
  }
}

void give_random_init(int order)
{
 int a=(rand()%10) + 1;
 int b=(rand()%10) + 1;
 int c=(rand()%10) + 1;
 int d=(rand()%10) + 1;
 switch(order){
	case 0 :
		cpu_burst=a%3 +1;
		io_burst=a%3 +8;
		for_random=1;
		break;
///
	case 1 :cpu_burst=a%3 + 3;
		io_burst=c%3 + 6;
		for_random=2;
		break;

	case 2 :
		cpu_burst=d%3 +3;
		io_burst=b%3 + 6;
		for_random=2;
		break;
//
	case 3 :cpu_burst=b%3 + 4;
		io_burst=a%3 + 4;
		for_random=3;
		break;
	case 4 :cpu_burst=d%3 + 4;
		io_burst=b%3 + 4;
		for_random=3;
		break;
	case 5 :cpu_burst=c%3 + 4;
		io_burst=d%3 + 4;
		for_random=3;
		break;
	case 6 :
		cpu_burst=a%3 + 4;
		io_burst=c%3 + 4;
		for_random=3;
		break;
//
	case 7 :cpu_burst=d%3 + 6;
		io_burst=c%3 +3;
		for_random=4;
		break;

	case 8 :
		cpu_burst=b%3 + 6;
		io_burst=d%3 +3;
		for_random=4;
		break;
//
	case 9 :
		cpu_burst=a%3 + 8;
		io_burst=a%3 + 1;
		for_random=5;
		break;

 }

}
void enqueue(int input)
{
msg.mtype=0;
msg.pid=getpid();
msg.data=input;
ret=msgsnd(msgq,&msg,sizeof(msg),0);
return;
}
void dequeue()
{
ret = msgrcv(msgq,&msg,sizeof(msg),0,0);
return;
}
void check_end(){
	
	if(global_counter==MAX_TIME+1){
	printf("                         <Result>\n");	
	   for(int i=0;i<ready_count;i++){
		total_response+=(float)r_pcb->head->response/r_pcb->head->throuput;
		total_turnaround+=(float)r_pcb->head->turnaround/r_pcb->head->throuput;
		total_waiting_time+=r_pcb->head->cnt_wait;
		total_throuput+=r_pcb->head->throuput;
		kill(r_pcb->head->pid,SIGKILL);
        	r_pcb->head=r_pcb->head->next;
        	}
           for(int i=0;i<wait_count;i++){
		total_response+=(float)r_pcb->head->response/r_pcb->head->throuput;
		total_turnaround+=(float)r_pcb->head->turnaround/r_pcb->head->throuput;
		total_waiting_time+=r_pcb->head->cnt_wait;
		total_throuput+=r_pcb->head->throuput;
		kill(w_pcb->head->pid,SIGKILL);
        	w_pcb->head=w_pcb->head->next;
        	}
	printf("average waiting time: %d \n",total_waiting_time/MAX_PROC);
	printf("average response time: %f \n",(float)total_response/MAX_PROC);
	printf("average turnaround time: %f \n",(float)total_turnaround/MAX_PROC);
	printf("total throughput: %d \n",total_throuput);
	exit(0);
	}

}
