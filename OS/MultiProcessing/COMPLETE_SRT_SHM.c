#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_PROC 10 
#define KEY_NUM 1234
#define MEM_SIZE 8 


int exec_time;
int all_completed;
int recive_time;
int global_counter;
int check_first;
float waiting_time = 0;
int q_empty = 0;


void time_tick(int signo);
void child_completed(int signo);
void do_child();
void child_sighandler(int signo);
void dec_waitq();
void calcu_waiting();
void waitq_print();
void runq_print();
void finish_io();
void end_of_program();
void check_starvate();
int real_time(int i, int time);
int real_time2(int i, int time);




typedef struct sh_mem{
	int exec_cpu;
	int recive_io;
}sh;

typedef struct pcb{
	struct pcb* next;
	int pid;
	int io_time;
	int cpu_time;
	int all_wait_time;
	int wait_time;
	int must_do;
	int form;
	int cnt_cpu;
	int throuput;
	int for_response;
	int response;
	int response_flag;
	int for_turnaround;
	int turnaround;
}pcb;

int total_throuput;
float total_response;
float total_turnaround;

typedef struct L{
	pcb *head;
}pcb_L;


pcb_L* create_pcb(void)
{
	pcb_L *L;
	L = (pcb_L*)malloc(sizeof(pcb_L));
	L->head = NULL;
	return L;
}


void push_pcb(pcb_L *L, int pid, int cpu_time, int wait_time, int form)
{
	pcb *newpcb;
	pcb *temp;
	newpcb = (pcb*)malloc(sizeof(pcb));
	temp = (pcb*)malloc(sizeof(pcb));
	newpcb->pid = pid;
	newpcb->cpu_time = cpu_time;
	newpcb->all_wait_time = wait_time;
	newpcb->wait_time = wait_time;
	newpcb->must_do = wait_time;
	newpcb->form = form;
	newpcb->next = NULL;

	if(L->head == NULL){
		L->head = newpcb;
		newpcb->next = NULL;
		return;
	}

	temp = L->head;
	if(temp->next == NULL){
		if(temp->cpu_time <= newpcb->cpu_time){
			temp->next = newpcb;
			newpcb->next = NULL;
		}
		else{
			newpcb->next = temp;
			L->head = newpcb;
			temp->next = NULL;
		}
	}
	else{
		if(temp->cpu_time <= newpcb->cpu_time){
			if(temp->next != NULL){	
				while(temp->next->cpu_time <= newpcb->cpu_time){
					temp = temp->next;
					if(temp->next == NULL)
						break;
				}
			}	
			newpcb->next = temp->next;
			temp->next = newpcb;
		}
		else{
			newpcb->next = temp;
			L->head = newpcb;
		}
	}

}

void push_old_pcb(pcb_L *L , pcb* newpcb, int cpu_time)
{

	pcb *temp;
	pcb *temp2;
	temp = (pcb*)malloc(sizeof(pcb));
	temp2 = (pcb*)malloc(sizeof(pcb));
	newpcb->cpu_time = cpu_time;
	newpcb->next = NULL;


	if(L->head == NULL){
		L->head = newpcb;
		newpcb->next = NULL;	
		return;
	}
	
	temp = L->head;
	while(temp->must_do == 1){
		temp2 = temp;
		temp = temp->next;
	}	
	if(temp == NULL){
		temp->next = newpcb;
		newpcb->next = NULL;
		return;
	}

	if(temp->next == NULL){	
		if(temp->cpu_time <= newpcb->cpu_time){
			temp->next = newpcb;
			newpcb->next = NULL;
		}
		else{	
			newpcb->next = temp;
			if(L->head->must_do == 1)
				temp2->next = newpcb;
			else
				L->head = newpcb;
			temp->next = NULL;
		}
	}
	else{	
		if(temp->cpu_time <= newpcb->cpu_time){
			if(temp->next != NULL){	
				while(temp->next->cpu_time <= newpcb->cpu_time){
					temp = temp->next;
					if(temp->next == NULL)
						break;		
				}	
			}
			newpcb->next = temp->next;
			temp->next = newpcb;
		}
		else{	
			newpcb->next = temp;
			if(L->head->must_do == 1)
				temp2->next = newpcb;
			else
				L->head = newpcb;
		}
	}
			

}


