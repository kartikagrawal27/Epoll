#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>

#define MAX_EVENTS 60
#define MSG_SIZE 2

void print_usage(){
    fprintf(stderr, "Usage: ./server [port]\n");
}

int set_up_TCP(char* port) {

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int addr = getaddrinfo(NULL, port, &hints, &result);
    if (addr != 0) {
        fprintf(stderr, "%s\n", gai_strerror(addr));
        exit(1);
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1) {
      perror(NULL);
      exit(1);
    }

    int optval = 1;
    if ( setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1){
    	perror(NULL);
    	exit(1);
    }

    if ( bind(socket_fd, result->ai_addr, result->ai_addrlen) != 0 ) {
        perror(NULL);
        exit(1);
    }
    freeaddrinfo(result);

    return socket_fd;
}


void read_from_client( int client_fd) {
    fprintf(stderr,"processing client\n");

    char msg_buf[MSG_SIZE];
    int done_reading = 0;
    while(!done_reading ){
        ssize_t count = read(client_fd, msg_buf, MSG_SIZE);
        if (count < 0 && errno == EAGAIN) {
            done_reading = 1;
        }
        else if ( count >= 0) {
            if ( msg_buf[0] == 'T')
                done_reading = 1;
        }
        else {
            // some error on read
            close(client_fd);
            done_reading = 1;
        }
    }


    fprintf(stderr, "done reading\n");
}


void write_to_client(int client_fd) {
    fprintf(stderr,"writing to client\n");

    int done_writing = 0;
    char msg_buf[MSG_SIZE] = "B";
    ssize_t nwrite = 0;
    while(!done_writing) {
        nwrite = write(client_fd, msg_buf, MSG_SIZE);
        if ( nwrite == -1) {
            if ( errno != EAGAIN)
                done_writing = 1;
        }
        else if (nwrite == MSG_SIZE)
            done_writing = 1;
    }
    fprintf(stderr, "done writing\n");
}

void* process_client(void* arg) {
    int client_fd = (intptr_t) arg;
    read_from_client(client_fd);
    write_to_client(client_fd);
    close(client_fd);
    return NULL;
}
void add_client(int socket_fd ) {
    while(1) {
        struct sockaddr client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd;
        fprintf(stderr, "Waiting for connection...\n");
        client_fd = accept(socket_fd, &client_addr, &len);
        if ( client_fd == -1) {
            if ( (errno == EAGAIN) || (errno == EWOULDBLOCK ))
                break;
            else {
                perror("accept");
                exit(1);
            }
        }
        fprintf(stderr, "Connection made: client_fd=%d\n", client_fd);

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        int ok = pthread_create(&tid,&attr, process_client, (void*) (intptr_t) client_fd);
        if(ok != 0) {
           perror("Could not create threads");
	   close(client_fd);
        }
    }
}


int main(int argc, char* argv[]) {

    int socket_fd;

    if (argc != 2) {
        print_usage();
        exit(1);
    }

    // set up TCP socket
    socket_fd = set_up_TCP(argv[1]);


    if ( listen(socket_fd,  SOMAXCONN) != 0 ) {
        perror(NULL);
        exit(1);
    }

    add_client(socket_fd);

    close (socket_fd);
    return 0;
}
