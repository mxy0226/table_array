all:main
main:main.c table_array.h table_array.o
	cc main.c table_array.h table_array.o -lpthread -o main
table_array.o:table_array.c
	cc -c table_array.c
clean:
	rm table_array.o main