void push_wait(pcb_L *L, pcb* newpcb)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));

	if(L->head == NULL){
		L->head = newpcb;
		newpcb->next = NULL;	
		return;
		
	}

	temp = L->head;
	if(temp->next == NULL){
		if(temp->io_time <= newpcb->io_time){
			temp->next = newpcb;
			newpcb->next = NULL;
		}
		else{
			newpcb->next = temp;
			L->head = newpcb;
			temp->next = NULL;
		}
	}
	else{
		if(temp->io_time <= newpcb->io_time){	
			if(temp->next != NULL){	
				while(temp->next->io_time <= newpcb->io_time){
					temp = temp->next;
					if(temp->next == NULL)
						break;
				}
			}	
			newpcb->next = temp->next;
			temp->next = newpcb;
		}
		else{
			newpcb->next = temp;
			L->head = newpcb;
		}
	}
}

//starvation 시에 pop한 노드 재 push
void push_mid_pcb(pcb_L* L, pcb* old)
{
	old->next = L->head;
	L->head = old;
}


pcb*  pop_pcb(pcb_L *L)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));
	if(L->head == NULL){
		printf("현재 node는 empty\n");	
		return NULL;
	}
	else if(L->head->next == NULL){
		temp = L->head;
		L->head = NULL;
		temp->next = NULL;
		return temp;
	}

	temp = L->head;
	L->head = L->head->next;
	temp->next = NULL;
	return temp;
}

//starvation 시에 running queue의 중간에서 pop
pcb* pop_mid_pcb(pcb_L *L, pcb* old)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));
	temp = L->head;
	while(temp->next != old)
		temp = temp->next;
	
	temp->next = old->next;
	old->next = NULL;

	return old;
}	


pcb *curr;
pcb_L *p_pcb;
pcb_L *wait_q;	
sh* shm_info = NULL;



int main(int argc, char *arg[])
{
	check_first = 1;
	srand(time(NULL));	
	all_completed = 0;
	int cpu_time;
	int shmid;
	int pid;
	int status;
	void* address = (void*)0;
	
	if((shmid = shmget(KEY_NUM, 8, IPC_CREAT | 0666)) != (-1)){
		address = shmat(shmid, NULL, 0);

		printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
		printf("공유 메모리 생성\n");
		printf("shmid = %d\n", shmid);
		printf("공유 메모리 주소 : %p\n", address);
		printf("ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ\n");
	}
	else
		printf("공유 메모리 실패\n");

	shm_info = (sh*)address;


	p_pcb = create_pcb();
	wait_q = create_pcb();

	//SIGALRM
	struct sigaction old_sa, new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = time_tick;	
	sigaction(SIGALRM, &new_sa, &old_sa);
	//time_tick 함수로 이동



	//SIGUSR1
	struct sigaction old_sa_siguser1, new_sa_siguser1;
	memset(&new_sa_siguser1, 0, sizeof(new_sa_siguser1));
	new_sa_siguser1.sa_handler = child_completed;
	sigaction(SIGUSR1, &new_sa_siguser1, &old_sa_siguser1);
	//child_completed 함수로 이동


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
			cpu_time = (rand() % 10) + 1;
			cpu_time = real_time(i, cpu_time);
			push_pcb(p_pcb, pid, cpu_time, 0, i);
		}

	}
	



	struct itimerval new_timer, old_timer;
	memset(&new_timer, 0, sizeof(new_timer));
	new_timer.it_interval.tv_sec = 0;	//몇초 간격으로 알람할지
	new_timer.it_interval.tv_usec = 20000;
	new_timer.it_value.tv_sec = 1;		//처음 알람 시작을 몇초 뒤에 할지
	new_timer.it_value.tv_usec = 0;
	
	//설정할 타이머 종류, 설정할 타이머 정보 구조체, 이전 타이머 정보 구조체
	setitimer(ITIMER_REAL, &new_timer, &old_timer);
	
	curr = p_pcb->head;  
	while(1){
		sleep(1);
	}

	return 0;

}


