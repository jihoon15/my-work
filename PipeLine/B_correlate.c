//warm-up 초창기에 last로하다가 마지막꺼를 앞에다가 비교
//correlation 다른거하나안하나
// 합쳐서 하이브리드 __________
// 만약 pc저장된게 한개다->히스토리봐서 데이터가어느정도없으면 last 있으면패턴분석
#include<stdio.h>
unsigned int MEMORY_INST[0x100000 / 4]={0,};
int pc;
int clock;
void store();// 처음에 메모리에 INST 저장
typedef struct{//control signal
        int RegDest;
        int Jump;
        int Branch;
        int MemRead;
        int MemtoReg;
        int ALUOp;//
        int MemWrite;
        int ALUSrc;
        int RegWrite;
}ctrlsignal;
int REGISTER[32]={0,};//레지스터
//__________________________________latch1
typedef struct{
int PCsum4;
int INST;
int whereisINST;
int JUMPORNOT;
}LATCH1;
LATCH1 IF_ID[3];
//__________________________________latch2
typedef struct{
int opcode;
int rs;
int rt;
int rd;
int address;
int Signextend;
int val_rs;
int val_rt;
int Mux2result;
int shamt;
int imm;
int funct;
int whereisINST;
int PCsum4;
int JUMPORNOT;

ctrlsignal second;
int INST;
int Write_reg;

}LATCH2;
LATCH2 ID_EX[3];
//__________________________________latch3
typedef struct{
int ALU1result;
int val_rt;

ctrlsignal third;
int INST;
int Write_reg;
}LATCH3;
LATCH3 EX_MEM[3];
//__________________________________latch4
typedef struct{
int Mux3result;

ctrlsignal fourth;
int INST;
int Write_reg;
}LATCH4;

LATCH4 MEM_WB[3];
//_______________________________________파라미터로 수
void IF();
void ID_RF(LATCH1 latch1); 
void control();
void EX_AG(LATCH2 latch2);
void MEM(LATCH3 latch3);
void WB(LATCH4 latch4);
void ALU1(LATCH2 latch2);

