//Librería que permite escribir interfaces basadas en texto, 
#include <ncurses.h>

//Librerias para el Socket 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef struct{short int x, y, c; bool movhor, movver;} object;

//Logica del socket

/*
 * Conexión socket 
 */

void error(const char *msg)
{
    #ifdef DEBUG
    perror(msg);
    #else
    printf("Either the server shut down or the other player disconnected.\nGame over.\n");
    #endif 

    exit(0);
}

/*
 * Socket Functions
 */

/* Lee mensajes char del servidor. */
void recv_msg(int sockfd, char * msg)
{
    /* All messages are 3 bytes. */
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3) /* Not what we were expecting. Server got killed or the other client disconnected. */ 
        error("ERROR reading message from server socket.");

    #ifdef DEBUG
    printf("[DEBUG] Received message: %s\n", msg);
    #endif 
}

/* Lee mensajes int del servidor*/
int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
    #ifdef DEBUG
    printf("[DEBUG] Received int: %d\n", msg);
    #endif 
    
    return msg;
}


/* Escribe enteros en el servidor. */
void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
}

/*
 * Connect Functions
 */

/* Set up server. */
int connect_to_server(char * hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    /* Get socket. */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) 
        error("ERROR opening socket for server.");
	
    /* obtiene la dirección del server */
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
	
	
	memset(&serv_addr, 0, sizeof(serv_addr));

	/* Set up server info. */
    serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

	/*Hace la conexión */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting to server");

    return sockfd;
}



//Lógica del juego

int main(int argc, char *argv[]){
    
  char msg[4];
  
  //Se conecta al socket y recive un ID   
  int sockfd = connect_to_server(argv[1], atoi(argv[2]));
  int id = recv_int(sockfd);

  object scr; int i = 0,cont=0; bool end=false;
  
  /*Espera a los dos player antes de empezar.*/
    do {
        recv_msg(sockfd, msg);
        if (!strcmp(msg, "HLD"))
            mvprintw(1,1,"Esperando al segundo jugador...\n");
    } while ( strcmp(msg, "SRT") );

  initscr();
  start_color();
  init_pair(1,COLOR_BLUE,COLOR_BLACK);
  keypad(stdscr,true);
  noecho();
  curs_set(0);
  getmaxyx(stdscr,scr.y,scr.x);
  object b1={scr.x-2,scr.y/2,0,false,false},
         b2={1,scr.y/2,0,false,false},
         b={scr.x/2,scr.y/2,0,false,false};
  mvprintw(5,2,
               "\n"
               "\t \t\tPlayer 1 'A' y 'Q'     \n"
               "\t \t\tPlayer 2  'UP y 'DOWN' \n"
               "\t \t\t'P' para pausar y ESC para salir", argv[1]); 

  mvprintw(1,1,"\t ID: %i \n" 
               "\t Puerto: %s \n"
               "\t Servidor: %s \n", id, argv[2], argv[1]);   
  getch();

  

  /*
  Logica de movimiento de la pelota y barras de jugadores
  */
  for (nodelay(stdscr,1); !end; usleep(4000)) {
    if (++cont%16==0){
      if ((b.y==scr.y-1)||(b.y==1))
        b.movver=!b.movver;
      if ((b.x>=scr.x-2)||(b.x<=2)){
        b.movhor=!b.movhor;
        if ((b.y==b1.y-1)||(b.y==b2.y-1)) {
          b.movver=false;
        } else if ((b.y==b1.y+1)||(b.y==b2.y+1)) {
          b.movver=true;
        } else if ((b.y != b1.y) && (b.y != b2.y)) {
          (b.x>=scr.x-2) ? b1.c++: b2.c++;
          b.x=scr.x/2;
          b.y=scr.y/2;
        }
      }
      b.x=b.movhor ? b.x+1 : b.x-1;
      b.y=b.movver ? b.y+1 : b.y-1;

      if (b1.y<=1)
        b1.y=scr.y-2;
      if (b1.y>=scr.y-1)
        b1.y=2;
      if (b2.y<=1)
        b2.y=scr.y-2;
      if (b2.y>=scr.y-1)
        b2.y=2;
    }

    // aqui se obtiene la posición en coordenadas (x,y) de la pelota y se manda a ambos clientes 
    write_server_int(sockfd, b.y);
    int pos_y = recv_int(sockfd);
    write_server_int(sockfd, b.x); 
    int pos_x = recv_int(sockfd);
    
    
    /*

    Lógica del movimiento de las barras en cada jugador 
    Aquí deberíamos aplicar socket en el movimiento que hace cada jugador 
    
    */
    switch (getch()) {
   
      case KEY_DOWN: b1.y--; break;
      case KEY_UP: b1.y++; break;

      case 'q':      b2.y--; break;
      case 'a':      b2.y++; break;

      case 'p':      getchar(); break;

      case 0x1B:    endwin(); end++;  close(sockfd); break; //cuanto termina, cierra el socket 
    }
    erase();

    mvprintw(2,scr.x/2-2,"%i | %i",b1.c,b2.c); //Imprime el marcador de puntos 
    mvvline(0,scr.x/2,ACS_VLINE,scr.y); //Crea la linea central 
    attron(COLOR_PAIR(1));    
    mvprintw(pos_y,pos_x ,"o"); //Imprime la pelota de ping pong
    
    /*

    Genera las barras de cada jugador 

    */
    
    for(i=-1;i<=2;i++){
      mvprintw(b1.y+i,b1.x,"|");
      mvprintw(b2.y+i,b2.x,"|");}

    attroff(COLOR_PAIR(1));
  }
}