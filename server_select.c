#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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

void set_non_blocking(int socket_fd) {
    int flag;
    flag = fcntl(socket_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(socket_fd, F_SETFL, flag);
}

void add_client(int socket, fd_set * active_read_set_ptr ) {
    int client_fd;
    struct sockaddr_in clientname;
    unsigned int size = sizeof (clientname);
    client_fd = accept (socket, (struct sockaddr *) &clientname, &size);
    if (client_fd < 0)
    {
        perror ("accept");
        exit (EXIT_FAILURE);
    }
    fprintf(stderr, "Connection made: client_fd=%d\n", client_fd);
    set_non_blocking(client_fd);
    FD_SET (client_fd, active_read_set_ptr);
}

// 1 success; -1 retry; 0 error, close it
int read_from_client(int client_fd) {
    fprintf(stderr,"reading from client\n");

    char msg_buf[MSG_SIZE];
    ssize_t count = read(client_fd, msg_buf, MSG_SIZE);
    int rev = 0;

    if (count < 0 && errno == EAGAIN) {
        // done reading, do nothing
        rev = -1;
    }
    else if ( count >= 0) {
        if ( msg_buf[0] == 'T') {
            rev = 1;
        }
        else
            rev = -1;
    }
    else {
        rev = 0;
    }

    fprintf(stderr, "done reading\n");
    return rev;
}

// 1 success; -1 error
int write_client(int client_fd) {
    fprintf(stderr,"writing client\n");
    char msg_buf[MSG_SIZE] = "B";

    int rev = 0;

    ssize_t nwrite = 0;
    nwrite = write(client_fd, msg_buf, MSG_SIZE);
    if ( nwrite == -1) {
        rev = -1;
    }
    else if (nwrite == MSG_SIZE)
        rev = 1;

    fprintf(stderr, "done writing\n");
    return rev;
}

int main(int argc, char* argv[]) {

    int socket_fd;


    if (argc != 2) {
        print_usage();
        exit(1);
    }
    socket_fd = set_up_TCP(argv[1]);

    set_non_blocking(socket_fd);
    if ( listen(socket_fd,  SOMAXCONN) != 0 ) {
        perror(NULL);
        exit(1);
    }

    fd_set active_read_set, read_fd_set, write_fd_set, active_write_set;
    int i;

    FD_ZERO (&active_read_set);
    FD_ZERO (&write_fd_set);
    FD_ZERO (&active_write_set);
    FD_ZERO (&read_fd_set);

    while (1) {

    	FD_SET (socket_fd, &active_read_set);
        read_fd_set = active_read_set;
        write_fd_set = active_write_set;
        if (select (FD_SETSIZE, &read_fd_set, &write_fd_set, NULL, NULL) < 0)
          {
            perror ("select");
            exit (EXIT_FAILURE);
          }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == socket_fd) {
                    add_client(socket_fd, &active_read_set);
                }
                else {
                    int rev;
                    rev= read_from_client(i);
                    if ( rev > 0 ) {
                        FD_CLR(i, &active_read_set);
                        FD_SET(i, &active_write_set);
                    }
                    else if (rev == 0) {
                        FD_CLR(i, &active_read_set);
                        close(i);
                    }
                    else {
                        FD_SET(i, &active_read_set);
                    }
                }
            }
            else if (FD_ISSET(i, &write_fd_set)) {
                write_client(i);
                FD_CLR(i, &active_write_set);
	        close(i);
            }
        }
    }


    close (socket_fd);

    return 0;
}
