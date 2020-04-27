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
#include "pop_push.h"
#include "scheduling.h"
#include "fs.h"

#define MAX_PROC 3
#define KEY_NUM 5923
#define MEM_SIZE 8
#define END_TIME 100
FILE* fp;
struct META_DATA* MDP;
struct dentry* dp;

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
int opened_file[100] = {0, };
int open_file_num = 0;
int check_first_change[100] = {0, };
int write_ing[100] = {0, };

int open_count = 0;
int close_count = 0;
int hit_in_pcb = 0;
int file_find_fail = 0;
int access_denied = 0;
int write_file = 0;
int hit_N_noting = 0;


void time_tick(int signo);
void child_completed(int signo);
void do_child();
void child_sighandler(int signo);
void end_of_program();
void mount_META_DATA();
void work(int signo);
void open_file();
void give_data();
void receive_data(int signo);
void write_data();
void print_inode_info();
void print_openfile();
void opened_zero(int signo);


typedef struct sh_mem{
        int exec_cpu;
        int recive_io;
        int run_check;
        char operation;
        char file_name[16];
        char mode;
        int file_n;
        int data[BLOCK_SIZE * 6];
}sh;

char edit_data[6] = {'c', 'h', 'a', 'n', 'g', 'e'};


pcb *curr;
pcb *hold_curr;
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



        //SIGUSR1-부모
        struct sigaction old_sa_siguser1, new_sa_siguser1;
        memset(&new_sa_siguser1, 0, sizeof(new_sa_siguser1));
        new_sa_siguser1.sa_handler = child_completed;
        sigaction(SIGUSR1, &new_sa_siguser1, &old_sa_siguser1);
        //child_completed 함수로 이동



        //SIGUSR2-부모
        struct sigaction old_sa_siguser2, new_sa_siguser2;
        memset(&new_sa_siguser2, 0, sizeof(new_sa_siguser2));
        new_sa_siguser2.sa_handler = work;
        sigaction(SIGABRT, &new_sa_siguser2, &old_sa_siguser2);
        //work 함수로 이동




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


        mount_META_DATA();


        struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
        new_timer.it_interval.tv_sec = 0;       //몇초 간격으로 알람할지
        new_timer.it_interval.tv_usec = 50000;
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

        int i;

        for(i = 0; i < BLOCK_SIZE; i++){
                shm_info->file_name[i] = '\0';
        }

        shm_info->file_name[0] = 'f';
        shm_info->file_name[1] = 'i';
        shm_info->file_name[2] = 'l';
        shm_info->file_name[3] = 'e';
        shm_info->file_name[4] = '_';

        printf("\n\n\n");
        printf("_____________<%d tick>______________\n", global_counter - 1);

        print_openfile();

        if(global_counter != 1)
                calcu_waiting(check_first, global_counter, p_pcb);
        finish_io(wait_q, p_pcb);

        if(p_pcb->head == NULL){
                dec_waitq(wait_q);
                end_of_program();
                return;
        }

        if(check_first != 1 && shm_info->exec_cpu != 0){
                curr->cpu_time = shm_info->exec_cpu;
        }

        if(check_first != 1 && shm_info->run_check == 1){
                shm_info->run_check = 0;
                curr = curr->next;
                p_pcb->head = p_pcb->head->next;
                p_pcb->tail = p_pcb->tail->next;
        }

        check_first = 0;

        dec_waitq(wait_q);

        shm_info->exec_cpu  = curr->cpu_time;

        end_of_program();

        if(curr->response_flag == 0){
                curr->response_flag = 1;
                curr->response += curr->for_response;
                curr->for_response = 0;
        }

        curr->cnt_cpu++;
        curr->for_turnaround++;

        hold_curr = curr;
        printf("current pid : %d \n", curr->pid);

        kill(curr->pid, SIGUSR2);


}
//curr->pid(자식프로세스)로 SIGUSR2을 보냄(do_child 함수에서 받음)


//when SIGUSR1 occur
void child_completed(int signo)
{
        curr->cpu_time = shm_info->exec_cpu;
        curr->io_time = real_time2(curr->form, shm_info->recive_io);
        curr = curr->next;
        push_wait(wait_q, pop_pcb(p_pcb));
}


