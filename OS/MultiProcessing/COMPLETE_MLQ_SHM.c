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

#define MAX_PROC 10 	//프로세스 개수(자식)
#define KEY_NUM 1234	//shared memory에 주소
#define MEM_SIZE 20 	//shared memory에 크기
#define batch_str 100
#define student_str 250
#define ba_starvate 30
#define stu_starvate 12

/****************자식 프로세스에서 사용*****************/
int exec_time;		//프로세스가 작업을 마치는데 걸리는 시간
int run_time;		//round robin 방식에서 quantum time
int recive_time;	//wait queue의 wait time
/*******************************************************/
/****************부모 프로세스에서 사용*****************/
int global_counter;	//OS의 시간 경과(초 단위)
int check_first;	//첫번째 tick인지 확인
float waiting_time = 0;	//cpu를 할당 받지 못한 전체 시간 계산
int q_empty = 0;	//running queue가 비었는지 확인
int multi_level;	//5단계의 muti level queue 중 몇번째 인지 확인
int starvate = 0;	//한 queue가 오랬동안 대기 하면 1로 바꾸게 해줌
/******************************************************/

//함수에 대한 설명은 main문 아래 함수 부분에 써 있음
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
int runq_empty();
int real_time(int ml, int time);
void r_robin_match();
void curr_match();
void calcu_response();


typedef struct sh_mem{	//shared memory
	int exec_cpu;
	int recive_io;
	int level;
	int run_quantun;
}sh;

typedef struct pcb{	//자식 프로세스의 노드 구조체
	struct pcb* next;
	int pid;
	int io_time;
	int cpu_time;
	int mul_lv;
	int all_wait_time;
	int wait_time;
	int quantun_time;

	int throuput;
	int for_turnaround;
	int turnaround;
	int for_response;
	int response;
	int response_flag;
	int cnt_cpu;
}pcb;

int total_throuput;
float total_response;
float total_turnaround;
int total_throuput;

typedef struct L{	//큐 포인터의 구조체
	pcb *head;
	pcb *tail;
	int num;
}pcb_L;


pcb_L* create_pcb(void)	//큐 포인터 생성
{
	pcb_L *L;
	L = (pcb_L*)malloc(sizeof(pcb_L));
	L->head = NULL;
	L->tail = NULL;
	L->num  = 0;
	return L;
}

//초기단계 running queue에 push(runing queue + ready queue를편의상 running queue라 하겠음)
//(추가로 mu_lv이 1인 systems은 넣어줄 때 cpu time으로 비교하고 정렬해서 넣어줌)
void push_pcb(pcb_L *L, int pid, int cpu_time, int mul_lv)
{
	pcb *newpcb;
	pcb *temp;
	newpcb = (pcb*)malloc(sizeof(pcb));
	temp = (pcb*)malloc(sizeof(pcb));
	newpcb->pid = pid;
	newpcb->mul_lv = mul_lv;	
	newpcb->cpu_time = real_time(mul_lv, cpu_time);
	newpcb->all_wait_time = 0;
	newpcb->wait_time = 0;
	newpcb->quantun_time = 3;
	newpcb->next = NULL;
	L->num++;


	if(newpcb->mul_lv == 1){		//multi level이 1 일때
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
	else{					//muti levle이 2, 3, 4, 5 일 
		if(L->head == NULL){
             		L->head = newpcb;
           		newpcb->next = newpcb;
          		L->tail = newpcb;
                	return;
       		}

     		L->tail->next = newpcb;
       		L->tail = newpcb;
	        newpcb->next = L->head;
	}

			
}

//wait queue에서 running queue로 넘어올 때의 push
//(추가로 mu_lv이 1인 systems은 넣어줄 때 cpu time으로 비교하고 정렬해서 넣어줌)
void push_old_pcb(pcb_L *L , pcb* newpcb, int cpu_time)
{

	pcb *temp;
	pcb *temp2;
	temp = (pcb*)malloc(sizeof(pcb));
	temp2 = (pcb*)malloc(sizeof(pcb));
	newpcb->cpu_time = real_time(newpcb->mul_lv, cpu_time);
	newpcb->next = NULL;
	L->num++;


	if(newpcb->mul_lv == 1){
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
	else{
		if(L->head == NULL){
             		L->head = newpcb;
           		newpcb->next = newpcb;
          		L->tail = newpcb;
                	return;
       		}

     		L->tail->next = newpcb;
       		L->tail = newpcb;
	        newpcb->next = L->head;
	}


}

//wait queue에 push(io time에 대해 정렬해서 넣어줌)
void push_wait(pcb_L *L, pcb* newpcb)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));
	L->num++;	

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

