#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// Send SIGUSR1 signal
void send_signal(int pid) {
    if (kill(pid, SIGUSR1) == -1) {
        perror("kill");
        exit(EXIT_FAILURE);
    }
}



void main(int argc, char** argv) {
	if (argc != 4){
		printf("Usage: ./final_client <ip> <port> <pid>");
		exit(EXIT_FAILURE);
	}
	char *ip = argv[1];
	int port = atoi(argv[2]);    // string to int
	int server_pid = atoi(argv[3]);
	
	int straight_car = 1;
	int horizontal_car = 1;
	
    int client_socket;
    char buffer[256];
    struct sockaddr_in server_addr;

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // transform ip to binary
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("error ip");
        exit(EXIT_FAILURE);
    }

    // connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }else printf("connect successfully, ip = %s, port = %d\n", ip, port);
    
      
    int i;
	char ch;
 while (1) {
        ch = getchar();


        memset(buffer, 0, sizeof(buffer));

        if (ch == 'w') {
            send_signal(server_pid);
        } else if (ch == 'a') {
            straight_car++;
            snprintf(buffer, sizeof(buffer), "straight %d", straight_car);
        } else if (ch == 'z') {
            straight_car--;
            snprintf(buffer, sizeof(buffer), "straight %d", straight_car);
        } else if (ch == 'd') {
            horizontal_car++;
            snprintf(buffer, sizeof(buffer), "horizontal %d", horizontal_car);
        } else if (ch == 'c') {
            horizontal_car--;
            snprintf(buffer, sizeof(buffer), "horizontal %d", horizontal_car);
        } else {
            continue; 
        }

        if (strlen(buffer) > 0) {
            if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
                perror("send error");
                close(client_socket);
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
}
    
   