//start child process
void do_child()
{
        //SIGUSR2-자식
        struct sigaction old_sa_siguser2c, new_sa_siguser2c;
        memset(&new_sa_siguser2c, 0, sizeof(new_sa_siguser2c));
        new_sa_siguser2c.sa_handler = child_sighandler;
        sigaction(SIGUSR2, &new_sa_siguser2c, &old_sa_siguser2c);

        //SIGHUP-자식
        struct sigaction old_sa_siguser1c, new_sa_siguser1c;
        memset(&new_sa_siguser1c, 0, sizeof(new_sa_siguser1c));
        new_sa_siguser1c.sa_handler = receive_data;
        sigaction(SIGHUP, &new_sa_siguser1c, &old_sa_siguser1c);

        //SIGFPE-자식
        struct sigaction old_sa_siguser3c, new_sa_siguser3c;
        memset(&new_sa_siguser3c, 0, sizeof(new_sa_siguser3c));
        new_sa_siguser3c.sa_handler = opened_zero;
        sigaction(SIGFPE, &new_sa_siguser3c, &old_sa_siguser3c);




        while(1){
                sleep(1);
        }
}


//when SIGUSR2 occur
void child_sighandler(int signo)
{
        int operation_n;
        int file_n;
        int mode_n;
        char file_NUM[3];
        int i;

        int randomly;

        operation_n = (rand() % 10);
        file_n = (rand() % 10) + 1;
        mode_n = (rand() % 10);


        if(operation_n <= 6 || open_file_num == 0){
                if(opened_file[file_n - 1] != 1)
                        open_file_num++;
                opened_file[file_n - 1] = 1;
                shm_info->operation = 'O';
        }
        else{
                if(opened_file[file_n - 1] != 1){
                        randomly = (rand() % open_file_num) + 1;
                        file_n = -1;
                        while(randomly != 0){
                                file_n++;
                                if(opened_file[file_n - 1] == 1)
                                        randomly--;
                                if(file_n == 120)
                                        break;
                        }
                }
                open_file_num--;
                opened_file[file_n - 1] = 0;
                shm_info->operation = 'C';
        }
        if(file_n > 100)
                shm_info->operation = 'O';

        shm_info->file_n = file_n;
        sprintf(file_NUM , "%d", file_n);
        strcat(shm_info->file_name, file_NUM);

        if(mode_n <= 6)
                shm_info->mode = 'r';
        else{
                shm_info->mode = 'w';   
        }
        printf("-----------------------------------\n");
        printf("operation(Open/Close) : %c\n", shm_info->operation);
        printf("modei(Read/Write)     : %c\n", shm_info->mode);
        printf("file_name             : %s\n\n", shm_info->file_name);

        kill(getppid(), SIGABRT);

        shm_info->exec_cpu--;
        exec_time = shm_info->exec_cpu;
        run_time--;
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


//100 tick이 끝날을 때
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
		printf("\n\n\n");
                printf("%d tick이 되어 프로그램을 종료합니다\n", END_TIME);
		printf("----------스케줄링 관련 출력값----------\n");
                printf("waiting time : %.5f\n", waiting_time / 10);
                printf("average response time: %f \n", (float)total_response / MAX_PROC);
                printf("average turnaround time: %f \n", (float)total_turnaround / MAX_PROC);
                printf("total throughput: %d \n", total_throuput);
		printf("----------------------------------------\n");
		printf("---------파일시스템 관련 출력값---------\n");
		printf("open count : %d\n", open_count);
		printf("close count : %d\n", close_count);
		printf("hit in pcb : %d\n", hit_in_pcb);
		printf("file find fail : %d\n", file_find_fail);
		printf("access denied : %d\n", access_denied);
		printf("write file : %d\n", write_file);
		printf("hit N noting : %d\n", hit_N_noting);
		printf("----------------------------------------\n");
		

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


void mount_META_DATA()
{

        MDP = (struct META_DATA*)malloc(sizeof(struct META_DATA));
        fp = fopen("disk.img", "r+");

        if(fp == NULL){
                printf("파일을 열수 없습니다.\n");
                exit(0);
        }

        fread(&MDP->s.partition_type, sizeof(int), 1, fp);
        fread(&MDP->s.block_size, sizeof(int), 1, fp);
        fread(&MDP->s.inode_size, sizeof(int), 1, fp);
        fread(&MDP->s.first_inode, sizeof(int), 1, fp);

        fread(&MDP->s.num_inodes, sizeof(int), 1, fp);
        fread(&MDP->s.num_inode_blocks, sizeof(int), 1, fp);
        fread(&MDP->s.num_free_inodes, sizeof(int), 1, fp);

        fread(&MDP->s.num_blocks, sizeof(int), 1, fp);
        fread(&MDP->s.num_free_blocks, sizeof(int), 1, fp);
        fread(&MDP->s.first_data_block, sizeof(int), 1, fp);
        fread(&MDP->s.volume_name, sizeof(char), 24, fp);
        fread(&MDP->s.padding, sizeof(char), 960, fp);
        //SUPER block print
        printf("-------------super block information-------------\n");
        printf("partition_type          : %d\n", MDP->s.partition_type);
        printf("block_size              : %d\n", MDP->s.block_size);
        printf("inode_zise              : %d\n", MDP->s.inode_size);
        printf("inode of directory file : %d\n", MDP->s.first_inode);
        printf("num_inodes              : %d\n", MDP->s.num_inodes);
        printf("num_inode_blocks        : %d\n", MDP->s.num_inode_blocks);
        printf("num_free_inodes         : %d\n", MDP->s.num_free_inodes);
        printf("num_blocks              : %d\n", MDP->s.num_blocks);
        printf("num_free_block          : %d\n", MDP->s.num_free_blocks);
        printf("first_data_block        : %d\n", MDP->s.first_data_block);
        printf("volume_name             : %s\n", MDP->s.volume_name);
        printf("padding                 : %s\n", MDP->s.padding);
        printf("-------------------------------------------------\n");
        printf("\n");

        //inode print
        for(int k = 0; k < 224; k++){

                fread(&MDP->inode_table[k].mode, sizeof(int), 1, fp);
                fread(&MDP->inode_table[k].locked, sizeof(int), 1, fp);
                fread(&MDP->inode_table[k].date, sizeof(int), 1, fp);
                fread(&MDP->inode_table[k].size, sizeof(int), 1, fp);
                fread(&MDP->inode_table[k].indirect_block, sizeof(int), 1, fp);
                fread(&MDP->inode_table[k].blocks, sizeof(short), 6, fp);

                printf("----%dth inode information----\n", k);
                printf("mode            : %d\n", MDP->inode_table[k].mode);
                printf("locked          : %d\n", MDP->inode_table[k].locked);
                printf("date            : %d\n", MDP->inode_table[k].date);
                printf("size            : %d\n", MDP->inode_table[k].size);
                printf("indirect_block  : %d\n", MDP->inode_table[k].indirect_block);
                printf("blocks          : ");
                for(int i = 0; i < 6; i++){
                        printf(" %d ", MDP->inode_table[k].blocks[i]);
                }
                printf("\n");
        }

        //ls -al
        printf("----------------------------------------file list---------------------------------------\n");
        int addr;
        int for_print = 0;
        for(int rootinode = 0; rootinode < 4; rootinode++)
        {
                addr = ((MDP->s.first_data_block) + (MDP->inode_table[MDP->s.first_inode].blocks[rootinode])) * BLOCK_SIZE;
                fseek(fp, addr, SEEK_SET);
                for(int datablock = 0; datablock < 32; datablock++)// 4kb를 뒤진다데이터              블록 4개
                {
                //inode 사이즈32kb
                        dp = malloc(sizeof(struct dentry));
                        fread(&dp->inode, sizeof(int), 1, fp);
                        fread(&dp->dir_length, sizeof(int), 1, fp);
                        fread(&dp->name_len, sizeof(int), 1, fp);
                        fread(&dp->file_type, sizeof(int), 1, fp);
                        fread(&dp->name, sizeof(char), 16, fp);// union 떄문에               n_pad[16]을 기준으로 봐야 오류가안남
                        printf("%s  ", dp->name);
                        for_print++;
                        if(for_print == 10){ for_print = 0; printf("\n");}
                        free(dp);
                }

        }
        printf("\n");
        printf("----------------------------------------------------------------------------------------\n");

}


void work(int signo)
{
        int i;
        int j;
        int k;
        int count;
        int block_address;
        char f_name[256];

        char operation = shm_info->operation;
        char file_name[16];
        for(i = 0; i < 16; i++){
                file_name[i] = shm_info->file_name[i];
        }
        char mode = shm_info->mode;
        int file_n = shm_info->file_n - 1;

        if(operation == 'O'){
                if(hold_curr->valid[file_n] == 1){
                        printf("file is already open\n");
                        print_inode_info();
			hit_in_pcb++;
                        if(mode == 'r'){
				write_ing[file_n] = 0;
                                give_data();
			}
                        else if(mode == 'w'){
				if(MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked == 1 && hold_curr->valid[file_n] == 1){
					printf("read모드에서 write모드로 변경합니다.\n");
                                	write_data();
				}
				else{
					kill(hold_curr->pid, SIGFPE);
                                	printf("이미 파일이 다른 프로세스에서 열려있어 접근 불가입니다.\n");
					hit_N_noting++;
					access_denied++;
				}
			}
                }
                else{
                        for(i = 0; i < 6; i++){
                                block_address = MDP->s.first_data_block * BLOCK_SIZE + MDP->inode_table[MDP->s.first_inode].blocks[i] * BLOCK_SIZE;
                                fseek(fp, block_address + 0x10, SEEK_SET);
                                for(j = 0; j < BLOCK_SIZE / 0x20; j++){
                                        count = 0;
                                        fread(&f_name, sizeof(char), 16, fp);
                                        for(k = 0; k < 8; k++){
                                                if(f_name[k] == file_name[k]){
                                                        count++;
                                                }
                                        }
                                        if(count == 8)
                                                break;
                                        fseek(fp, 0x10, SEEK_CUR);
                                }
                                if(count == 8)
                                        break;
                        }
                        if(count == 8){
				if(write_ing[file_n] != 0){
					kill(hold_curr->pid, SIGFPE);
					printf("다른 프로세스에서 write 중이어서 파일을 열 수 없습니다\n");
					return;
				}	
                                open_file();
                                print_inode_info();
                                if(mode == 'r')
                                        give_data();
                                else if(mode == 'w' && MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked == 0)
                                        write_data();
                                else{
                                	hold_curr->valid[file_n] = 0;
                                        MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked--;
                                        free(hold_curr->opened_file[file_n]);
					kill(hold_curr->pid, SIGFPE);
                                        printf("이미 파일이 다른 프로세스에서 열려있어 접근 불가입니다.\n");
					open_count--;
					access_denied++;
                                }
                        }
                        else{
                                printf("찾는 파일이 없습니다\n");
				file_find_fail++;
			}
                }
        }
        else if(operation == 'C'){
                print_inode_info();
                hold_curr->valid[file_n] = 0;
		write_ing[file_n] = 0;
                MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked--;
                free(hold_curr->opened_file[file_n]);
		close_count++;
        }
}


void open_file()
{
        int file_n = shm_info->file_n - 1;

        fseek(fp, -0x20, SEEK_CUR);

        struct dentry *newdent = (struct dentry*)malloc(sizeof(struct dentry));
        hold_curr->opened_file[file_n] = newdent;

        fread(&hold_curr->opened_file[file_n]->inode, sizeof(int), 1, fp);
        fread(&hold_curr->opened_file[file_n]->dir_length, sizeof(int), 1, fp);
        fread(&hold_curr->opened_file[file_n]->name_len, sizeof(int), 1, fp);
        fread(&hold_curr->opened_file[file_n]->file_type, sizeof(int), 1, fp);
        fread(&hold_curr->opened_file[file_n]->name, sizeof(int), 16, fp);

//      fseek(fp, -17, SEEK_CUR);
        MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked++;

	write_ing[file_n] = 0;
        hold_curr->valid[file_n] = 1;
	open_count++;
}

void give_data()
{
        int i;
        int j = 0;
        int file_n = shm_info->file_n - 1;
        int block_address;
        char data[BLOCK_SIZE * 6];
        for(i = 0; i < 6; i++){
                block_address = MDP->s.first_data_block * BLOCK_SIZE + MDP->inode_table[hold_curr->opened_file[file_n]->inode].blocks[i] * BLOCK_SIZE;
                fseek(fp, block_address, SEEK_SET);
                fread(&data, sizeof(char), BLOCK_SIZE, fp);
                for(; j < BLOCK_SIZE * (i + 1); j++){
                        shm_info->data[j] = data[j];
                        if(data[j] == '\0' && data[j+1] == '\0' && data[j+2] == '\0')
                                break;
                }
        }
        kill(hold_curr->pid, SIGHUP);
}

void receive_data(int signo)
{
        int i;
        int file_n = shm_info->file_n - 1;
        char data[BLOCK_SIZE];
        for(i = 0; i < BLOCK_SIZE; i++){
                data[i] = shm_info->data[i];
                        if(data[i] == '\0' && data[i+1] == '\0' && data[i+2] == '\0' && data[i+3] == '\0' && data[i+4] == '\0')
                                break;
        }

        printf("데이터 출력 값 : ");
        for(i = 0; i < BLOCK_SIZE; i++){
                printf("%c", data[i]);
                if(data[i] == '\0' && data[i+1] == '\0' && data[i+2] == '\0' && data[i+3] == '\0' && data[i+4] == '\0')
                        break;
        }
        printf("\n");
}

void write_data()
{
	
        int ch_zero[1] = {0};
        int address;
        int file_n = shm_info->file_n - 1;
        int start_block = MDP->s.first_data_block * BLOCK_SIZE + MDP->inode_table[hold_curr->opened_file[file_n]->inode].blocks[0] * BLOCK_SIZE;

        address = start_block + MDP->inode_table[hold_curr->opened_file[file_n]->inode].size;

        fseek(fp, address, SEEK_SET);
        fwrite(edit_data, 1, sizeof(edit_data), fp);
        if(check_first_change[file_n] == 0){
                fseek(fp, address - 1, SEEK_SET);
                fwrite(ch_zero, 1, 1, fp);
                check_first_change[file_n] = 1;
        }
	write_ing[file_n] = hold_curr->pid;
  	write_file++;

}


void print_inode_info()
{

        int i;
        int file_n = shm_info->file_n - 1;
        printf("-------------File info-------------\n");
        printf("mode                  : %d\n", MDP->inode_table[hold_curr->opened_file[file_n]->inode].mode);
        printf("locked                : %d\n", MDP->inode_table[hold_curr->opened_file[file_n]->inode].locked);
        printf("date                  : %d\n", MDP->inode_table[hold_curr->opened_file[file_n]->inode].date);
        printf("size                  : %d\n", MDP->inode_table[hold_curr->opened_file[file_n]->inode].size);
        printf("indirect_block        : %d\n", MDP->inode_table[hold_curr->opened_file[file_n]->inode].indirect_block);
        printf("blocks                : ");
        for(i = 0; i < 6; i++){
                if(MDP->inode_table[hold_curr->opened_file[file_n]->inode].blocks[i] != 0)
                        printf("%d, ", MDP->inode_table[hold_curr->opened_file[file_n]->inode].blocks[i]);
        }
        printf("\n");
        printf("-----------------------------------\n");

}


void print_openfile()
{

        int i, j;
        pcb *temp;
        temp = (pcb*)malloc(sizeof(pcb));
        temp = p_pcb->head;

        printf("opened file\n");
        while(temp != p_pcb->tail){
                printf("pid : %d \n", temp->pid);
                for(j = 0; j < 10; j++){
                        if(temp->valid[j] == 1)
                                printf("file_%d, ", j + 1);
                }
                printf("\n");
                temp = temp->next;
        }
        printf("pid : %d \n", temp->pid);
        for(j = 0; j < 10; j++){
        	if(temp->valid[j] == 1)
       		printf("file_%d, ", j + 1);
        }
	printf("\n");

	temp = wait_q->head;
	while(temp != NULL){
                printf("pid : %d \n", temp->pid);
                for(j = 0; j < 10; j++){
                        if(temp->valid[j] == 1)
                                printf("file_%d, ", j + 1);
                }
                printf("\n");
		temp = temp->next;
	}
     	
        printf("-----------------------------------\n");

}

void opened_zero(int signo)
{
	opened_file[shm_info->file_n - 1] = 0;
}

