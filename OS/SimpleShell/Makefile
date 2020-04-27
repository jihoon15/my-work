shell.exe : main.o start.o shell.o
	gcc -o shell.exe main.o start.o shell.o

main.o : main.c
	gcc -c -o main.o main.c
start.o : start.c
	gcc -c -o start.o start.c
shell.o : shell.c
	gcc -c -o shell.o shell.c
clean :
	rm *.o shell.exe

