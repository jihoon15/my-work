#include <stdio.h>
unsigned int MEMORY_INST[0x100000]={0,};
int pc;
int next_pc;//pc+4
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
ctrlsignal C_SIGNAL;
//_____________________________________
typedef struct{
int valid;
int dirty;

int tag;
int data[16];
}line;
line CACHE[0xc0];//   adress bit 20 8 4;
//__________________________________
int REGISTER[32]={0,};//레지스터
/////////////////
void IF();
void ID_RF(); 
void control();
void EX_AG();
void MEM();
void WB();
void PRINT();
void ALU1();
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
int Readreg1;
int Readreg2;

int Writereg;
int Readdata;
int Writedata_data;
int ALU1result;
int ALU2result;
int changed_address;
int bcond;
////////////////////
int MUX1(int rt, int rs);//0,1
int Mux1result;
int MUX2(int rt, int imm);
int Mux2result;
int MUX3(int alu,int readmem);
int Mux3result;
int MUX4(int normalpc,int branch);
int Mux4result;
int MUX5(int mux4,int jump);
int Mux5result;
void imm_extend();
void STORE_cache(int dest_addr,int index);
void STORE_mem(int drain_addr,int index);
int ReadMem(int Addr);
void WriteMem(int Addr, int val);
/////////////////
void count_type();
int type_R;
int type_I;
int type_J;
int nop;
int num_branch_inst;//브랜치 인스트럭션의 개수
int num_branch_execute;//브랜치 실행횟
int memory_access;