//when SIGARM occur
void time_tick(int signo)
{
	global_counter++;
	printf("%d tick\n", global_counter - 1);

	calcu_waiting();
	waitq_print();
	finish_io();	

	if(p_pcb->head == NULL){
		q_empty = 1;
		dec_waitq();
		runq_print();
		end_of_program();
		return;
	}
	if(p_pcb->head != NULL){	
		if(check_first != 1 && q_empty != 1){
			curr->cpu_time = shm_info->exec_cpu;
		}
	}

	check_starvate();
	
	curr = p_pcb->head;

	check_first = 0;
	q_empty = 0;

	dec_waitq();
		
	shm_info->exec_cpu  = curr->cpu_time;	

	runq_print();
	end_of_program();
	
	if(curr->response_flag == 0){
		curr->response_flag = 1;
		curr->response += curr->for_response;
		curr->for_response = 0;
	}
	curr->cnt_cpu++;
	curr->for_turnaround++;
	kill(curr->pid, SIGUSR2);

	
}
//curr->pid(자식프로세스)로 SIGUSR2을 보냄(do_child 함수에서 받음)


//when SIGUSR1 occur
void child_completed(int signo)
{
	curr->cpu_time = shm_info->exec_cpu; 
	curr->io_time = real_time2(curr->form, shm_info->recive_io);
	curr->must_do = 0;
	push_wait(wait_q, pop_pcb(p_pcb));
}


//start child process
void do_child()
{
	//SIGUSR2
	struct sigaction old_sa_siguser2, new_sa_siguser2;
	memset(&new_sa_siguser2, 0, sizeof(new_sa_siguser2));
	new_sa_siguser2.sa_handler = child_sighandler;
	sigaction(SIGUSR2, &new_sa_siguser2, &old_sa_siguser2);

	while(1){
		sleep(1);
	}
}


//when SIGUSR2 occur
void child_sighandler(int signo)
{
	shm_info->exec_cpu--;
	exec_time = shm_info->exec_cpu;

	if(exec_time == 0){
		recive_time = (rand() % 10) + 1;
		shm_info->recive_io  = recive_time;
		kill(getppid(), SIGUSR1);
	}

}


//waitq의 io_time 소비
void dec_waitq()
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));

	if(wait_q->head != NULL){	
		temp = wait_q->head;
		temp->io_time--;

		temp->for_turnaround++;

		if(temp->next != NULL){
			temp = temp->next;
			temp->io_time--;

			temp->for_turnaround++;
			
		}
	}

}


//각 프로세스의 대기 시간 적립
void calcu_waiting()
{	
	if(check_first != 1 && global_counter <= 1000){
		pcb* for_wait;
		for_wait = (pcb*)malloc(sizeof(pcb));
		for_wait = p_pcb->head;
		if(for_wait != NULL)
			for_wait = for_wait->next;
		while(for_wait != NULL){
			for_wait->all_wait_time++;
			for_wait->wait_time++;
			
			for_wait->for_turnaround++;
			if(for_wait->response_flag == 0){
				for_wait->for_response++;
			}

			for_wait = for_wait->next;
		}
	}

}


