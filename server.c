#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

int player_count = 0;
pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

/*
 * Socket Read Functions
 */

/* Reads an int from a client socket. */
int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) /* Not what we were expecting. Client likely disconnected. */
        return -1;
    
    return msg;
}

/*
 * Socket Write Functions
 */

/* Writes a message to a client socket. */
void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
}

/* Writes an int to a client socket. */
void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}

/* Writes a message to both client sockets. */
void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

/* Writes an int to both client sockets. */
void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

/*
 * Connect Functions
 */

/* Sets up the listener socket. */
int setup_listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    /* Get a socket to listen on */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening listener socket.");
    

    memset(&serv_addr, 0, sizeof(serv_addr));
    
	/*server info */
    serv_addr.sin_family = AF_INET;	
    serv_addr.sin_addr.s_addr = INADDR_ANY;	
    serv_addr.sin_port = htons(portno);		

    /* Bind the server info to the listener socket. */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding listener socket.");

    #ifdef DEBUG
    printf("[DEBUG] Listener set.\n");    
    #endif 

    /* Return the socket number. */
    return sockfd;
}

/* Obtiene los clientes . */
void get_clients(int lis_sockfd, int * cli_sockfd)
{
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    
    /* Escucha a los dos clientes */
    int num_conn = 0;
    while(num_conn < 2)
    {
        /* Listen client. */
	    listen(lis_sockfd, 253 - player_count);
        
        memset(&cli_addr, 0, sizeof(cli_addr));

        clilen = sizeof(cli_addr);
	
	    /* Accept the connection from the client. */
        cli_sockfd[num_conn] = accept(lis_sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
        if (cli_sockfd[num_conn] < 0)
            
            error("ERROR al aceptar la conexión del cliente");

        
        /*client ID. */
        write(cli_sockfd[num_conn], &num_conn, sizeof(int));
        
        /* Incremento del contador de jugadores. */
        pthread_mutex_lock(&mutexcount);
        player_count++;
        printf("Numero de jugadores conectados: %d.\n", player_count);
        pthread_mutex_unlock(&mutexcount);

        if (num_conn == 0) {
            /* Envia "HLD" al primer cliente, hasta que este conectado un segundo player */
            write_client_msg(cli_sockfd[0],"HLD");
            
        }

        num_conn++;
    }
}

int get_player_move(int cli_sockfd)
{
     /* Get players move. */
    return recv_int(cli_sockfd);
} 


void *run_game(void *thread_data){
    
    int *cli_sockfd = (int*)thread_data; /* Client sockets. */
    printf("Game on!\n");

    /* Send the start message. */
    write_clients_msg(cli_sockfd, "SRT");

    #ifdef DEBUG
    printf("[DEBUG] Sent start message.\n");
    #endif

    int game_over = 0;
    int point_count = 0;
    int move = 0, move2=0;

    while(!game_over) {

        //Aqui se obtiene el movimiento de la pelota y se escribe en ambos clientes 
        move = get_player_move(cli_sockfd[0]);
        write_client_int(cli_sockfd[0], move);
        write_client_int(cli_sockfd[1], move);





    }
    printf("Game over.\n");

	/* Close client sockets */
    close(cli_sockfd[0]);
    close(cli_sockfd[1]);
}



/* 
 * Main Program
 */

int main(int argc, char *argv[])
{   
    if (argc < 2) {
        fprintf(stderr,"ERROR, ingrese puerto 7777\n");
        exit(1);
    }
    
    printf(" Servidor iniciado, esperando jugadores...\n ");
    int lis_sockfd = setup_listener(atoi(argv[1])); /* Listener socket. */
    pthread_mutex_init(&mutexcount, NULL);

    while (1) {
        if (player_count <= 252) {   
            
            int *cli_sockfd = (int*)malloc(2*sizeof(int)); /* Client sockets */
            memset(cli_sockfd, 0, 2*sizeof(int));  

            /* obtiene dos clientes conectados */
            get_clients(lis_sockfd, cli_sockfd);

            pthread_t thread; 
            int result = pthread_create(&thread, NULL, run_game, (void *)cli_sockfd);
            if (result){
                printf("Error %d\n", result);
                exit(-1);
            }
        
        }
    }

    close(lis_sockfd);

    pthread_mutex_destroy(&mutexcount);
    pthread_exit(NULL); 
}