int HIT;
int MISS;
////////////////
void main()
{
              	store();
		REGISTER[29]=0x400000;
		REGISTER[31]=-1;
                    while(1)
                       {
			if(pc==-1){break;}
                        IF();
                        ID_RF();
                        EX_AG();
                        MEM();
                        WB();
 		//	PRINT();
			}
		printf("-----------------cycle%d---------------------\n",(type_R+type_I+type_J+nop));
	printf("pc:0x%x\n",pc);
			printf("------direct-map---------------------\n");
			printf("FINAL RESULT:%d\n",REGISTER[2]);
			printf("memory_access:%d \n", memory_access);
			printf("num of R_type:%d\n",type_R);
			printf("num of I_type:%d\n",type_I);
			printf("num of J_type:%d\n",type_J);
			printf("num of nop:%d\n", nop);
			printf("ALL INST(except nop):%d\n",(type_R+type_I+type_J));
			printf("branch_inst:%d\n",num_branch_inst);
			printf("branch_succeed:%d\n",num_branch_execute);
			printf("CACHE_HIT:%d\n",HIT);
			printf("CACHE_MISS:%d\n",MISS);
			printf("HIT-rate: %f\n",(float)HIT/(HIT+MISS) *100);

         return;
}
void store()//메모리에 인스트럭션 저장
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
//____________________________________________
void WriteMem(int Addr, int val)
{
 int tag=	(Addr&0xfffff000)>>12;//
 int index=	(Addr&0x00000fc0)>>6;
 int offset=	(Addr&0x0000003f);
 int dest_addr =(Addr&0xffffffc0);
 int drain_addr = (CACHE[index].tag <<12)|(index<<6);



 if(CACHE[index].valid==1)
 {
   if(CACHE[index].tag==tag)//있을때
   {
    HIT++;
    CACHE[index].data[offset/4]=val;
    CACHE[index].dirty=1;
   }
   else//태그가 다를때
   {
    if(CACHE[index].dirty ==1)
    {
     STORE_mem(drain_addr,index);
    }
    STORE_cache(dest_addr,index);
    CACHE[index].tag=tag;
    CACHE[index].valid=1;
    CACHE[index].dirty=1;
    CACHE[index].data[offset/4]=val;
    MISS++;
    memory_access++;
   }
  
 }
 else//없을때
 {
 MISS++;
 memory_access++;
 STORE_cache(dest_addr,index);//현재 sp 0x100000 이므로 넘어가는거 신경 x
 CACHE[index].tag=tag;
 CACHE[index].valid=1;
 CACHE[index].dirty=1;
 CACHE[index].data[offset/4]=val;
 }

}
//________________________________________
void STORE_mem(int drain_addr, int index)
{
    for(int i=0;i<16;i++)
    {
      MEMORY_INST[(drain_addr+ 4*i)/4] = CACHE[index].data[i];
    }
}
//__________________________________________
void STORE_cache(int dest_addr, int index)
{
 for(int i=0;i<16;i++)
 {
   CACHE[index].data[i]=MEMORY_INST[(dest_addr/4)+i];
   //printf("CHACE[%d][%d]%x \n", index, 4*i, CACHE_LINE[index].data[i]);
 }
 return;
}
//__________________________________________
int ReadMem(int Addr)
{
 int tag=	(Addr&0xfffff000)>>12;//
 int index=	(Addr&0x00000fc0)>>6;
 int offset=	(Addr&0x0000003f);
 int dest_addr =(Addr&0xffffffc0);
 int drain_addr = (CACHE[index].tag <<12)|(index<<6);

if(CACHE[index].valid==1)
 {
   if(CACHE[index].tag==tag)//있을때
   {
    HIT++;
    return CACHE[index].data[offset/4];
   }
   else//없을때
   {
    if(CACHE[index].dirty ==1)
    {
     STORE_mem(drain_addr,index);
    }
    STORE_cache(dest_addr,index);
    CACHE[index].tag=tag;
    CACHE[index].valid=1;
    CACHE[index].dirty=0;
    MISS++;
    memory_access++;
    return CACHE[index].data[offset/4];
   }
 }
 else//없을때
 {
 MISS++;
 memory_access++;
 STORE_cache(dest_addr,index);//현재 sp 0x100000 이므로 넘어가는거 신경 x
 CACHE[index].tag=tag;
 CACHE[index].valid=1;
 CACHE[index].dirty=0;
 return CACHE[index].data[offset/4];
 }
}
///////////////////////////////
void IF()
{
  INST=ReadMem(pc);
  next_pc = (pc+4);//write back 과정서 pc=next_pc로 해i
  return;
}
//////////////////////////////
void ID_RF()
{
    DATA.opcode=((INST&0xfc000000)>>26) & 0x3f;
    DATA.rs=(INST&0x03e00000)>>21;
	DATA.rt=(INST&0x001f0000)>>16;
	DATA.rd=(INST&0x0000f800)>>11;
	DATA.shamt=(INST&0x000007c0)>>6;
	DATA.funct=(INST&0x0000003f);
	DATA.address=(INST&0x03ffffff);    	
    DATA.imm=(INST&0x0000ffff);
	//bit 분배 완료
	if(INST==0x00000000){nop++;}
	else{count_type();}
	control(); 
	imm_extend();//I일때

	changed_address=((next_pc)&0xf0000000)|(DATA.address<<2); //next_pc 앞 4비트+address <<2

	Mux1result=MUX1(DATA.rt,DATA.rd);
	Readreg1=REGISTER[DATA.rs];
	Readreg2=REGISTER[DATA.rt];
	Writereg=Mux1result;
	Writedata_data=Readreg2;
	Mux2result=MUX2(Readreg2, Signextend);

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

void EX_AG()
{
	ALU2result=((next_pc)+(Signextend<<2));
	ALU1();
	Mux4result=MUX4(next_pc,ALU2result);
		return;
}
void ALU1()
{
switch(C_SIGNAL.ALUOp){
	case 0://R_TYPE
		switch(DATA.funct){
 			case 0x00:ALU1result=Mux2result<<DATA.shamt;//sll
			break;
			case 0x02:ALU1result=Mux2result>>DATA.shamt;//srl
			break;
			case 0x08:next_pc=(Readreg1);//jr ra
			break;
			case 0x20:
			case 0x21:ALU1result=(Readreg1 + Mux2result);//add addu
			break;
			case 0x22:
			case 0x23:ALU1result=(Readreg1 -  Mux2result);//sub subu
			break;
			case 0x24:ALU1result=(Readreg1 &  Mux2result);//and
			break;
			case 0x25:ALU1result=(Readreg1 |  Mux2result);//or
			break;
			case 0x27:ALU1result=~(Readreg1 |  Mux2result);//nor
			break;
			case 0x2a:
			case 0x2b:ALU1result=((Readreg1<Mux2result)? 1:0);//slt sltu
			break;
			case 0x9:ALU1result=next_pc;//jalr
					 next_pc=(Readreg1);
			break;
			default:
			break;
			
			}
	break;

	case 1://I_TYPE ALUi
			switch(DATA.opcode){//I구현한것 add addi slti sltiu
				case 0x8:
				case 0x9:ALU1result=(Readreg1+Mux2result);//addi addiu
				break;
				case 0xa:
				case 0xb:ALU1result=((Readreg1<Mux2result)?1:0);//slti sltu
				break;
				case 0xc:ALU1result=Readreg1&DATA.imm;//andi
				break;
				case 0xd:ALU1result=Readreg1|DATA.imm;//ori
				break;
				case 0xf:ALU1result=DATA.imm<<16;//lui
				break;
				case 0x2://J
				break;
				case 0x3://jal
						ALU1result=next_pc + 4;
						Writereg=31;
				break;

				default:	
				break;
							}
	break;

	case 2:
	case 3:ALU1result=Readreg1+Mux2result;//lw sw
			return;
	break;

	case 4://BRANCH ** 이미 C_SIGNAL>Branch==1이다
			switch(DATA.opcode){
				case 0x4://beq
						num_branch_inst++;
						if(Readreg1==Mux2result)
						{
							bcond=1;
							if(0x00008000&DATA.imm){ALU2result=next_pc+((0xfffc0000)|(DATA.imm<<2));}
							else{ALU2result=next_pc+(DATA.imm<<2);}
							num_branch_execute++;
						}
						else{bcond=0;}

				break;

				case 0x5://bne
						num_branch_inst++;
						if(Readreg1!=Mux2result)
						{
							bcond=1;
							if(0x00008000&DATA.imm){ALU2result=next_pc+((0xfffc0000)|(DATA.imm<<2));}
							else{ALU2result=next_pc+(DATA.imm<<2);}
							num_branch_execute++;
						}
						else{bcond=0;}
						
				break;
				
				default:
				break;
			}
	break;
	default:
	break;
	}
}

void MEM()
{
	if(C_SIGNAL.MemRead==1)//Lw
	{
		Readdata=ReadMem(ALU1result);
	}
	else if(C_SIGNAL.MemWrite==1)//Sw
	{
		//MEMORY_INST[ALU1result/4]=Writedata_data;
		WriteMem(ALU1result,Writedata_data);
	}
	Mux3result=MUX3(ALU1result,Readdata);
	Mux5result=MUX5(Mux4result,changed_address);

 return;
}
void WB()
{
//	printf("-----------------cycle%d---------------------\n",(type_R+type_I+type_J+nop));
//	printf("pc:0x%x\n",pc);
	if(C_SIGNAL.RegWrite==1)
	{
		REGISTER[Writereg]=Mux3result;
	}
	pc=Mux5result;
	return;
}
void PRINT()
{
        printf("INST:%x \n",INST);
	printf("R[2]:%d\n",REGISTER[2]);
	printf("sp:0x%x\n",REGISTER[29]);
	printf("ra:0x%x\n",REGISTER[31]);
	printf("\n");
	printf("opcode:0x%x\n",DATA.opcode);
	printf("rs:%d\n",DATA.rs);
	printf("rt:%d\n",DATA.rt);
	printf("rd:%d\n",DATA.rd);
	printf("shamt:0x%x\n",DATA.shamt);
	printf("funct:0x%x\n",DATA.funct);
	printf("immidiate:0x%x\n",DATA.imm);
	printf("address:0x%x\n",DATA.address);
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
 if(C_SIGNAL.MemtoReg==1){return readmem;}
 else{return alu;}
}

int MUX4(int normalpc,int branch)
{
 if(C_SIGNAL.Branch&bcond==1){return branch;}
 else{return normalpc;}
}

int MUX5(int mux4,int jump)
{
 if(C_SIGNAL.Jump==1){return jump;}
 else{return mux4;}
}
void count_type()
{
	switch(DATA.opcode){
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
