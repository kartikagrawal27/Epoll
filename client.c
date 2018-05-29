#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define MSG_SIZE 2
#define PROBABILITY 10
char false_msg[2] = "F";
char correct_msg[2] = "T";

// gcc client.c -w -std=gnu99 -pthread -o client

void write_to_server(int server_fd) {
    srand(time(NULL));
    while(1) {
        int drawn = rand()%PROBABILITY;
        // with probability 1/40
        if ( drawn == 1){
            write(server_fd, correct_msg, 2);
            break;
        }
        else
            write(server_fd, false_msg, 2);
    }
}
int main(int argc, char **argv)
{
	if (argc < 3){
		fprintf(stderr, "./client <host> <port>\n");
		exit(1);
	}
    int addr;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1){
    	perror("socket");
    	exit(1);
    }
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    addr = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (addr != 0) {
        fprintf(stderr, "%s\n", gai_strerror(addr));
        exit(1);
    }

    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1){
        if ( errno != ECONNREFUSED){
        	perror("connect");
        	exit(1);
        }
        else {
	    perror("connect");
            fprintf(stderr, "Connection Refused\n");
	    exit(1);
        }
    }


    write_to_server(sock_fd);

    char read_buffer[MSG_SIZE];
    memset(read_buffer,0,MSG_SIZE);
	ssize_t count = read(sock_fd, read_buffer, MSG_SIZE);
	if ( count < 0)
		perror("read");
    freeaddrinfo(result);
    close(sock_fd);
    return 0;
}