/////////////////////////
int Readdata;
int ALU1result;
int ALU2result;
int INST_WB;
int J_target;
int MUX1(int rt, int rs);//0,1
int MUX2(int rt, int imm);
int MUX3(int alu,int readmem);
void imm_extend();
void debug();
void shift_latch();
//________________BRANCH  PREDICT___________________
int FIRST_BIT[10];//1이면 J_target 0이면 +8
int SECOND_BIT[10];//1이면 거기로 0이면 딴걸로  
// Dir/GorN 
//11 B으로 점프고 점프함 
//10 B지만 점프안함 
//01 NOT이고 그러함 
//00 NOT이고 점프안함
int t_target;
int nt_target;
int PC_HISTORY[10];
int T_ADDRESS[10];
int NT_ADDRESS[10];
int EMPTY[10];
int STATE[10];
int check;//항상 0인변스
void CHECK_HISTORY(int check,int PC);
void STORE_SLOT(LATCH2 latch2);
int HIT;
int MISS;
int check_index;
int check_fact;
int check_history;
int use_rs(LATCH2 latch2);
int use_rt(LATCH2 latch2);
int B_H[2];
void UPDATE_B_H(int i);
/////////////////
void count_type();
int type_R;
int type_I;
int type_J;
int nop;
int num_branch_inst;//브랜치 인스트럭션의 개수
int memory_access;
int JUMPORNOT;
////////////////
void main()
{
              	store();
		REGISTER[29]=0x100000;
		REGISTER[31]=-1;
		clock=1;
	        int run=2;
		        while(1)
                       {
			if(pc==-1)
			{
			run--;
			}
			//printf("________________clock %d______________\n", clock);
			//printf("pc:%x\n",pc);
			IF();
			WB(MEM_WB[1]);
			ID_RF(IF_ID[1]);
                        EX_AG(ID_EX[1]);
			if(((ID_EX[1].opcode==0x0)&&(ID_EX[1].funct==0x8)))//STALL EX에서 pc변경 jr || branch
			{IF_ID[0]=IF_ID[2];}
			MEM(EX_MEM[1]); 
		
			//debug();//각단계에서^ 실행한^ 인스트럭션 보여줌WB=o 읽뎡우 Wb단계에서 저거 wb홤
			if(run==0){break;}
			shift_latch();//함수추가할것 
 			clock++;
			}
			printf("(Global Correlation)------------------------\n");
			printf("total clock:%d\n",clock);
			printf("FINAL RESULT:%d\n",REGISTER[2]);
			printf("memory_access:%d \n", memory_access);
			printf("num of R_type:%d\n",type_R);
			printf("num of I_type:%d\n",type_I);
			printf("num of J_type:%d\n",type_J);	
			printf("ALL INST(except nop):%d\n",(type_R+type_I+type_J));
			printf("branch_inst:%d\n",num_branch_inst);
			printf("NUM OF HIT:%d\n",HIT);
			printf("NUM OF MISS:%d\n",MISS);
			printf("HIT-RATE:%f\n",(float)HIT/(float)num_branch_inst);

         return;
}
void store()//메모리에 인스트럭션 저장정
{
    int i=0;
    FILE* fp = NULL;
	unsigned int ch=0;
        unsigned int data = 0;
        fp = fopen("test_prog/input4.bin", "rb");
	     while(1)
       {
         ch=fread(&data, sizeof(int), 1, fp);
         if(feof(fp)){break;}
         MEMORY_INST[i]=((data & 0xff000000)>>24) | ((data & 0xff0000)>>8) | ((data & 0xff00)<<8) | ((data & 0xff)<<24);
         i++;
       }

    fclose(fp);
    return;
}
///////////////////////////////
void IF()
{
 if(pc==-1){return;}
 IF_ID[0].INST=MEMORY_INST[pc/4];
 IF_ID[0].PCsum4 = (pc+4);//write back 과정서 pc=next_pc로 해i
 IF_ID[0].whereisINST=pc;

 CHECK_HISTORY(check,pc);

 if(check_history==1)//있으면
 {
   /*if(B_H[0]==0)
   {
   	if(STATE[check_index]==0)
     	{pc=T_ADDRESS[check_index];}
	else
	{pc=NT_ADDRESS[check_index];}
        IF_ID[0].JUMPORNOT=0;
   }*/

  if((B_H[0]==1)&&(B_H[1]==1))
  {
     	if(STATE[check_index]==1)
     	{pc=T_ADDRESS[check_index];}
	else
	{pc=NT_ADDRESS[check_index];}
	IF_ID[0].JUMPORNOT=1;
  }
  else if((B_H[0]==0)||(B_H[1]==0))
  {
     	if(STATE[check_index]==0)
     	{pc=T_ADDRESS[check_index];}
	else
	{pc=NT_ADDRESS[check_index];}
	IF_ID[0].JUMPORNOT=0;
  }
  

  if((B_H[0]==0)&&(B_H[1]==0))
  {
     	if(STATE[check_index]==1)
     	{pc=T_ADDRESS[check_index];}
	else
	{pc=NT_ADDRESS[check_index];}
	IF_ID[0].JUMPORNOT=1;
  }
  //J_ADDRESS=pc;
 }
 else
 {
 pc=pc+4;
 }
  return;
}
//////////////////////////////
void ID_RF(LATCH1 latch1)
{
	ID_EX[0].opcode=((latch1.INST&0xfc000000)>>26) & 0x3f;
    	ID_EX[0].rs=(latch1.INST&0x03e00000)>>21;
	ID_EX[0].rt=(latch1.INST&0x001f0000)>>16;
	ID_EX[0].rd=(latch1.INST&0x0000f800)>>11;
	ID_EX[0].shamt=(latch1.INST&0x000007c0)>>6;
	ID_EX[0].funct=(latch1.INST&0x0000003f);
	ID_EX[0].address=(latch1.INST&0x03ffffff);    	
    	ID_EX[0].imm=(latch1.INST&0x0000ffff);
	control();
	imm_extend();//I일때
	ID_EX[0].PCsum4=latch1.PCsum4;
	//_____________________________JUMP_______________________________
	if(ID_EX[0].second.Jump==1)//점프시 바로 pc초기화, next 
	{
	pc=((latch1.PCsum4)&0xf0000000)|(ID_EX[0].address<<2);
	}

	//_________________저장_________________
	ID_EX[0].Write_reg=MUX1(ID_EX[0].rt,ID_EX[0].rd);
	//_____________________RS _______________________________________________
	if((ID_EX[0].rs == ID_EX[1].Write_reg)&&(use_rs(ID_EX[0])==1)&&(ID_EX[1].second.RegWrite))//거리1
	{
	ALU1(ID_EX[1]);
	ID_EX[0].val_rs=ALU1result;
	ALU1result=0;
	}

	else if((ID_EX[0].rs == EX_MEM[1].Write_reg)&&(use_rs(ID_EX[0])==1)&&(EX_MEM[1].third.RegWrite))//거리2
	{
	Readdata=MEMORY_INST[EX_MEM[1].ALU1result/4];
	ID_EX[0].val_rs=MUX3(EX_MEM[1].ALU1result,Readdata);
	Readdata=0;
	}

	else{ID_EX[0].val_rs=REGISTER[ID_EX[0].rs];}



	//______________________RT__________________________
	if((ID_EX[0].rt == ID_EX[1].Write_reg)&&(use_rt(ID_EX[0])==1)&&(ID_EX[1].second.RegWrite))//거리1
	{
	ALU1(ID_EX[1]);
	ID_EX[0].val_rt=ALU1result;
	ALU1result=0;
	}

	else if((ID_EX[0].rt == EX_MEM[1].Write_reg)&&(use_rt(ID_EX[0])==1)&&(EX_MEM[1].third.RegWrite))//거리2
	{
	Readdata=MEMORY_INST[EX_MEM[1].ALU1result/4];
	ID_EX[0].val_rt=MUX3(EX_MEM[1].ALU1result,Readdata);
	Readdata=0;
	}

	else{ID_EX[0].val_rt=REGISTER[ID_EX[0].rt];}

	ID_EX[0].Mux2result=MUX2(ID_EX[0].val_rt, ID_EX[0].Signextend);
	ID_EX[0].INST=latch1.INST;
	ID_EX[0].whereisINST=latch1.whereisINST;
	ID_EX[0].JUMPORNOT=latch1.JUMPORNOT;
	return;
}
//////////////////////
void imm_extend()// 그냥 변환
{
   if(ID_EX[0].imm & 0x00008000){ID_EX[0].Signextend=(ID_EX[0].imm | 0xffff0000);}
   else{ID_EX[0].Signextend = ID_EX[0].imm;}
   return;
}
//////////////////////////
void control()
{
 	if(ID_EX[0].opcode ==0)
	{ID_EX[0].second.RegDest=1;ID_EX[0].second.ALUOp=0;}// RegDst opcode ==0 ALUOp=0
    	else
	{ID_EX[0].second.RegDest=0; ID_EX[0].second.ALUOp=1;}//아닐경우 바로 ALUi 로설정 아래에서 걸릴경우에 값 변경

	if((ID_EX[0].opcode!=0x0)&&(ID_EX[0].opcode!=0x4)&&(ID_EX[0].opcode!=0x5))
	{ID_EX[0].second.ALUSrc=1;}//alusrc
	else
	{ID_EX[0].second.ALUSrc=0;}

	if(ID_EX[0].opcode==0x23)
	{ID_EX[0].second.MemtoReg=1; ID_EX[0].second.MemRead=1; ID_EX[0].second.ALUOp=2;}//memread memtoreg LW->ALUOp=2
	else
	{{ID_EX[0].second.MemtoReg=0;ID_EX[0].second.MemRead=0;}}

	if((ID_EX[0].opcode!=0x2b)&&(ID_EX[0].opcode!=0x5)&&(ID_EX[0].opcode!=0x4)&&
	(ID_EX[0].opcode!=0x2)&&!((ID_EX[0].opcode==0x0)&&(ID_EX[0].funct==0x8)))//regwrite

    	{ID_EX[0].second.RegWrite=1;}
	else
	{ID_EX[0].second.RegWrite=0;}

	if(ID_EX[0].opcode==0x2b)
	{ID_EX[0].second.MemWrite=1;ID_EX[0].second.ALUOp=3;}//Mwrite   SW-> ALUOp=3
	else
	{ID_EX[0].second.MemWrite=0;}

	if((ID_EX[0].opcode==0x2)||(ID_EX[0].opcode==0x3))
	{ID_EX[0].second.Jump=1;}//Jump
	else
	{ID_EX[0].second.Jump=0;}

	if((ID_EX[0].opcode==0x4)||(ID_EX[0].opcode==0x5))
	{ID_EX[0].second.Branch=1;ID_EX[0].second.ALUOp=4;}//branch 일 경우
	else
	{ID_EX[0].second.Branch=0;}
	return;
}

