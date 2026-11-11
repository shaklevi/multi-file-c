prog: main.o int_parser.o
	gcc main.o int_parser.o -o prog

main.o: main.c
	gcc -c main.c -o main.o

int_parser.o: int_parser.c
	gcc -c int_parser.c -o int_parser.o