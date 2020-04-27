int real_time(int i, int time);

//waitq의 io_time 소비
void dec_waitq(pcb_L* wait_q)
{
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        temp = wait_q->head;

        if(wait_q->head != NULL){
                temp->io_time--;
                temp->for_turnaround++;
                temp = temp->next;
                while(temp != NULL){
                        temp->io_time--;
                        temp->for_turnaround++;
                        temp =temp->next;
                }
        }
}


//각 프로세스의 대기 시간 적립
void calcu_waiting(int check_first, int global_counter, pcb_L* p_pcb)
{
        if(check_first != 1 && global_counter <= 1000){
                pcb* for_wait;
                for_wait = (pcb*)malloc(sizeof(pcb));
                for_wait = p_pcb->head;

                if(for_wait != NULL){
                        for_wait = for_wait->next;

                        while(for_wait != p_pcb->head){
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

}


//io time이 0인 프로세스 waitq에서 runq로 이동
void finish_io(pcb_L* wait_q, pcb_L* p_pcb)
{
        if(wait_q->head != NULL){
                while(wait_q->head->io_time == 0){

                        wait_q->head->throuput++;
                        wait_q->head->turnaround += wait_q->head->for_turnaround;
                        wait_q->head->for_turnaround = 0;
                        wait_q->head->response_flag = 0;
                        push_old_pcb(p_pcb, pop_wait(wait_q), real_time(p_pcb->head->form, (rand() % 10) + 1));

                        if(wait_q->head == NULL)
                                break;
                }
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
                        real = 1;
                        break;
                case 1 :
                case 2 :
                        real = 1;
                        break;
                case 3 :
                case 4 :
                case 5 :
                case 6 :
                        real = 1;
                        break;
                case 7 :
                case 8 :
                        real = 1;
                        break;
                case 9 :
                        real = 1;
                        break;
                default :
                        printf("오류오류\n");
                }

        return real;
}

