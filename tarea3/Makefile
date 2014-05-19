
server: main.c server.c defaults.h server.h tcp.c tcp.h debug.h
	gcc -g server.c tcp.c main.c  -I /home/israel/socketsProgramming/Practica1 -o server.o

$PHONY:
client:
	gcc -g client.c tcp.c -I /home/israel/socketsProgramming/Practica1 -o client.o

clean: 
	rm server.o

		
	
