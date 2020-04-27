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
#include "pop_push_copyonwrite.c"
#include "print.c"
#include "scheduling.c"

//_________헤더파일___________
#define MAX_PROC 10
#define KEY_NUM 1235
#define MEM_SIZE 100
//____________________________

//_______________free page frame list______________
#define PAGE_SIZE   0x1000      //4 키로바이트
#define req_num 10              //메모리 요청 개수
#define END_TIME 10000
#define M_MEMORY_INDEX 0x100000
int MAIN_MEMORY[M_MEMORY_INDEX];
//페이지당 4mb
//int P_MEMORY[MEMORY_SIZE/4]; //400kb 나누기 4할지말지 정해야
//********************************************
int page_hit = 0;
int page_miss = 0;

int all_completed;
int recive_time;
int global_counter;
int check_first;
float waiting_time = 0;
int total_throuput;
float total_response;
float total_turnaround;
int vm[req_num];

void time_tick(int signo);
void IO_request();
void do_child();
void CPU_dispatched();
void end_of_program();
void memory_access_request();

typedef struct sh_mem{
        int exec_cpu;
        int recive_io;
        int run_check;
        int vm[req_num];
        int flag;
}sh;

pcb *curr;
pcb *mmu_ptr;
pcb_L *p_pcb;
pcb_L *wait_q;
sh* shm_info = NULL;

//____________________________________________
//*************이번에 추가된 변수들***********
typedef struct list{
        struct list* next;
        int PFN;//프레임 넘버
}list;

typedef struct ptr{
 list* head;
 list* tail;
}fpf_head;

void push_fpflist(fpf_head* point, int pfn);
fpf_head* create_list();
fpf_head* FPF_HEAD;
int count_page;
int process_num;
#define FOR_FORK  600
#define FOR_WRITE 3
int num_for_fork;
int num_for_write;
int num_for_read_only = 0;

int copy_num;
int os_pid;

void FORK(pcb* parent);
void push_pcb_fork(pcb_L * L,pcb* parent);
void make_read_only(pcb* parent);
void copy_on_write(pcb* ptr, int index);
int write_num;
int give_num_write;
int test = 0;
void MMU(int vm_idex);
int main(int argc, char *arg[])
{
	give_num_write = 10000;
        check_first = 1;
        srand(time(NULL));
        all_completed = 0;
        int cpu_time;
        int shmid;
        int pid;
        int status;
        void* address = (void*)0;

	os_pid=getpid();
	count_page = 0;
	num_for_fork = 0;
	num_for_write = 0;
	copy_num = 0;

        if((shmid = shmget(KEY_NUM, 100, IPC_CREAT | 0666)) != (-1)){
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
        shm_info->flag = 0;

        p_pcb = create_pcb();
        wait_q = create_pcb();
        FPF_HEAD = create_list();

        //SIGALRM
        struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = time_tick;
        sigaction(SIGALRM, &new_sa, &old_sa);
        //time_tick 함수로 이동

        push_pcb(p_pcb, os_pid);//한번포크한거임

	process_num=1; //한개로시작
//엥간하면 여기에 생성해서 부모프로세스만 가지고있게하기//
        for(int i = 0; i < M_MEMORY_INDEX; i++){
                push_fpflist(FPF_HEAD, i);
        }
//전역변수로 free page frame list 선언 여기서 100개 푸쉬?//



        struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
        new_timer.it_interval.tv_sec = 0;       //몇초 간격으로 알람할지
        new_timer.it_interval.tv_usec = 10000;
        new_timer.it_value.tv_sec = 1;          //처음 알람 시작을 몇초 뒤에 할지
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
	num_for_fork++;
	num_for_write++;
        printf("--------------------------%d tick-------------------------\n", global_counter - 1);
        if(global_counter != 1){
                calcu_waiting(check_first, global_counter, p_pcb);
        finish_io(wait_q, p_pcb);
	}

   
        if(p_pcb->head == NULL){
		test++;
                dec_waitq(wait_q);
                end_of_program();
                return;
        }
	memory_access_request();

        if(check_first != 1 && shm_info->run_check == 1){
                shm_info->run_check = 0;
                curr = curr->next;
                p_pcb->head = p_pcb->head->next;
                p_pcb->tail = p_pcb->tail->next;
        }

        mmu_ptr = curr;

        check_first = 0;

        dec_waitq(wait_q);

        shm_info->exec_cpu = curr->cpu_time;

        end_of_program();

        if(curr->response_flag == 0){
                curr->response_flag = 1;
                curr->response += curr->for_response;
                curr->for_response = 0;
        }

        curr->cnt_cpu++;
        curr->for_turnaround++;
        CPU_dispatched(curr);


}
//curr->pid(자식프로세스)로 SIGUSR2을 보냄(do_child 함수에서 받음)


//when SIGUSR1 occur
void IO_request()
{
        curr->cpu_time = 5;
        curr->io_time = 2;
        curr = curr->next;
        push_wait(wait_q, pop_pcb(p_pcb));
}




//when SIGUSR2 occur
void CPU_dispatched()
{
        curr->cpu_time--;
       	curr->run_time--;

        for(int i = 0; i < req_num; i++){
                vm[i] = (rand() & 0xffffffff);
                shm_info->vm[i] = vm[i];
        }
        shm_info->flag = 1;             //flag on 자식 부모.

        if(curr->cpu_time == 0){
                
                shm_info->recive_io  = recive_time;
                curr->run_time = 3;
                IO_request();
        }
        else if(curr->run_time == 0){
                shm_info->run_check = 1;
                curr->run_time = 3;
        }
}

//1000 tick이 끝날을 때
void end_of_program()
{
        if(global_counter == END_TIME + 1){
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
                        int one = 0;
                        while(wtemp != p_pcb->head || one == 0){
                                waiting_time += wtemp->all_wait_time;
                                total_throuput += wtemp->throuput;
                                total_response += (float)(wtemp->response) / wtemp->throuput;
                                total_turnaround += (float)(wtemp->turnaround) / wtemp->throuput;
                                wtemp = wtemp->next;
                                one = 1;
                        }
                }
		printf("----------------------END----------------------------------\n");
                printf("5000 tick이 되어 프로그램을 종료합니다\n");
		printf("Page hit: %d\n",page_hit);
		printf("Page miss: %d\n",page_miss);
		printf("copy-on-write happen : %d times \n",copy_num);
		printf("write event happen : %d times\n",write_num);
		printf("ready queue empty: %d\n",test);
                exit(0);
        }

}