//waitq 상태 출력
void waitq_print()
{
	pcb* for_print;
	for_print = (pcb*)malloc(sizeof(pcb));
	for_print = wait_q->head;
	printf("----------------wait queue---------------\n\n");
	if(wait_q->head != NULL){
		while(for_print != NULL){
			if(for_print->io_time != 0)
				printf("(%d)노드 cpu_time : %d, io_time : %d, cpu점유율:%f  \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, (float)(for_print->cnt_cpu) / global_counter * 100);
			for_print = for_print->next;
		}
	}
	else
		printf("                   EMPTY                 \n\n");
	printf("-----------------------------------------\n\n");
}


//runq 상태 출력(편의상 runq는 runing 큐와 ready 큐를 합쳐 출력)
void runq_print()
{
	pcb* for_print;
	for_print = (pcb*)malloc(sizeof(pcb));
	for_print = p_pcb->head;
	printf("---------------running queue-------------\n\n");
	if(p_pcb->head != NULL){
		while(for_print != NULL){
			printf("(%d)노드 cpu_time : %d, io_time : %d, cpu점유율:%f \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, (float)(for_print->cnt_cpu) / global_counter * 100);
			for_print = for_print->next;
		}
	}
	else
		printf("                   EMPTY                 \n\n");
	printf("-----------------------------------------\n\n\n\n\n");

}


//io time이 0인 프로세스 waitq에서 runq로 이동
void finish_io()
{
	if(wait_q->head != NULL){
		while(wait_q->head->io_time == 0){

			wait_q->head->throuput++;
			wait_q->head->turnaround+=wait_q->head->for_turnaround;
			wait_q->head->for_turnaround=0;
			wait_q->head->response_flag=0;

			push_old_pcb(p_pcb, pop_pcb(wait_q), real_time(p_pcb->head->form , (rand() % 10) + 1));
			if(wait_q->head == NULL)
				break;
		}
	}
}


//1000 tick이 끝날을 때
void end_of_program()
{
	if(global_counter == 1001){
		pcb* wtemp;
		wtemp = (pcb*)malloc(sizeof(pcb));
		if(wait_q->head != NULL){
			wtemp = wait_q->head;
			while(wtemp != NULL){
				waiting_time += wtemp->all_wait_time;

				total_throuput += wtemp->throuput;
				total_response += (float)(wtemp->response) / wtemp->throuput;
				total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;

				wtemp = wtemp->next;
			}
		}
		if(p_pcb->head != NULL){
			wtemp = p_pcb->head;
			while(wtemp != NULL){
				waiting_time += wtemp->all_wait_time;

				total_throuput += wtemp->throuput;
				total_response += (float)(wtemp->response) / wtemp->throuput;
				total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;

				wtemp = wtemp->next;
			}
		}
		printf("1000 tick이 되어 프로그램을 종료합니다\n");
		printf("waiting time : %.5f\n", waiting_time / 10);
		printf("average response time: %f \n", (float)total_response / MAX_PROC);
		printf("average turnaround time: %f \n", (float)total_turnaround / MAX_PROC);
		printf("total throughput: %d \n", total_throuput);

		pcb* temp;
		temp = (pcb*)malloc(sizeof(pcb));
		while(p_pcb->head != NULL){				
			temp = p_pcb->head;
			p_pcb->head = p_pcb->head->next;
			kill(temp->pid, SIGINT);
		}	
		exit(0);
	}

}

//starvation이 나오는 경우
void check_starvate()
{
	pcb* temp;
	pcb* temp2;
	temp = (pcb*)malloc(sizeof(pcb));
	temp2 = (pcb*)malloc(sizeof(pcb));
	temp = p_pcb->head;
	while(temp != NULL){
		if(temp->wait_time ==100){
			temp2 = temp->next;
			temp->must_do = 1;
			push_mid_pcb(p_pcb, pop_mid_pcb(p_pcb, temp));
			temp->wait_time = 0;
			temp = temp2;
		}
		else
			temp = temp->next;
	}
}

int real_time(int i, int time)
{
        int real;
        switch(i)      {
                case 0 :
                        real = (time % 3) + 1;
                        break;
                case 1 :
		case 2 :
                        real = (time % 3) + 3;
                        break;
                case 3 :
		case 4 :
		case 5 :
		case 6 :
                        real = (time % 3) + 4;
                        break;
                case 7 :
		case 8 :
                        real = (time % 3) + 6;
                        break;
                case 9 :
                        real = (time % 3) + 8;
                        break;
                default :
                        printf("오류오류\n");
                }

        return real;
}


int real_time2(int i, int time)
{
        int real;
        switch(i)      {
                case 0 :
                        real = (time % 3) + 8;
                        break;
                case 1 :
		case 2 :
                        real = (time % 3) + 6;
                        break;
                case 3 :
		case 4 :
		case 5 :
		case 6 :
                        real = (time % 3) + 4;
                        break;
                case 7 :
		case 8 :
                        real = (time % 3) + 3;
                        break;
                case 9 :
                        real = (time % 3) + 1;
                        break;
                default :
                        printf("오류오류\n");
                }

        return real;
}