void EX_AG(LATCH2 latch2)
{
	ALU1(latch2);

	EX_MEM[0].ALU1result=ALU1result;
	EX_MEM[0].val_rt=latch2.val_rt;
	EX_MEM[0].third=latch2.second;
	EX_MEM[0].INST=latch2.INST;
	EX_MEM[0].Write_reg=latch2.Write_reg;
	return;
}
void ALU1(LATCH2 latch2)
{
switch(latch2.second.ALUOp){
	case 0://R_TYPE
		switch(latch2.funct){
 			case 0x00:ALU1result=latch2.Mux2result<<latch2.shamt;//sll
			break;
			case 0x02:ALU1result=latch2.Mux2result>>latch2.shamt;//srl
			break;
			case 0x08:pc=(latch2.val_rs);//jr ra_______________예외
			break;
			case 0x20:
			case 0x21:ALU1result=(latch2.val_rs + latch2.Mux2result);//add addu
			break;
			case 0x22:
			case 0x23:ALU1result=(latch2.val_rs -  latch2.Mux2result);//sub subu
			break;
			case 0x24:ALU1result=(latch2.val_rs &  latch2.Mux2result);//and
			break;
			case 0x25:ALU1result=(latch2.val_rs |  latch2.Mux2result);//or
			break;
			case 0x27:ALU1result=~(latch2.val_rs |  latch2.Mux2result);//nor
			break;
			case 0x2a:
			case 0x2b:ALU1result=((latch2.val_rs<latch2.Mux2result)? 1:0);//slt sltu
			break;
			case 0x9:ALU1result=latch2.PCsum4;//jalr______안씀
					 pc=(latch2.val_rs);
			break;
			default:
			break;
			
			}
	break;

	case 1://I_TYPE ALUi
			switch(latch2.opcode){//I구현한것 add addi slti sltiu
				case 0x8:
				case 0x9:ALU1result=(latch2.val_rs+latch2.Mux2result);//addi addiu
				break;
				case 0xa:
				case 0xb:ALU1result=((latch2.val_rs<latch2.Mux2result)?1:0);//slti sltu
				break;
				case 0xc:ALU1result=latch2.val_rs&latch2.imm;//andi
				break;
				case 0xd:ALU1result=latch2.val_rs|latch2.imm;//ori
				break;
				case 0xf:ALU1result=latch2.imm<<16;//lui
				break;
				case 0x2://J__________________________________________예외____________
				break;
				case 0x3://jal
						REGISTER[31]=latch2.PCsum4 + 4;
						
				break;

				default:	
				break;
							}
	break;

	case 2:
	case 3:ALU1result=latch2.val_rs+latch2.Mux2result;//lw sw
			return;
	break;
	case 4://BRANCH ** 이미 C_SIGNAL>Branch==1이다
			switch(latch2.opcode){//_______________________________________예외____________________________-
				case 0x4://beq
						num_branch_inst++;
						if(latch2.val_rs==latch2.Mux2result)
						{
							if(0x00008000&latch2.imm){t_target=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
							else{t_target=(latch2.PCsum4)+(latch2.imm<<2);}
							check_fact=1;
							nt_target=latch2.whereisINST+4;
						}
						else{
							if(0x00008000&latch2.imm){nt_target=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
							else{nt_target=(latch2.PCsum4)+(latch2.imm<<2);}
							t_target=latch2.whereisINST+4;
							check_fact=0;
						}
 						UPDATE_B_H(check_fact);
						CHECK_HISTORY(check,latch2.whereisINST);

						if(check_history==1)//저장되있으면
						{
						   if(latch2.JUMPORNOT==check_fact)//성공!
						   {
						       HIT++;
						   }
						   else//실패
						   {
						   MISS++;
						   IF_ID[0]=IF_ID[2];
						   ID_EX[0]=ID_EX[2];
						   pc=t_target;
						   }
						}

						else{STORE_SLOT(latch2);MISS++;}//없으면
								
				break;

				case 0x5://bne
						num_branch_inst++;
						if(latch2.val_rs!=latch2.Mux2result)
						{
							if(0x00008000&latch2.imm){t_target=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
							else{t_target=(latch2.PCsum4)+(latch2.imm<<2);}
							nt_target=latch2.whereisINST+4;
							check_fact=1;
						}
						else
						{
						if(0x00008000&latch2.imm){nt_target=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
						else{nt_target=(latch2.PCsum4)+(latch2.imm<<2);}
						t_target=latch2.whereisINST+4;
						check_fact=0;

						}
 						UPDATE_B_H(check_fact);
						CHECK_HISTORY(check,latch2.whereisINST);

						if(check_history==1)//저장되있으면
						{
					           if(latch2.JUMPORNOT==check_fact)//성공!
						   {
						       HIT++;
						   }					
						   else//실패
						   {
						   MISS++;
						   IF_ID[0]=IF_ID[2];
						   ID_EX[0]=ID_EX[2];
						   pc=t_target;
						   }
						}
						else{STORE_SLOT(latch2);MISS++;}//없으면
						
				break;
				
				default:
				break;
			}
	break;
/////////////////////////////////////////////////////////////////////
	default:
	break;
	}
}

void MEM(LATCH3 latch3)
{
	if(latch3.third.MemRead==1)//Lw
	{
		Readdata=MEMORY_INST[latch3.ALU1result/4];
		memory_access++;
	}
	else if(latch3.third.MemWrite==1)//Sw성
	{
		MEMORY_INST[latch3.ALU1result/4]=latch3.val_rt;
		memory_access++;
	}
	
	MEM_WB[0].Mux3result=MUX3(latch3.ALU1result,Readdata);
	MEM_WB[0].INST=latch3.INST;
	MEM_WB[0].Write_reg=latch3.Write_reg;
	MEM_WB[0].fourth=latch3.third;
	return;
}
void WB(LATCH4 latch4)
{
	if(latch4.INST==0x00000000){nop++;}
	else{count_type(latch4);}
	INST_WB=latch4.INST;
	if(latch4.fourth.RegWrite==1)
	{
		REGISTER[latch4.Write_reg]=latch4.Mux3result;
	}
	return;
}

int MUX1(int rt, int rs)//mux(0,1)
{
 if(ID_EX[0].second.RegDest==1){return rs;}
 else{return rt;}
}
int MUX2(int rt, int imm)
{
 if(ID_EX[0].second.ALUSrc==1){return imm;}
 else{return rt;}
}

int MUX3(int alu,int readmem)
{
 if(EX_MEM[1].third.MemtoReg==1){return readmem;}
 else{return alu;}
}

void count_type(LATCH4 latch4)
{
	switch((((latch4.INST&0xfc000000)>>26) & 0x3f))
	{
	case 0x0:type_R++;
	break;
	case 0x2:
	case 0x3:type_J++;
	break;
	default:type_I++;
	break;
	}
	return;
}
void debug()
{

printf("IF:%08x\n",IF_ID[0].INST);
printf("ID:%08x\n",ID_EX[0].INST);
printf("EX:%08x\n",EX_MEM[0].INST);
printf("MEM:%08x\n",MEM_WB[0].INST);
printf("WB:%08x\n",INST_WB);
if(pc!=-1)
{printf("next pc=%x\n", pc);}
else
{printf("Ra is now -1\n");}
printf("RA: %x\n",REGISTER[31]);
printf("sp: %x\n",REGISTER[29]);
printf("V0: %d\n",REGISTER[2]);
}
void shift_latch()
{
	IF_ID[1]=IF_ID[0];
	IF_ID[0]=IF_ID[2];

	ID_EX[1]=ID_EX[0];
	ID_EX[0]=ID_EX[2];

	EX_MEM[1]=EX_MEM[0];
	EX_MEM[0]=EX_MEM[2];

	MEM_WB[1]=MEM_WB[0];
	MEM_WB[0]=MEM_WB[2];
}
//_____________________________________________________________________________
void  CHECK_HISTORY(int check, int PC)//PC 값이 있나 없나 체크함
{
 if(check==10){check_history=0; return;}
 if(EMPTY[check]==1){
  if(PC_HISTORY[check]==PC)
  {
  check_history=1;
  check_index=check;
  return;
  }
 }
 CHECK_HISTORY(check+1,PC);

}
void STORE_SLOT(LATCH2 latch2)//처음저장///////////////////////////////////////////////완
{
 int i=0;
 while(PC_HISTORY[i]!=0)
 {
 i++;
 }
 STATE[i]=check_fact;
 PC_HISTORY[i]=latch2.whereisINST;
 T_ADDRESS[i]=t_target;
 NT_ADDRESS[i]=nt_target;
 EMPTY[i]=1;
 IF_ID[0]=IF_ID[2];

 pc=t_target;
 return;
}
int use_rs(LATCH2 latch2)
{
 if(latch2.rs!=0)
 {
  if((latch2.opcode==0x3)||(latch2.opcode==0xf)||((latch2.opcode==0x0)&&(((latch2.funct==0x2))||(latch2.funct==0x2))))
  {
  return 0;
  }
  else
  {
  return 1;
  }
 }
 else
 {
  return 0;
 }

}
int use_rt(LATCH2 latch2)
{
  if(latch2.rt!=0)
 {
  if((latch2.opcode==0x3)||(latch2.opcode==0xf)||((latch2.opcode==0x0)&&(latch2.funct==0x8))||(latch2.opcode==0x8)||(latch2.opcode==0x9)||(latch2.opcode==0xc)||(latch2.opcode==0x30)||(latch2.opcode==0x23)||(latch2.opcode==0xd)||(latch2.opcode==0xa)||(latch2.opcode==0xb))
  {
  return 0;
  }
  else
  {
  return 1;
  }
 }
 else
 {
  return 0;
 }
}
void UPDATE_B_H(int i)
{
 B_H[0]=B_H[1];
 B_H[1]=i;
 return;
}
