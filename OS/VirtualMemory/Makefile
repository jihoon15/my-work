VIRTUAL_MEMORY.exe : two_swap.o 
	gcc -o VIRTUAL_MEMORY.exe two_swap.o

two_swap.o : COMPLETE_TWO_LEVEL_SWAP.c
	gcc -c -o two_swap.o COMPLETE_TWO_LEVEL_SWAP.c

clean :
	rm *.o VIRTUAL_MEMORY.exe
