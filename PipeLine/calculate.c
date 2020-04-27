#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct {
	char opcode;
	int operand1[2];// [0] 구분 [1]값
	int operand2[2];
}reg;

reg state[10];
int state_num=0;

int R[10];
int reg_num;

void getop(char a);
int check=1;

FILE* fp;
char inst_reg[20];
char* pc = inst_reg;

int GCD(int a, int b);

void store();

int main()
{
   store();
   fclose(fp);
   if(check==0){printf("Wrong Instruction!\n"); return 0;}

   int COUNT=state_num;
   state_num = 0;

   while(state_num != COUNT)
   { printf("%d-th instruction..\n", state_num + 1);

    if(state[state_num].opcode=='P'){printf("Operated before, Pass..\n");
                                     state_num++; continue;
                                    }
    printf("your equation::%c ", state[state_num].opcode);

   if(state[state_num].operand1[0]==-1) 
    {
    printf("%d ", state[state_num].operand1[1]);
    }
    else
    {
     printf("R%d ", state[state_num].operand1[0]);
    }

    if(state[state_num].operand2[0]==-1) 
    {
    printf("%d \n", state[state_num].operand2[1]);
    }
    else
    {
     printf("R%d \n", state[state_num].operand2[0]);
    }   
    
       getop(state[state_num].opcode);

       for(reg_num=0;reg_num<10;reg_num++)
      {
        printf("R%d:%d ", reg_num, R[reg_num]);
      } 

       printf("\n\n");
       state_num++;
  }
   printf("FINISH!\n\n"); 
	
   return 0;

}

void  getop(char a)
{ 
     if(state[state_num].operand1[0] != -1){state[state_num].operand1[1]=R[state[state_num].operand1[0]];}
     if(state[state_num].operand2[0] != -1){state[state_num].operand2[1]=R[state[state_num].operand2[0]];}
	switch (a) {
	case '+': R[0] = (state[state_num].operand1[1] + state[state_num].operand2[1]); 
		break;

	case '-': R[0] = (state[state_num].operand1[1] - state[state_num].operand2[1]);
		break;

	case '*': R[0] = (state[state_num].operand1[1] * state[state_num].operand2[1]);
		break;

	case '/': R[0] = (state[state_num].operand1[1] / state[state_num].operand2[1]);
		break;

	case 'M':  R[state[state_num].operand1[0]] = state[state_num].operand2[1];
                   
                   printf("R%d is stored by %d \n", state[state_num].operand1[0], state[state_num].operand2[1]);
      		break;

        case 'C': if(state[state_num].operand1[1] > state[state_num].operand2[1]){R[0]=1;}
	          else if(state[state_num].operand1[1]== state[state_num].operand2[1]){R[0]=0;}
                  else{R[0]=-1;}
                break;

        case 'G': R[0]=GCD(state[state_num].operand1[1],state[state_num].operand2[1]); 
                break;
        case 'J':state[state_num].opcode = 'P'; 
                 state_num = (state[state_num].operand1[1] - 2);
                break;
        case 'B':if(R[0]==state[state_num].operand1[1])
                {
                 printf("R[0] is %d, B operated\n", state[state_num].operand1[1]);
                 state[state_num].opcode = 'P';
                 state_num = (state[state_num].operand2[1] - 2);
                }
                else 
                printf("R[0] is not %d, B ignored\n", state[state_num].operand1[1]);
                return;
                break;
      

        default:check=0; 
       }
}
int GCD(int a, int b)
{
 if(a==b){ return a;}
 else if(a>b) GCD(a-b,b);
 else GCD(a,b-a);
}
void store()
{
        fp = fopen("input.txt", "r");

	while (!feof(fp))
	{

		fgets(inst_reg, sizeof(inst_reg), fp);
		
		pc = strtok(inst_reg, " ");
		state[state_num].opcode = *pc;
               
         	if (state[state_num].opcode =='H'){return;}
             	pc = strtok(NULL, " ");

          switch(state[state_num].opcode){
             case '+':
             case '-':
             case '/':
             case '*':
             case 'M':
             case 'C':
             case 'G':
	     	 switch ((*pc)) {
		 case 'R':
                        state[state_num].operand1[0] = atoi(pc+1);
			break;

		 case '0':state[state_num].operand1[1] = strtoul(pc, NULL, 16);
                         state[state_num].operand1[0] = -1;
			break;

                default:check=0;	
                               }
                pc = strtok(NULL, " ");
              
		switch ((*pc)) {
		case 'R': state[state_num].operand2[0] = atoi(pc+1);
	        	break;

		case '0':state[state_num].operand2[1] = strtoul(pc, NULL, 16);
                         state[state_num].operand2[0] = -1;
			break;

                default:check=0;	
                               }
             break;
             
             case 'J': state[state_num].operand1[1]= atoi(pc);
                       state[state_num].operand1[0]= -1;
                       state[state_num].operand2[0]= -1;
                       state[state_num].operand2[1]= 0;
             break;
             case 'B': state[state_num].operand1[1]= atoi(pc);
                       state[state_num].operand1[0]= -1;
                       pc = strtok(NULL, " ");
                       state[state_num].operand2[0]= -1;
                       state[state_num].operand2[1]= atoi(pc);
             break;

                        }

		      
                   
                 state_num++;
	
          }      
               
}
