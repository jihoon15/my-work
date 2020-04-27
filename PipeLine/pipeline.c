#include<stdio.h>
#include<string.h>
unsigned int MEMORY_INST[0x100000 / 4]={0,};
int pc;
int count;
void store();// 처음에 메모리에 INST 저장
int Readdata_D;
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
ctrlsignal C_SIGNAL;

int REGISTER[32]={0,};//레지스터
//__________________________________latch1
typedef struct{
int PCsum4;
int INST;
}LATCH1;
LATCH1 IF_ID[3];
//__________________________________latch2
typedef struct{
int opcode;
int J_address;
int Signextend;
int val_rs;
int val_rt;
int Mux2result;
int shamt;
int imm;
int funct;

int PCsum4;

ctrlsignal second;
int INST;
int Write_reg;

}LATCH2;
LATCH2 ID_EX[3];
//__________________________________latch3
typedef struct{
int J_address;
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
void PRINT();
void ALU1(LATCH2 latch2);
////////////////////
int INST;//IF로가져올 인스트럭
typedef struct{
int opcode;
int rs;
int rt;
int rd;
int funct;
int imm;
int address;
int shamt;
}INSTdata;
INSTdata DATA;
/////////////////////////i
int Signextend;
int val_rs;
int val_rt;
int use_rs(LATCH2 latch2);
int use_rt(LATCH2 latch2);
int Writereg;
int Readdata;
int Writedata_data;
int ALU1result;
int ALU2result;
int J_address;
int bcond;
int INST_WB;
int HIT;
int MISS;
////////////////////
int MUX1(int rt, int rs);//0,1
int MUX2(int rt, int imm);
int Mux2result;
int MUX3(int alu,int readmem);
int Mux3result;
int MUX4(int normalpc,int branch);
int Mux4result;
int MUX5(int mux4,int jump);
int Mux5result;
void imm_extend();

void debug();
void shift_latch();
/////////////////
void count_type();
int type_R;
int type_I;
int type_J;
int nop;
int num_branch_inst;//브랜치 인스트럭션의 개수
int num_branch_execute;//브랜치 실행횟
int memory_access;
void all_zero();
////////////////
void main()
{
              	store();
		REGISTER[29]=0x100000;
		REGISTER[31]=-1;
		count=1;
		float H=100;	
		int run=2;
		        while(run!=0)
                       {
			if(pc==-1)
			{
		        run--;	
			}
			//printf("________________clock %d______________\n", count);
			//printf("pc:%x\n",pc);
			IF();
			WB(MEM_WB[1]);
			ID_RF(IF_ID[1]);
                        EX_AG(ID_EX[1]);
			if(((ID_EX[1].second.Branch)&&bcond)||((ID_EX[1].opcode==0x0)&&(ID_EX[1].funct==0x8)))//STALL EX에서 pc변경 jr || branch
			{IF_ID[0]=IF_ID[2];}
			MEM(EX_MEM[1]); 
		
			//debug();//각단계에서^ 실행한^ 인스트럭션 보여줌WB=o 읽뎡우 Wb단계에서 저거 wb홤
			shift_latch();//함수추가할것 
 			count++;
			}
			
			printf("(Always-not-taken)------------------------\n");
			printf("total clock:%d\n",count);
			printf("FINAL RESULT:%d\n",REGISTER[2]);
			printf("memory_access:%d \n", memory_access);
			printf("num of R_type:%d\n",type_R);
			printf("num of I_type:%d\n",type_I);
			printf("num of J_type:%d\n",type_J);	
			printf("ALL INST(except nop):%d\n",(type_R+type_I+type_J));
			printf("branch_inst:%d\n",num_branch_inst);
			printf("HIT:%d\n",HIT);
			printf("MISS:%d\n",MISS);
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
 pc=pc+4;

  return;
}
//////////////////////////////
void ID_RF(LATCH1 latch1)
{
	DATA.opcode=((latch1.INST&0xfc000000)>>26) & 0x3f;
    	DATA.rs=(latch1.INST&0x03e00000)>>21;
	DATA.rt=(latch1.INST&0x001f0000)>>16;
	DATA.rd=(latch1.INST&0x0000f800)>>11;
	DATA.shamt=(latch1.INST&0x000007c0)>>6;
	DATA.funct=(latch1.INST&0x0000003f);
	DATA.address=(latch1.INST&0x03ffffff);    	
    	DATA.imm=(latch1.INST&0x0000ffff);
	//bit 분배 완료
	//if(latch1.INST==0x00000000){nop++;}
	control();
	imm_extend();//I일때
	ID_EX[0].PCsum4=latch1.PCsum4;
	//_____________________________JUMP_______________________________
	if(C_SIGNAL.Jump==1)//점프시 바로 pc초기화, next 
	{
	pc=((latch1.PCsum4)&0xf0000000)|(DATA.address<<2);
	}


	//_________________저장_________________
	ID_EX[0].opcode=DATA.opcode;
	ID_EX[0].Signextend=Signextend;
	//ID_EX[0].J_address=((latch1.next_pc)&0xf0000000)|(DATA.address<<2); //next_pc 앞 4비트+address <<2
	ID_EX[0].Write_reg=MUX1(DATA.rt,DATA.rd);
	ID_EX[0].funct=DATA.funct;
	ID_EX[0].shamt=DATA.shamt;
	ID_EX[0].imm=DATA.imm;
	//_____________________RS _______________________
	if((DATA.rs == ID_EX[1].Write_reg)&&(DATA.rs!=0)&&(ID_EX[1].second.RegWrite))//거리1
	{
	ALU1(ID_EX[1]);
	ID_EX[0].val_rs=ALU1result;
	ALU1result=0;
	}

	else if((DATA.rs == EX_MEM[1].Write_reg)&&(DATA.rs!=0)&&(EX_MEM[1].third.RegWrite))//거리2
	{
	Readdata_D=MEMORY_INST[EX_MEM[1].ALU1result/4];
	ID_EX[0].val_rs=MUX3(EX_MEM[1].ALU1result,Readdata_D);
	Readdata_D=0;
	}

	else{ID_EX[0].val_rs=REGISTER[DATA.rs];}



	//______________________RT__________________________
	if((DATA.rt == ID_EX[1].Write_reg)&&(DATA.rt!=0)&&(ID_EX[1].second.RegWrite))//거리1
	{
	ALU1(ID_EX[1]);
	ID_EX[0].val_rt=ALU1result;
	ALU1result=0;
	}

	else if((DATA.rt == EX_MEM[1].Write_reg)&&(DATA.rt!=0)&&(EX_MEM[1].third.RegWrite))//거리2
	{
	Readdata_D=MEMORY_INST[EX_MEM[1].ALU1result/4];
	ID_EX[0].val_rt=MUX3(EX_MEM[1].ALU1result,Readdata_D);
	Readdata_D=0;
	}

	else{ID_EX[0].val_rt=REGISTER[DATA.rt];}

	ID_EX[0].Mux2result=MUX2(ID_EX[0].val_rt, ID_EX[0].Signextend);
	ID_EX[0].second=C_SIGNAL;
	ID_EX[0].INST=latch1.INST;
	return;
}
//////////////////////
void imm_extend()// 그냥 변환
{
   if(DATA.imm & 0x00008000){Signextend=(DATA.imm | 0xffff0000);}
   else{Signextend = DATA.imm;}
   return;
}
//////////////////////////
void control()
{
 	if(DATA.opcode ==0){C_SIGNAL.RegDest=1;C_SIGNAL.ALUOp=0;}// RegDst opcode ==0 ALUOp=0
    	else{C_SIGNAL.RegDest=0; C_SIGNAL.ALUOp=1;}//아닐경우 바로 ALUi 로설정 아래에서 걸릴경우에 값 변경

	if((DATA.opcode!=0x0)&&(DATA.opcode!=0x4)&&(DATA.opcode!=0x5)){C_SIGNAL.ALUSrc=1;}//alusrc
	else{C_SIGNAL.ALUSrc=0;}

	if(DATA.opcode==0x23){C_SIGNAL.MemtoReg=1; C_SIGNAL.MemRead=1; C_SIGNAL.ALUOp=2;}//memread memtoreg LW->ALUOp=2
	else{{C_SIGNAL.MemtoReg=0;C_SIGNAL.MemRead=0;}}

	if((DATA.opcode!=0x2b)&&(DATA.opcode!=0x5)&&(DATA.opcode!=0x4)&&
	(DATA.opcode!=0x2)&&!((DATA.opcode==0x0)&&(DATA.funct==0x8)))//regwrite
    	{C_SIGNAL.RegWrite=1;}
	else{C_SIGNAL.RegWrite=0;}

	if(DATA.opcode==0x2b){C_SIGNAL.MemWrite=1;C_SIGNAL.ALUOp=3;}//Mwrite   SW-> ALUOp=3
	else{C_SIGNAL.MemWrite=0;}

	if((DATA.opcode==0x2)||(DATA.opcode==0x3)){C_SIGNAL.Jump=1;}//Jump
	else{C_SIGNAL.Jump=0;}

	if((DATA.opcode==0x4)||(DATA.opcode==0x5)){C_SIGNAL.Branch=1;C_SIGNAL.ALUOp=4;}//branch 일 경우
	else{C_SIGNAL.Branch=0;}
	return;
}

void EX_AG(LATCH2 latch2)
{
	ALU1(latch2);

	EX_MEM[0].J_address=latch2.J_address;
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
//////////////////////////////////////////////////////////////
	case 4://BRANCH ** 이미 C_SIGNAL>Branch==1이다
			switch(latch2.opcode){//_______________________________________예외____________________________-
				case 0x4://beq
						num_branch_inst++;
						if(latch2.val_rs==latch2.Mux2result)
						{
							bcond=1;
							if(0x00008000&latch2.imm){pc=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
							else{pc=(latch2.PCsum4)+(latch2.imm<<2);}
							MISS++;
						}
						else{bcond=0;HIT++;}

				break;

				case 0x5://bne
						num_branch_inst++;
						if(latch2.val_rs!=latch2.Mux2result)
						{
							bcond=1;
							if(0x00008000&latch2.imm){pc=(latch2.PCsum4)+((0xfffc0000)|(latch2.imm<<2));}
							else{pc=(latch2.PCsum4)+(latch2.imm<<2);}
							MISS++;
						}
						else{bcond=0;HIT++;}
						
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
	else if(latch3.third.MemWrite==1)//Sw
	{
		MEMORY_INST[latch3.ALU1result/4]=latch3.val_rt;
		memory_access++;
	}
	Mux3result=MUX3(latch3.ALU1result,Readdata);
	
	MEM_WB[0].Mux3result=Mux3result;
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

/////////////////////////////////////////
int MUX1(int rt, int rs)//mux(0,1)
{
 if(C_SIGNAL.RegDest==1){return rs;}
 else{return rt;}
}
int MUX2(int rt, int imm)
{
 if(C_SIGNAL.ALUSrc==1){return imm;}
 else{return rt;}
}

int MUX3(int alu,int readmem)
{
 if(EX_MEM[1].third.MemtoReg==1){return readmem;}
 else{return alu;}
}

int MUX4(int normalpc,int branch)
{
 if(ID_EX[1].second.Branch&&bcond){return branch;}
 else{return normalpc;}
}

int MUX5(int mux4,int jump)
{
 if(ID_EX[1].second.Jump==1){return jump;}
 else{return mux4;}
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