//____________추가된 함수들____________
//*************************************
void push_fpflist(fpf_head* point, int pfn)//맨뒤에넣음
{
        list* newpage;
        newpage = (list* )malloc(sizeof(list));
        newpage->PFN = pfn;
        newpage->next = NULL;

        if(point->head==NULL){
                point->head = newpage;
                point->tail = newpage;
        }
        else{
                point->tail->next = newpage;
                point->tail = newpage;
        }
}

fpf_head* create_list(){
        fpf_head* L;
        L = (fpf_head*)malloc(sizeof(fpf_head));
        L->head = NULL;
        L->tail = NULL;
        return L;
}


void memory_access_request()//부모에서하는거임
{
        if(shm_info->flag == 1){
 	       	int vm_page_index;
               	int vm_offset;
		int pm;
		printf("________________current process(pid) <%d>______________ \n",mmu_ptr->pid);

               	for(int c = 0; c < req_num; c++){
                        vm[c] = shm_info->vm[c];
                        vm_page_index = (vm[c] & 0xFFFFF000) >> 12;
                        vm_offset = (vm[c] & 0x00000FFF);
			printf("%d'th access-----------------------------------------------\n", c+1);

			MMU(vm_page_index);
			
		  	printf("|va            page_index   ->   PFN              pa      |\n");
                        pm = (mmu_ptr->page_t->pagetable[vm_page_index] << 12) + vm_offset;
			printf("|%08x      %08x          %08x         %08x|\n",vm[c] , vm_page_index,mmu_ptr->page_t->pagetable[vm_page_index] ,pm);
	
                        printf("\n");
                }
                shm_info->flag = 0;     //다 수행 후 plag 0으로
        }
	if(process_num < MAX_PROC){
	FORK(mmu_ptr);
	}
}
void MMU(int vm_index){
	if(num_for_write > FOR_WRITE){//write이벤트 발생!~!!!!!!!!!!!!
		  num_for_write = 0;
		  copy_on_write(mmu_ptr,vm_index);
		  }
		  else{
                        if(mmu_ptr->page_t->valid[vm_index] == 0){
			//write가 일어나지않아서 readonly가 1이든 0이든 그냥 출력하면 된다
                                mmu_ptr->page_t->pagetable[vm_index] = FPF_HEAD->head->PFN;
				MAIN_MEMORY[FPF_HEAD->head->PFN] = count_page;
				count_page++;
				page_miss++;
                                //프리페이지를 준다.
                                if(FPF_HEAD->head->next == NULL){
                                        FPF_HEAD->head = NULL;
                                        FPF_HEAD->tail = NULL;
                                }
                                else{
                                        FPF_HEAD->head = FPF_HEAD->head->next;
                                }
                                mmu_ptr->page_t->valid[vm_index]=1;
                        }
			else{
			page_hit++;
			}
		  }

}
void FORK(pcb* parent){
	if(num_for_fork > FOR_FORK)
	{
		num_for_fork = 0;
		process_num++;
		push_pcb_fork(p_pcb,parent);
	}
}
	