//running queue에서의 pop(queue의 맨 앞 노드를 pop함)
pcb*  pop_pcb(pcb_L *L)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));
	L->num--;
	if(L->head == NULL){
		L->num++;
		printf("현재 node는 empty\n");	
		return NULL;
	}
	if(L->head->mul_lv == 1){
		if(L->head->next == NULL){
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
	else{
		if(L->head == L->tail){
			temp = L->head;
			L->head = NULL;
			L->tail = NULL;
			temp->next = NULL;
			return temp;
		}
		
		temp = L->head;
		L->head = L->head->next;
		L->tail->next = L->head;
		temp->next = NULL;
		return temp;
	}		
	
}

//wait queue에서의 pop(queue의 맨 앞 노드를 pop)
pcb* pop_wait(pcb_L *L)
{
	pcb* temp;
	temp = (pcb*)malloc(sizeof(pcb));
	L->num--;
	if(L->head == NULL){
		L->num++;
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


pcb *curr;			//cpu를 할당 받는 프로세스 노드 지정
pcb_L *p_pcb;			//5단계의 queue 중 지정할 queue를 가르킴
pcb_L *systems;			//첫 번째 running queue
pcb_L *interactive;		//두 번째 running queue
pcb_L *int_editing;		//세 번째 running queue
pcb_L *batch;			//네 번째 running queue
pcb_L *student;			//다섯 번째 running queue
pcb_L *wait_q;			//wait queue
pcb_L* matching_q(int ml);	//함수에 대한 설명은 아래에서 확인	
sh* shm_info = NULL;		//shared memory 포인터



int main(int argc, char *arg[])
{
	check_first = 1;
	srand(time(NULL));	
	int cpu_time;
	int shmid;
	int pid;
	int status;
	void* address = (void*)0;
	run_time = 3;
	
	//shared memory 생성
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


	//queue들 생성
	p_pcb = create_pcb();
	systems = create_pcb();
	interactive = create_pcb();
	int_editing = create_pcb();
	batch = create_pcb();
	student = create_pcb();
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
			if(i == 0)
				multi_level = 1;
			else if(i == 1 || i == 2)
				multi_level = 2;
			else if(i == 3 || i == 4 || i == 5 || i == 6)
				multi_level = 3;
			else if(i == 7 || i == 8)
				multi_level = 4;
			else
				multi_level = 5;
			p_pcb = matching_q(multi_level);
			cpu_time = (rand() % 10) + 1;
			push_pcb(p_pcb, pid, cpu_time, multi_level);
		}

	}
	



	struct itimerval new_timer, old_timer;
	memset(&new_timer, 0, sizeof(new_timer));
	new_timer.it_interval.tv_sec = 0;	//몇초 간격으로 알람할지
	new_timer.it_interval.tv_usec = 100000;
	new_timer.it_value.tv_sec = 1;		//처음 알람 시작을 몇초 뒤에 할지
	new_timer.it_value.tv_usec = 0;
	
	//설정할 타이머 종류, 설정할 타이머 정보 구조체, 이전 타이머 정보 구조체
	setitimer(ITIMER_REAL, &new_timer, &old_timer);
	
	curr = systems->head;  
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
	
	if(runq_empty()){
		q_empty = 1;
		dec_waitq();
		runq_print();
		end_of_program();
		return;
	}

	if(check_first != 1 && q_empty != 1){
		curr->cpu_time = shm_info->exec_cpu;
		curr->quantun_time = shm_info->run_quantun;
	}

	r_robin_match();
	curr_match();

	check_first = 0;
	q_empty = 0;

	dec_waitq();
		
	shm_info->exec_cpu  = curr->cpu_time;
	printf("curr->mul_lv : %d\n", curr->mul_lv);	
	shm_info->level = curr->mul_lv;

	runq_print();
	end_of_program();
	calcu_response();
	
	kill(curr->pid, SIGUSR2);

	
}
//curr->pid(자식프로세스)로 SIGUSR2을 보냄(do_child 함수에서 받음)


//when SIGUSR1 occur
void child_completed(int signo)
{
	curr->cpu_time = shm_info->exec_cpu; 
	curr->io_time = shm_info->recive_io;

	p_pcb = matching_q(curr->mul_lv);
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
	if(shm_info->level == 1){
		if(exec_time == 0){
			recive_time = (rand() % 3) + 8;
			shm_info->recive_io  = recive_time;
			kill(getppid(), SIGUSR1);
		}
	}
	else if(shm_info->level == 2 || shm_info->level == 3){
		run_time--;
		shm_info->run_quantun = run_time;
		if(exec_time == 0){
			run_time = 3;
			if(shm_info->level == 2)
				recive_time = (rand() % 3) + 6;
			else if(shm_info->level ==3)
				recive_time = (rand() % 3) + 4;
			shm_info->recive_io = recive_time;
			kill(getppid(), SIGUSR1);
		}
		else if(run_time == 0){
			run_time = 3;
		}
	}
	else{
		if(exec_time == 0){
			if(shm_info->level == 4)
				recive_time = (rand() % 3) + 3;
			else if(shm_info->level == 5)
				recive_time = (rand() % 3) + 1;
			shm_info->recive_io  = recive_time;
			kill(getppid(), SIGUSR1);
		}
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
	if(batch->head != NULL && batch->head->wait_time > batch_str && starvate == 0){		//batch의 starvation 현상 발생 시
		starvate = 1;									//aging 실행
	}
	else if(student->head != NULL && student->head->wait_time > student_str && starvate == 0){	//student의 starvation 현상 발생 시
		starvate = 1;									//aging 실행
	}

	if(check_first != 1 && global_counter <= 1000){
		pcb* for_wait;
		for_wait = (pcb*)malloc(sizeof(pcb));
		int num = 0;
		for(int i = 1; i < 6; i++){
			p_pcb = matching_q(i);		
			for_wait = p_pcb->head;
			if(i == 1){
				while(for_wait != NULL && p_pcb->num >= num){
					num++;
					for_wait->all_wait_time++;
					for_wait->wait_time++;

					for_wait->for_turnaround++;
					if(for_wait->response_flag == 0){
						for_wait->for_response++;
					}

					for_wait = for_wait->next;
				}
			}
			else{
				if(for_wait == NULL)
					continue;
				else{
					while(for_wait != p_pcb->tail && p_pcb->num > num){
						num++;
						for_wait->all_wait_time++;
						for_wait->wait_time++;
						
						for_wait->for_turnaround++;
						if(for_wait->response_flag == 0){
							for_wait->for_response++;
						}

						for_wait = for_wait->next;
					}
					for_wait->all_wait_time++;
					for_wait->wait_time++;

					for_wait->for_turnaround++;
					if(for_wait->response_flag == 0){
						for_wait->for_response++;
					}
					
				}
			}
		}
	}
	if(curr != NULL){
		curr->all_wait_time--;
		curr->wait_time--;

		curr->for_turnaround--;
		if(curr->response_flag == 0){
			curr->for_response--;
		}
	}


}


//waitq 상태 출력
void waitq_print()
{
	pcb* for_print;
	for_print = (pcb*)malloc(sizeof(pcb));
	for_print = wait_q->head;
	int num = 0;
	printf("-----------------------wait queue----------------------\n\n");
	if(wait_q->head != NULL){
		while(for_print != NULL && wait_q->num >= num){
			num++;
			if(for_print->io_time != 0)
				printf("  (%d)노드 cpu_time : %d, io_time : %d, multi_level : %d, cpu점유율: %f \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, for_print->mul_lv, (float)(for_print->cnt_cpu) / global_counter * 100);
			for_print = for_print->next;
		}
	}
	else
		printf("                          EMPTY                        \n\n");
	printf("-------------------------------------------------------\n\n");
}


//runq 상태 출력(편의상 runq는 runing 큐와 ready 큐를 합쳐 출력)
void runq_print()
{
	for(int i = 1; i < 6; i++){
		p_pcb = matching_q(i);
		pcb* for_print;
		for_print = (pcb*)malloc(sizeof(pcb));
		for_print = p_pcb->head;
		int num = 0;
		switch(i){
			case 1 : 
			printf("-----------------------system queue--------------------\n\n");
				break;
			case 2 :
			printf("---------------------interactive queue-----------------\n\n");
				break;
			case 3 :
			printf("---------------------int_editing queue-----------------\n\n");
			break;
			case 4 :
			printf("------------------------batch queue--------------------\n\n");
			break;
			case 5 :
			printf("-----------------------student queue-------------------\n\n");
			break;
			default :
			break;
		}
		if(i == 1){
			if(p_pcb->head != NULL){
				while(for_print != NULL && p_pcb->num >= num){
					num++;
					printf("  (%d)노드 cpu_time : %d, io_time : %d, multi_level : %d, cpu점유율: %f \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, for_print->mul_lv, (float)(for_print->cnt_cpu) / global_counter * 100);
					for_print = for_print->next;
				}
			}
			else
				printf("                          EMPTY                      \n\n");
		}
		else{
			if(p_pcb->head != NULL){
				while(for_print->next != p_pcb->head && p_pcb->num >= num){	//문제
					num++;
					printf("  (%d)노드 cpu_time : %d, io_time : %d, multi_level : %d, cpu점유율: %f \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, for_print->mul_lv, (float)(for_print->cnt_cpu) / global_counter * 100);
					for_print = for_print->next;
					if(for_print == NULL)
						printf("yes\n");
				}
				printf("  (%d)노드 cpu_time : %d, io_time : %d, multi_level : %d, cpu점유율: %f \n\n", for_print->pid - getpid(), for_print->cpu_time, for_print->io_time, for_print->mul_lv, (float)(for_print->cnt_cpu) / global_counter * 100);
			}
			else
				printf("                          EMPTY                      \n\n");

		}
		printf("-------------------------------------------------------\n\n\n");
	}
	printf("\n\n");
}


//io time이 0인 프로세스 waitq에서 runq로 이동
void finish_io()
{
	if(wait_q->head != NULL){
		while(wait_q->head->io_time == 0){
			p_pcb = matching_q(wait_q->head->mul_lv);

			wait_q->head->throuput++;
			wait_q->head->turnaround += wait_q->head->for_turnaround;
		 	wait_q->head->for_turnaround = 0;
			wait_q->head->response_flag = 0;

			push_old_pcb(p_pcb, pop_wait(wait_q), (rand() % 10) + 1);
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
				waiting_time = waiting_time + wtemp->all_wait_time;
				//_________________
				total_throuput += wtemp->throuput;
				total_response += (float)(wtemp->response) / wtemp->throuput;
				total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;

				wtemp = wtemp->next;
			}
		}
		if(runq_empty() != 1){
			for(int i = 1; i < 6; i++){
				p_pcb = matching_q(i);
				wtemp = p_pcb->head;
				if(i == 1){
					while(wtemp != NULL){
						waiting_time = waiting_time + wtemp->all_wait_time;
						//______________
						total_throuput += wtemp->throuput;
						total_response += (float)(wtemp->response) / wtemp->throuput;
						total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;

						wtemp = wtemp->next;
					}
				}
				else{
					if(p_pcb->head != NULL){
						while(wtemp != p_pcb->tail){
							waiting_time = waiting_time + wtemp->all_wait_time;
							//___________________
							total_throuput += wtemp->throuput;
							total_response += (float)(wtemp->response) / wtemp->throuput;
							total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;

							wtemp = wtemp->next;
						}
						waiting_time = waiting_time + wtemp->all_wait_time;
						//_____________________
						total_throuput += wtemp->throuput;
						total_response += (float)(wtemp->response) / wtemp->throuput;
						total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;
					}
				}
			}
		}
		printf("1000 tick이 되어 프로그램을 종료합니다\n");
		printf("waiting time : %.5f\n", waiting_time / 10);
		printf("average response time: %f \n", (float)total_response / MAX_PROC);
		printf("average turnaround time: %f \n", (float)total_turnaround / MAX_PROC);
		printf("total throughput: %d \n", total_throuput);
		pcb* temp;
		temp = (pcb*)malloc(sizeof(pcb));
		for(int j = 1; j < 6; j++){
			p_pcb = matching_q(j);
			if(j == 1){
				while(p_pcb->head != NULL){				
					temp = p_pcb->head;
					p_pcb->head = p_pcb->head->next;
					kill(temp->pid, SIGINT);
				}
			}
			else{
				if(p_pcb->head != NULL){
					while(p_pcb->head != p_pcb->tail){
						temp = p_pcb->head;
						p_pcb->head = p_pcb->head->next;
						kill(temp->pid, SIGINT);
					}
					temp = p_pcb->head;
					kill(temp->pid, SIGINT);
				}
			}
		}	
		exit(0);
	}

}

//5개의 running queue 중에 multi_level에 맞춰 큐 선택
pcb_L* matching_q(int ml)
{
	pcb_L *temp;
	switch(ml){
		case 1 :
			temp = systems;
			break;
		case 2 :
			temp = interactive;
			break;	
		case 3 :
			temp = int_editing;
			break;
		case 4 :
			temp = batch;
			break;
		case 5 :
			temp = student;
			break;
		default :
			printf("오류오류\n");
				
		}
	
	return temp;
}

//running queue가 비었는지 확인(비었으면 1 반환, 안 비었으면 0 반환)
int runq_empty()
{
	if(systems->head == NULL && interactive->head == NULL && int_editing->head == NULL && batch->head == NULL && student->head == NULL)
		return 1;
	else
		return 0;
}

//각 multi level 마다 cpu time 범위 설정
int real_time(int ml, int time)
{
	int real;
	switch(ml)	{
		case 1 :
			real = (time % 3) + 1;
			break;
		case 2 :
			real = (time % 3) + 3;
			break;
		case 3 :
			real = (time % 3) + 4;
			break;
		case 4 :
			real = (time % 3) + 6;
			break;
		case 5 :
			real = (time % 3) + 8;
			break;
		default :
			printf("오류오류\n");
		}	

	return real;
}

//round robin 형식인 2, 3번째 queue의 head와 tail의 위치 재 설정
void r_robin_match()
{
	if(curr->mul_lv == 2 || curr->mul_lv == 3){
		if(curr->quantun_time == 0){
			if(curr->mul_lv == 2 && interactive->head != NULL){
				interactive->head = interactive->head->next;
				interactive->tail = interactive->tail->next;
			}
			else if(curr->mul_lv == 3 && int_editing->head != NULL){
				int_editing->head = int_editing->head->next;
				int_editing->tail = int_editing->tail->next;
			}	
		}
	}
}

//curr의 지정
void curr_match()
{
	if(starvate == 0){	
		curr = systems->head;	
		if(curr == NULL){
			curr = interactive->head;
			if(curr == NULL){
				curr = int_editing->head;
				if(curr == NULL){
					curr = batch->head;
					if(curr == NULL){
						curr = student->head;
					}
				}
			}
		}
	}
	else{
		if(batch->head != NULL && batch->head->wait_time > batch_str){
			starvate++;
			if(starvate >= ba_starvate){
				starvate = 0;
				batch->head->wait_time = 0;
			}
			curr = batch->head;		
		}
		else if(student->head != NULL && student->head->wait_time > student_str){
			starvate++;
			if(starvate >= stu_starvate){
				starvate = 0;
				student->head->wait_time = 0;
			}
			curr = student->head;
		}
		else{
			starvate = 0;
			curr_match();
		}
	}

}


void calcu_response()
{
	if(curr->response_flag == 0){
		curr->response_flag = 1;
		curr->response += curr->for_response;
		curr->for_response = 0;
	}
	curr->for_turnaround++;
	curr->cnt_cpu++;

}
