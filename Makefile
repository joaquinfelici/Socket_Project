all: server client

server: server.o
	gcc -Wall   -Werror -pedantic   -o	server	server.o

server.o: server.c 
	gcc -Wall   -Werror -pedantic   -c	server.c

client: client.o
	gcc -Wall   -Werror -pedantic    -o	client	client.o

client.o: client.c
	gcc -Wall   -Werror -pedantic    -c	client.c

clean:
	rm *.o client server