void push_pcb_fork(pcb_L *L, pcb* parent) //프로세스 생성 시 run,ready q에 push
{
        pcb *newpcb;
        pcb *temp;
        pt *newpt;
        newpcb = (pcb*)malloc(sizeof(pcb));
        newpt = (pt*)malloc(sizeof(pt));
        newpcb->pid = os_pid + process_num ;//pid 다르게
        newpcb->cpu_time = 5;
	newpcb->run_time = 3;
	newpcb->io_time = 2;
        newpcb->all_wait_time = 0;
        newpcb->wait_time = 0;
        newpcb->next = NULL;
	//페이지테이블 복사한뒤에 연결
	make_read_only(parent);
	//테이블의 매핑되있는부분들 전부 리드온리비트 1로바꿈
	memcpy(newpt, parent->page_t, sizeof(struct page_table));//둘다 pt 구조체 가리키는 포인터
        newpcb->page_t = newpt;

	

        if(L->head == NULL){
                L->head = newpcb;
                newpcb->next = newpcb;
                L->tail = newpcb;
                return;
        }
        else{
                L->tail->next = newpcb;
                L->tail = newpcb;
                newpcb->next = L->head;
        }
	printf("<%d> is forked from <%d> ... \n",newpcb->pid ,parent->pid);
	printf("valid table entry num = saved page number: %d\n ", num_for_read_only);
	num_for_read_only = 0;

}

void make_read_only(pcb* parent){
	for(int i = 0; i < 0x100000; i++){
		if(parent->page_t->valid[i]==1){//매핑되있는친구들만
		parent->page_t->read_only[i] = 1;//리드온리비트 1로바꿈
		num_for_read_only++;
		}
	}
}
void copy_on_write(pcb* ptr, int index){//공유되는 메모리일경우, 따로 페이지받고 값바꾸고 레지던스0으
			write_num++;
                        if(ptr->page_t->valid[index] == 0){
				page_miss++;
                                ptr->page_t->pagetable[index] = FPF_HEAD->head->PFN;
				MAIN_MEMORY[FPF_HEAD->head->PFN] = give_num_write;//write
				give_num_write++;
				count_page++;
                                //프리페이지를 준다.
                                if(FPF_HEAD->head->next == NULL){
                                        FPF_HEAD->head = NULL;
                                        FPF_HEAD->tail = NULL;
                                }
                                else{
                                        FPF_HEAD->head = FPF_HEAD->head->next;
                                }
                                ptr->page_t->valid[index]=1;
                        }
			else{//매핑이 되어있을때
				page_hit++;
				if(ptr->page_t->read_only[index]==1){
				//리드온리비트가1 즉 공유되는 메모리일경우에
				//************write event**********************
				printf("Copy-on-Write event\n");
				printf("origin PFN, page data: %x , %d ->", ptr->page_t->pagetable[index],MAIN_MEMORY[ptr->page_t->pagetable[index]]);
				ptr->page_t->pagetable[index] = FPF_HEAD->head->PFN;

				if(FPF_HEAD->head->next == NULL){
                                        FPF_HEAD->head = NULL;
                                        FPF_HEAD->tail = NULL;
                                }
                                else{
                                        FPF_HEAD->head = FPF_HEAD->head->next;
                                }

				ptr->page_t->read_only[index] = 0;
				MAIN_MEMORY[ptr->page_t->pagetable[index]] = give_num_write;//
				printf(" changed PFN, page data: %x , %d\n", ptr->page_t->pagetable[index],MAIN_MEMORY[ptr->page_t->pagetable[index]]);
				give_num_write++;
				copy_num++;//리드온리부분을 건들이는경우에만
				}
				else{
				printf("Write event\n");
				printf("page index: %x",index);
				printf("origin data: %d -> changed data: %d\n", MAIN_MEMORY[ptr->page_t->pagetable[index]], give_num_write);
				MAIN_MEMORY[ptr->page_t->pagetable[index]] = give_num_write;//
				give_num_write++;
				//공유되는 부분이 아닐 경우에
				}
			}
}
