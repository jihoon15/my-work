#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define ASCII_SIZE 256
int buffer_num;

typedef struct sharedobject {
	FILE *rfile;
	char *line[24];
	pthread_mutex_t lock[24];
	int turn[24];
	int finish;
} so_t;

pthread_mutex_t hand_out_num=PTHREAD_MUTEX_INITIALIZER;

void PRINT();
int count=0;
int temp=0;
int stat[ASCII_SIZE];
void PRINT();

void *producer(void *arg) {
        int num=0;;
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line=NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		///////
		if(so->turn[num]!=0)
		{
		num++;
		num=num%buffer_num;
		continue;
		}
		pthread_mutex_lock(&so->lock[num]);

		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) {
			for(int h=0;h<buffer_num;h++)
			{
			 if(so->turn[h]==0)//사용중인애들
			 {
			 so->turn[h]=1;
			 so->line[h] = NULL;
			 pthread_mutex_unlock(&so->lock[h]);
			 }
			 else//아닌애
			  {
			  so->finish=1;
			  }
			 }
			break;
		}
		so->line[num] = strdup(line);      /* share the line */
		so->turn[num] = 1;
		i++;
		pthread_mutex_unlock(&so->lock[num]);
	}
	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	
	int *stat_individual = malloc(sizeof(int)*257);
	memset(stat_individual, 0 , sizeof(stat_individual));

	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;
	int order;
	////////////////////
	pthread_mutex_lock(&hand_out_num);
	order=count;
	count++;
	pthread_mutex_unlock(&hand_out_num);

	while (1) {

		if(so->finish==1&&so->turn[order]==0)
		{
		break;
		}
		while(so->turn[order]==0);
		pthread_mutex_lock(&so->lock[order]);
		line = so->line[order];
		if (line == NULL) {
			pthread_mutex_unlock(&so->lock[order]);
			break;
		}
		len = strlen(line);

		for(int b=0;b<len;b++)
		{
		 if(*line<256 && *line>1)
		 {
		  stat_individual[*line]++;
		 }
		 line++;
		}

		free(so->line[order]);
		stat_individual[257]++;//i++
		so->turn[order]=0;
		pthread_mutex_unlock(&so->lock[order]);
		////////////////////count char

	}
	printf("Cons: %d lines\n", stat_individual[257]);
	pthread_exit(stat_individual);
}


int main (int argc, char *argv[])
{
	memset(stat,0,sizeof(stat));
	int* getarray= malloc(sizeof(int)*256);
	memset(getarray,0,sizeof(getarray));	
	time_t start,end;
	double result;
	//start=time(NULL);
	pthread_t prod[24];
	pthread_t cons[24];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	FILE *rfile;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t *share = malloc(sizeof(so_t));
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 24) Nprod = 24;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 24) Ncons = 24;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

	buffer_num=Ncons;

	share->rfile = rfile;
	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share);

	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &getarray);
		for(int K=0;K<256;K++)
		{
		 stat[K]+=getarray[K];
		}
		printf("main: consumer_%d joined with %d\n", i, getarray[257]);
	}
	//printf("\n\n\n\n\n");
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);//쓰레드가 종료되는걸 기다
		printf("main: producer_%d joined with %d\n", i, *ret);
	}
	PRINT();
	//end=time(NULL);
	//result=(double)(end-start);
	//printf("Taken time:%f\n",result);

	pthread_exit(NULL);
	exit(0);
}
void PRINT()
{
printf("char: A Freq: %d\n",stat['A']+stat['a']);
printf("char: B Freq: %d\n",stat['B']+stat['b']);
printf("char: C Freq: %d\n",stat['C']+stat['c']);
printf("char: D Freq: %d\n",stat['D']+stat['d']);
printf("char: E Freq: %d\n",stat['E']+stat['e']);
printf("char: F Freq: %d\n",stat['F']+stat['f']);
printf("char: G Freq: %d\n",stat['G']+stat['g']);
printf("char: H Freq: %d\n",stat['H']+stat['h']);
printf("char: I Freq: %d\n",stat['I']+stat['i']);
printf("char: J Freq: %d\n",stat['J']+stat['j']);
printf("char: K Freq: %d\n",stat['K']+stat['k']);
printf("char: L Freq: %d\n",stat['L']+stat['l']);
printf("char: M Freq: %d\n",stat['M']+stat['m']);
printf("char: N Freq: %d\n",stat['N']+stat['n']);
printf("char: O Freq: %d\n",stat['O']+stat['o']);
printf("char: P Freq: %d\n",stat['P']+stat['p']);
printf("char: Q Freq: %d\n",stat['Q']+stat['q']);
printf("char: R Freq: %d\n",stat['R']+stat['r']);
printf("char: S Freq: %d\n",stat['S']+stat['s']);
printf("char: T Freq: %d\n",stat['T']+stat['t']);
printf("char: U Freq: %d\n",stat['U']+stat['u']);
printf("char: V Freq: %d\n",stat['V']+stat['v']);
printf("char: W Freq: %d\n",stat['W']+stat['w']);
printf("char: X Freq: %d\n",stat['X']+stat['x']);
printf("char: Y Freq: %d\n",stat['Y']+stat['y']);
printf("char: Z Freq: %d\n",stat['Z']+stat['z']);
}

