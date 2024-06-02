#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
// check args number
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <ip> <port> <deposit/withdraw> <amount> <times>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

// use argv to set the varible
    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *operation = argv[3];
    int amount = atoi(argv[4]);
    int times = atoi(argv[5]);

// create socket
    int s;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("create socket failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);
    
// connecet to server
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        close(s);
        exit(EXIT_FAILURE);
    }
// send message to server
    snprintf(message, BUFFER_SIZE, "%s %d %d", operation, amount, times);

    if (send(s, message, strlen(message), 0) < 0) {
        perror("Send failed");
        close(s);
        exit(EXIT_FAILURE);
    }
    
// close
    close(s);
    return 0;
}
