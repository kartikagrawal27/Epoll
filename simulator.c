#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if ( argc != 5) {
        fprintf(stderr, "Usage: ./simulator [client_exe] [addr] [port] [num_connections] \n");
        exit(1);
    }

    int num_connections = atoi(argv[4]);
    int already_connect = 0;
    int i = 0;

    for ( i = 0 ; i < num_connections ; i++) {
        int pid = vfork();
        if ( pid < 0) {
            perror("vfork\n");
            int j= 0;
            if (errno == EAGAIN) {
                for ( j = 0; j < already_connect ; j++) {
                    wait(NULL);
                }
                already_connect = 0;
                continue;
            }
            exit(1);
        }
        else if ( pid == 0) {
            int rev = execlp(argv[1],argv[1], argv[2], argv[3], NULL);
            perror("exec");
            exit(1);
        }
        else {
            already_connect++;
            // dummy
        }
    }

    for (i = 0 ; i < num_connections ; i++) {
        wait(NULL);
    }
    fprintf(stderr, "getting every body" );
    return 0;
}
