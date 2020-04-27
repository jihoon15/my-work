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
#include "pop_push_emerged.c"
#include "print.c"
#include "scheduling.c"

//_________헤더파일___________
#define MAX_PROC 10
#define KEY_NUM 2345
#define END_TIME 10000
//____________________________

//_______________free page frame list______________
#define M_MEMORY_INDEX 0x4000   //실제
#define size_submem 0x10000000    //보조 메모리배열의 인덱스, 배열 한 칸당 한페이지로간주한다
#define PAGE_SIZE   0x1000      //4 키로바이트, 페이지의 단위
#define req_num 10              //메모리 요청 개수
#define access_range 0x0fffffff
//********************************************

int MAIN_MEMORY[M_MEMORY_INDEX];//
int SUB_MEMORY[size_submem];	//배열 한칸당 4kb, 한페이지 단위로 취급한다 페이지크기*인덱스의수 = 실제사이
int submem_count;
int count_page;

int exec_time;
int all_completed;
int recive_time;
int global_counter;
int check_first;
float waiting_time = 0;
int run_time = 3;
int total_throuput;
float total_response;
float total_turnaround;
int vm[req_num];

int check_hit_inner = 0;
int check_hit_outer = 0;
int check_empty = 0;
int check_submem = 0;
int check_fault_outer = 0;
int check_fault_inner = 0;

void time_tick(int signo);
void IO_request(int signo);
void do_child();
void CPU_dispatched(int signo);
void end_of_program();
void memory_access_request();
void MMU(int vmpi1, int vmpi2);
void free_to_used(pcb* ptr, int index, int index2);

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
//************************
typedef struct list{
        struct list* next;
        int PFN;//프레임 넘버
        pcb* point_pcb;
        int point_index;
        int point_index2;
}list;
//Free page frame list 와 Used page list의 구현에 사용되는 노드
//***********************
typedef struct ptr{
        list* head;
        list* tail;
}fpf_head;
//Free page frame list와 Used page list의 맨 앞과 맨뒤를 가리킨다
//**********************
void push_fpflist(fpf_head* point, int pfn);
void push_used_list(fpf_head* point, list* newpage);
fpf_head* create_list();
fpf_head* FPF_LIST;
fpf_head* USED_LIST;
void move_used_page(pcb* ptr, int index, int index2);
void page_fault_empty(pcb* ptr, int index, int index2);
void page_fault_submem(pcb* ptr, int index, int index2);
int test = 0;


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
        FPF_LIST = create_list();
        USED_LIST = create_list();

        //SIGALRM
        struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = time_tick;
        sigaction(SIGALRM, &new_sa, &old_sa);

        struct sigaction old_sa_siguser1, new_sa_siguser1;
        memset(&new_sa_siguser1, 0, sizeof(new_sa_siguser1));
        new_sa_siguser1.sa_handler = IO_request;
        sigaction(SIGUSR1, &new_sa_siguser1, &old_sa_siguser1);

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


        for(int i = 0; i < M_MEMORY_INDEX; i++){
                push_fpflist(FPF_LIST, i);
        }

        submem_count = 0; //보조메모리의 인덱스를 가리키는 변수, swap_empty가  일어날 때 이 인덱스로 옮긴다
			    
	count_page = 0;	  //실제메모리가 table에mapping이 되면 값을 넣어준다. swap_empty 때도 값을 넣어준다.
			  //(swap_submem은 보조메모리와 실제메모리의 값이 서로 바뀌어서 그때는 값을 넣지않는다)

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
        printf("____________________________________%d tick____________________________________\n", global_counter - 1);
        if(global_counter != 1)
                calcu_waiting(check_first, global_counter, p_pcb);
        finish_io(wait_q, p_pcb);


        if(p_pcb->head == NULL){
                dec_waitq(wait_q);
                end_of_program();
		test++;
                return;
        }

	memory_access_request();

        if(check_first != 1 && shm_info->exec_cpu != 0){
                curr->cpu_time = shm_info->exec_cpu;
        }

        if(check_first != 1 && shm_info->run_check == 1){
                shm_info->run_check = 0;
                curr = curr->next;
                p_pcb->head = p_pcb->head->next;
                p_pcb->tail = p_pcb->tail->next;
        }

        mmu_ptr = curr;

        check_first = 0;

        dec_waitq(wait_q);

        shm_info->exec_cpu  = curr->cpu_time;

        //runq_print(p_pcb, global_counter);
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


