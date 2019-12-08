all: pong server

pong: pong.c 
	gcc pong.c -lncurses -o pong

server: server.c
	gcc -pthread server.c -o server

clean:
	rm -rf pong server
