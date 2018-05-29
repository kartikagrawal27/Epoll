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

#define MAX_EVENTS 60
#define MSG_SIZE 2

struct epoll_event read_event;
struct epoll_event write_event;

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

void epoll_add_read_fd(int epoll_fd, int read_fd) {
    read_event.data.fd = read_fd;
    read_event.events = EPOLLIN;
    if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, read_fd, &read_event) == -1) {
        perror("epoll_ctl");
        exit(1);
    }
}
void epoll_mod_write(int epoll_fd, int write_fd) {
    write_event.data.fd = write_fd;
    write_event.events = EPOLLOUT;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, write_fd, &write_event);
}
void add_client(int epoll_fd, int socket_fd ) {
    while(1) {
        struct sockaddr client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd;

        client_fd = accept(socket_fd, &client_addr, &len);
        if ( client_fd == -1) {
            if ( (errno == EAGAIN) || (errno == EWOULDBLOCK ))
                break;
            else {
                perror("accept");
                exit(1);
            }
        }
        set_non_blocking(client_fd);
        epoll_add_read_fd(epoll_fd, client_fd);
    }
}

void read_from_client(int epoll_fd, int client_fd) {
    fprintf(stderr,"processing client\n");

    char msg_buf[MSG_SIZE];

    ssize_t count = read(client_fd, msg_buf, MSG_SIZE);
    if (count < 0 && errno == EAGAIN) {
        // done reading, do nothing
    }
    else if ( count >= 0) {
        if ( msg_buf[0] == 'T') {
            epoll_mod_write(epoll_fd, client_fd);
        }
    }
    else {
        // some error on read
        close(client_fd);
    }

    fprintf(stderr, "done reading\n");
}

void write_to_client(int epoll_fd, int client_fd) {
    fprintf(stderr,"writing to client\n");

    char msg_buf[MSG_SIZE] = "B";

    write(client_fd, msg_buf, MSG_SIZE );

    fprintf(stderr, "done writing\n");
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    
}

int main(int argc, char* argv[]) {

    int socket_fd;

    if (argc != 2) {
        print_usage();
        exit(1);
    }

    // set up TCP socket
    socket_fd = set_up_TCP(argv[1]);

    set_non_blocking(socket_fd);

    if ( listen(socket_fd,  SOMAXCONN) != 0 ) {
        perror(NULL);
        exit(1);
    }

    // create epoll fd
    int epoll_fd = epoll_create1(0);

    epoll_add_read_fd(epoll_fd, socket_fd);

    struct epoll_event *ready_events;
    ready_events = (struct epoll_event*) calloc(MAX_EVENTS, sizeof(struct epoll_event));

    while(1) {
        int num_ready = epoll_wait ( epoll_fd, ready_events, MAX_EVENTS, -1);
        if ( num_ready == -1) {
            perror("epoll_wait");
            exit(1);
        }

        int i = 0;
        for ( i = 0 ; i < num_ready; i++) {
            int ready_fd = ready_events[i].data.fd;
            uint32_t ready_event = ready_events[i].events;
            if ( (ready_event & EPOLLERR) ||
                 (ready_event & EPOLLHUP)) {
    	      close (ready_fd);
    	      continue;
    	    }
            else if (ready_fd == socket_fd) {
                // new client
                add_client(epoll_fd, socket_fd );
                continue;
            }
            // if it is a read event, read from client
            else if ( (ready_event| EPOLLIN) == 1){
                // read from client
                read_from_client( epoll_fd, ready_fd);
            }
            // if it is a write event, write to client
            else{
                // write to client
                write_to_client(epoll_fd, ready_fd);
            }
        }
    }

    free (ready_events);
    close (socket_fd);

    return 0;
}
