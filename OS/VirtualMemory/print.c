//waitq 상태 출력
void waitq_print(pcb_L* wait_q, int global_counter)
{
        pcb* for_print;
        for_print = (pcb*)malloc(sizeof(pcb));
        for_print = wait_q->head;
        printf("----------------wait queue---------------\n\n");
        if(wait_q->head != NULL){
                while(for_print != NULL){
                        if(for_print->io_time != 0)
                                printf("(%d)노드 cpu_time : %d, io_time : %d, cpu점유율:%f  \n\n", for_print->pid % 10, for_print->cpu_time, for_print->io_time, (float)(for_print->cnt_cpu) / (global_counter - 1) * 100);
                        for_print = for_print->next;
                }
        }
        else
                printf("                   EMPTY                 \n\n");
        printf("-----------------------------------------\n\n");
}


//runq 상태 출력(편의상 runq는 runing 큐와 ready 큐를 합쳐 출력)
void runq_print(pcb_L* p_pcb, int global_counter)
{
        pcb* for_print;
        for_print = (pcb*)malloc(sizeof(pcb));
        for_print = p_pcb->head;
        printf("---------------running queue-------------\n\n");
        if(p_pcb->head != NULL){
                while(for_print != p_pcb->tail){
                        printf("(%d)노드 cpu_time : %d, io_time : %d, cpu점유율:%f \n\n", for_print->pid % 10, for_print->cpu_time, for_print->io_time, (float)(for_print->cnt_cpu) / (global_counter - 1) * 100);
                        for_print = for_print->next;
                }
                printf("(%d)노드 cpu_time : %d, io_time : %d, cpu점유율:%f \n\n", for_print->pid %10, for_print->cpu_time, for_print->io_time, (float)(for_print->cnt_cpu) / (global_counter - 1) * 100);

        }
        else
                printf("                   EMPTY                 \n\n");
        printf("-----------------------------------------\n\n\n\n\n");

}