void IO_request(int signo)
{
        curr->cpu_time = shm_info->exec_cpu;
        curr->io_time = real_time2(curr->form, shm_info->recive_io);
        curr = curr->next;
        push_wait(wait_q, pop_pcb(p_pcb));
}


//start child process
void do_child()
{
        struct sigaction old_sa_siguser2, new_sa_siguser2;
        memset(&new_sa_siguser2, 0, sizeof(new_sa_siguser2));
        new_sa_siguser2.sa_handler = CPU_dispatched;
        sigaction(SIGUSR2, &new_sa_siguser2, &old_sa_siguser2);

        while(1){
                sleep(1);
        }
}

void CPU_dispatched(int signo)
{
        shm_info->exec_cpu--;
        exec_time = shm_info->exec_cpu;
        run_time--;

        for(int i = 0; i < req_num; i++){
                vm[i] = (rand() & access_range);
                shm_info->vm[i] = vm[i];
        }
        shm_info->flag = 1;            

        if(exec_time == 0){
                recive_time = (rand() % 10) + 1;
                shm_info->recive_io  = recive_time;
                run_time = 3;
                kill(getppid(), SIGUSR1);
        }
        else if(run_time == 0){
                shm_info->run_check = 1;
                run_time = 3;
        }
}

void end_of_program()
{
        if(global_counter == END_TIME+1){
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
		printf("-----------------------------END------------------------------------------------\n");
                printf("%d tick이 되어 프로그램을 종료합니다\n", END_TIME);

                printf("page_hit  outer page: %d , inner page: %d \n", check_hit_outer, check_hit_inner);
		printf("page_miss_outer page: %d , inner page: %d \n", check_fault_outer, check_fault_inner);
                printf("swap_empty  : %d\n", check_empty);
                printf("swap_submem : %d\n", check_submem);
		printf("Used Sub Memory size : %d \n",submem_count);
		printf("ready queue empty: %d\n", test);

                pcb* temp;
                temp = (pcb*)malloc(sizeof(pcb));
                while(p_pcb->head != p_pcb->tail){
                       temp = p_pcb->head;
                       p_pcb->head = p_pcb->head->next;
                       kill(temp->pid, SIGINT);
                }
                kill(p_pcb->head->pid, SIGINT);
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


void memory_access_request()
{
        if(shm_info->flag == 1){
                int vmpi1;      //vm_page_index1
                int vmpi2;      //vm_page_index2
                int vm_offset;
                int pm;
			printf("___________________________current process(pid) <%d>________________________ \n",mmu_ptr->pid);
                for(int c = 0; c < req_num; c++){
                        vm[c] = shm_info->vm[c];
                        vmpi1 = (vm[c] & 0xFFC00000) >> 22;
                        vmpi2 = (vm[c] & 0x003FF000) >> 12;
                        vm_offset = (vm[c] & 0x00000FFF);
			printf("%d'th access--------------------------------------------------------------------\n", c + 1);

                        MMU(vmpi1, vmpi2);   

                        printf("|va           1Lv index         2Lv index   ->    PFN        pa      |\n");
                        pm = (mmu_ptr->page_t->page_t2[vmpi1]->page_fn[vmpi2] << 12) + vm_offset;
                        printf("|%08x     %08x          %08x          %08x   %08x|\n",vm[c] , vmpi1, vmpi2, mmu_ptr->page_t->page_t2[vmpi1]->page_fn[vmpi2], pm);
			printf("\n");
                }
                shm_info->flag = 0;     //다 수행 후 plag 0으로
        }

}


void MMU(int vmpi1, int vmpi2)
{

        if(FPF_LIST->head != NULL){     
	//Free page frame list 가 남아있을때
                if(mmu_ptr->page_t->valid[vmpi1] == 0){ 
		//page table1에서 mapping이 안 되어있을 때-> page table2 생성후 이어주고, Free page frame을 주어 page table2를 채운다.
			printf("Page Fault at outer page table[%x]: Not in Main Memory\n",vmpi1);
                        pt2 *newpt2 = (pt2*)malloc(sizeof(pt2));
                        mmu_ptr->page_t->page_t2[vmpi1] = newpt2;
			printf("Inner page table is created (Outer page table[%x])\n", vmpi1);
			printf("Page Fault at inner page table[%x]: Not in Main Memory\n", vmpi2);
                        mmu_ptr->page_t->page_t2[vmpi1]->page_fn[vmpi2] = FPF_LIST->head->PFN;
                        MAIN_MEMORY[FPF_LIST->head->PFN] = count_page;//실제메모리가 할당 받았다는 것을 표현해준다. 값이 들어감
                        count_page++;
                        mmu_ptr->page_t->valid[vmpi1] = 1;
                        mmu_ptr->page_t->page_t2[vmpi1]->valid[vmpi2] = 1;

			check_fault_outer++;

			check_fault_inner++;
                        free_to_used(mmu_ptr, vmpi1, vmpi2);//Free page frame list -> Used page list
			printf("Inner page table[%x] is updated\n", vmpi2);
                }
                else{   
		//page table1에서mapping이 되어 있을 때 -> page table2를 보면된다
		check_hit_outer++;

                        if(mmu_ptr->page_t->page_t2[vmpi1]->valid[vmpi2] == 0){ 
			//page table2에서 mapping이 안 되어 있을 때-> Free page frame 을 받아서 page table2를 채운다.
				printf("Page Fault at inner page table[%x]: Not in Main Memory\n", vmpi2);
				
                                mmu_ptr->page_t->page_t2[vmpi1]->page_fn[vmpi2] = FPF_LIST->head->PFN;
                                MAIN_MEMORY[FPF_LIST->head->PFN] = count_page;
                                count_page++;
                                mmu_ptr->page_t->page_t2[vmpi1]->valid[vmpi2] = 1;
				check_fault_inner++;
                                free_to_used(mmu_ptr, vmpi1, vmpi2);
				printf("Inner page[%x] is updated\n", vmpi2);
                        }
                        else{
			//page table2에서 mapping이 되어 있을 때-> 그대로사용!
				printf("Page hit!\n");    
                                move_used_page(mmu_ptr, vmpi1, vmpi2);
			}
                }
        }

        else{   
	//Free page frame list가 없을때 -> swap 이 일어난다.
                if(mmu_ptr->page_t->valid[vmpi1] == 0){ 
		//page table1 에서 mapping이 안되어 있을 때-> page table2를 생성하고 page_fault_empty(보조메모리에도없음)을 실행하여 Free page를 받고  table2를 채워줌
			printf("Page Fault at outer page %x: Not in Main Memory\n",vmpi1);
                        pt2 *newpt2 = (pt2*)malloc(sizeof(pt2));
                        mmu_ptr->page_t->page_t2[vmpi1] = newpt2;
			//생성하고 fault handle
			printf("Inner page table is created (Outer page table[%x])\n", vmpi1);
			printf("Page Fault at inner page table[%x]: Not in Main Memory\n", vmpi2);
			check_fault_outer++;
			check_fault_inner++;
                        page_fault_empty(mmu_ptr, vmpi1, vmpi2);
			printf("Inner page table[%x] is updated\n", vmpi2);
                }
                else{   
		//page table1의 에서 mapping이 되어 있을 때 -> page table2를 본다.
		check_hit_outer++;
                        if(mmu_ptr->page_t->page_t2[vmpi1]->valid[vmpi2] == 0){ 
			//page table2에서 mapping이 안 되어 있을 때-> page_fault_empty(보조메모리에도없음)을 실행하여 Free page받고 table2 채워줌
			printf("Page Fault at inner page table[%x]: Not in Main Memory\n", vmpi2);
				check_fault_inner++;
                                page_fault_empty(mmu_ptr, vmpi1, vmpi2);
			printf("Inner page table[%x] is updated\n", vmpi2);
                        }
                        else{
			//page table2에서 mapping이 되어 있을 때 -> 실제메모리에 있나, 보조메모리에 있나 확인
                                if(mmu_ptr->page_t->page_t2[vmpi1]->residence[vmpi2] == 1){      
				//보조메모리에 존재할 때 ->  page_fault_submem 실행하여 주메모리로 가져온후에 table2를 업데이트
					printf("Page Fault at inner page table[%x]: Not in Main Memory\n", vmpi2);
					check_fault_inner++;
                                        page_fault_submem(mmu_ptr, vmpi1, vmpi2);
					printf("Inner page table[%x] is updated\n", vmpi2);
				}
                                else{    
				//실제메모리에 존재할 때 -> 그대로 사
                                        move_used_page(mmu_ptr, vmpi1, vmpi2);
					printf("Page hit!\n");
				}
                        }
                }
        }

}
//빠끄~
void free_to_used(pcb* ptr, int index, int index2){

        FPF_LIST->head->point_pcb = ptr;
        FPF_LIST->head->point_index = index;
        FPF_LIST->head->point_index2 = index2;
	//노드의값 변경->swap 시에 사용하기위해서

        list* temp = FPF_LIST->head;
	//push 해주기 위해 저장
	
        if(FPF_LIST->head->next == NULL){
                FPF_LIST->head = NULL;
                FPF_LIST->tail = NULL;
        }
        else{
                FPF_LIST->head = FPF_LIST->head->next;
        }
	//사용한 Free page frame 제외시켜준다 

        push_used_list(USED_LIST, temp);//push

}

void push_used_list(fpf_head* point, list* newpage){
//Used list 에 push 해준다.
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

void move_used_page(pcb* ptr, int index, int index2)
{
//Page hit 시에 사용된 페이지를 Used list의 맨뒤로 옮겨줌 -> swap할때 걸러주는 기준이된
        list* temp;
        list* for_save;
        temp = USED_LIST->head;

        if(temp->next == NULL){
                return;
	}//하나만 있을때 그냥 리턴함
        if(temp->PFN == ptr->page_t->page_t2[index]->page_fn[index2]){
                USED_LIST->head = USED_LIST->head->next;
                push_used_list(USED_LIST, temp);
        //맨앞에있을시에
        }
        else{
                while(temp->next->PFN != ptr->page_t->page_t2[index]->page_fn[index2]){
                        temp = temp->next;
                }
                //바꿀거 바로직전에 가리키는중
                for_save = temp->next;
                temp->next = temp->next->next;
                push_used_list(USED_LIST,for_save);
        }

        check_hit_inner++;

}
//Page Fault : 보조메모리와 실제메모리에 둘다 존재 하지 않을 
void page_fault_empty(pcb* ptr, int index, int index2)//page fault가 발생한 프로세스와 가상주소가 parameter로 받는다
{
        list* temp_list;
        pcb* lru_pcb;   //
        int page_index; //
        int page_index2;//보조메모리로 보내야할 프로세스와 page index를 받는다

        int main_pfn;   //실제메모리가 스왑되는 걸 보이기위해 배열안에 값이 저장되있는데 그걸 옮기기위한 index 저장

        temp_list = USED_LIST->head;		//맨앞의 노드를 받는다
        USED_LIST->head = USED_LIST->head->next;//사용된 노드(맨앞)는 나중에 뒤로 push 됨
        temp_list->next = NULL;

        page_index = temp_list->point_index;  //
        page_index2 = temp_list->point_index2;//
        lru_pcb = temp_list->point_pcb;	      //보조메모리로 보내는데 필요한 정보들 받는다


        main_pfn = lru_pcb->page_t->page_t2[page_index]->page_fn[page_index2];//실제주소의 PFN받는다

        lru_pcb->page_t->page_t2[page_index]->residence[page_index2] = 1;//
        lru_pcb->page_t->page_t2[page_index]->valid[page_index2] = 1;    //보조메모리에 존재한다고 표시

        lru_pcb->page_t->page_t2[page_index]->page_fn[page_index2] = submem_count;//보조메모리의 어디에 존재하는지 table2를 업데이트해준다

        SUB_MEMORY[submem_count] = MAIN_MEMORY[main_pfn];//보조메모리로 값(실제메모리가 할당이 될때 값을 준다)을 옮긴다
        submem_count++;

        ptr->page_t->valid[index] = 1;				//
        ptr->page_t->page_t2[index]->valid[index2] = 1;		//
        ptr->page_t->page_t2[index]->page_fn[index2] = main_pfn;//pagefault가 발생한 table2을 업데이트함
        count_page++;
        MAIN_MEMORY[main_pfn] = count_page;//보조메모리에서 가져온게아님, 값 새로줘야함

        temp_list->point_index = index;		//
        temp_list->point_index2 = index2;	//
        temp_list->point_pcb = ptr;		//
						
        push_used_list(USED_LIST, temp_list);	//temp값을 새롭게 변경-> Used list의 맨뒤로 보낸다.


        check_empty++;//swap_empty가 일어난걸 확인할 변수
	printf("Swap event(empty)\n");
	printf("<%d>Inner page table[%x] is changed: page data = %d move: main[%x~%x] -> sub[%x~%x]\n", lru_pcb->pid, page_index2, SUB_MEMORY[submem_count - 1], main_pfn, (main_pfn << 12) + 0xfff, submem_count-1, ((submem_count-1) << 12) + 0xfff);
	printf("<%d>Inner page table[%x] is changed: page data = %d move: empty -> main[%x~%x]\n", ptr->pid, index2, count_page , main_pfn,(main_pfn << 12) + 0xfff );

}
//Page Fault : 보조메모리에 존재할 때  
void page_fault_submem(pcb* ptr, int index, int index2)// page fault가 발생한 프로세스와 가상주소를 parameter로 받는다
{
        int submem_pfn;
        int submem_data;
        submem_pfn = ptr->page_t->page_t2[index]->page_fn[index2];//page fault가 일어난 table2의 보조메모리pfn
        submem_data = SUB_MEMORY[submem_pfn];			  //보조메모리의 데이터를 저장

        list* temp_list;
        pcb* lru_pcb;
        int main_pfn;    //
        int page_index;  //
        int page_index2; //보조메모리로 보내야할 프로세스와 page index를 받는다

        int main_data;   //메인메모리의 값을 저장

        temp_list = USED_LIST->head;		//Used page list의 맨앞 노드를 받는다
        USED_LIST->head = USED_LIST->head->next;//사용된 노드는 나중에 맨뒤로 push되므로 head이동
        temp_list->next = NULL;			//

        page_index = temp_list->point_index;  //
        page_index2 = temp_list->point_index2;//
        lru_pcb = temp_list->point_pcb;	      //보조메모리로 보내는데 필요한 정보를 받는다

        main_pfn = lru_pcb->page_t->page_t2[page_index]->page_fn[page_index2];//
        main_data = MAIN_MEMORY[main_pfn];				      //주메모리의 주소와 데이터를 받는다


        lru_pcb->page_t->page_t2[page_index]->residence[page_index2] = 1;	//
        lru_pcb->page_t->page_t2[page_index]->valid[page_index2] = 1;	 	//보조메모리에 존재한다고 표시
        lru_pcb->page_t->page_t2[page_index]->page_fn[page_index2] = submem_count;//보조메모리의 어디에 존재하는지 table2를 업데이트해준다
        SUB_MEMORY[submem_count] = main_data;//보조메모리로 값을 옮긴다
	submem_count++;

        ptr->page_t->page_t2[index]->valid[index2] = 1;		//
        ptr->page_t->page_t2[index]->residence[index2] = 0;	//
        ptr->page_t->page_t2[index]->page_fn[index2] = main_pfn;//page fault가 발생한 table2를 업데이트한다

        MAIN_MEMORY[main_pfn] = submem_data;//swap하는 보조메모리의 데이터를 주메모리로 보낸다

        temp_list->point_index = index;		//
        temp_list->point_index2 = index2;	//
        temp_list->point_pcb = ptr;		//

        push_used_list(USED_LIST, temp_list);	//temp의 값을 새롭게변경 -> Used list의 맨뒤로 push

        check_submem++;//swap_submem이 일어난걸 확인할 변수
	printf("Swap event(sub memory)\n");
	printf("<%d>Inner page table[%x] is changed: page data = %d move: main[%x~%x] -> sub[%x~%x]\n", lru_pcb->pid, page_index2, main_data,main_pfn, (main_pfn << 12) + 0xfff, (submem_count - 1), ((submem_count - 1) << 12) + 0xfff);
	printf("<%d>Inner page table[%x] is changed: page data = %d move: sub[%x~%x] -> main[%x~%X]\n", ptr->pid, index2, submem_pfn, submem_data,(submem_pfn << 12) + 0xfff, main_pfn, (main_pfn << 12) + 0xfff);
}
