prog: main.o fibonacci.o
	gcc main.o fibonacci.o -o prog

main.o: main.c
	gcc -c main.c -o main.o

fibonacci.o: fibonacci.c
	gcc -c fibonacci.c -o fibonacci.o