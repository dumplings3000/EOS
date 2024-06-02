#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/sem.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_SIZE 1024

int account_balance = 0;
int s;

// lock
int P(int s){
    struct sembuf sop;
    sop.sem_num = 0; 
    sop.sem_op = -1; 
    sop.sem_flg = 0; 
    if (semop (s, &sop, 1) < 0) {
        fprintf(stderr,"P(): semop_lock failed: %s\n",strerror(errno));
        return -1;
    } 
    else {
        return 0;
    }
}
// unlock
int V(int s){
    struct sembuf sop; 
    sop.sem_num = 0; 
    sop.sem_op = 1; 
    sop.sem_flg = 0; 
    if (semop(s, &sop, 1) < 0) {
        fprintf(stderr,"V(): semop_unlock failed: %s\n",strerror(errno));
        return -1;
    }
    else {
        return 0;
    }
}

// to clean semaphore when ctrl+c
void signal_handler(int signum) {
    if (signum == SIGINT) {
        if (semctl (s, 0, IPC_RMID, 0) < 0){
        fprintf (stderr, "unable to remove semaphore ");
        exit(1);
    }
    printf("Semaphore removed\n");
    exit(EXIT_SUCCESS);
    }
}

/* call back function
(when recieve the message from client , run this function)*/
void *handle_client(void *client_socket) {
    int sock = *(int *)client_socket;
    free(client_socket);
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        char operation[10];
        int amount, times;
        sscanf(buffer, "%s %d %d", operation, &amount, &times);

        for (int i = 0; i < times; i++) {         
            if (strcmp(operation, "deposit") == 0) {
                P(s);
                // critical section
                account_balance += amount;
                printf("After deposit: %d\n", account_balance);
                V(s);
                sleep(amount * 0.00001);
            } else if (strcmp(operation, "withdraw") == 0) {
                P(s);
                // critical section
                account_balance -= amount;
                printf("After withdraw: %d\n", account_balance);
                V(s);
                sleep(amount * 0.00002);
            }
        }
    }
    close(sock);
    return NULL;
}


int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
// check parameter number
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

// define global varible
    int port = atoi(argv[1]);
    int server_socket, client_socket, *new_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);
    long int key = 1234;

    // define server struct varible
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

// create semaphore
    s = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (s < 0){
        fprintf(stderr,"%s: creation of semaphore %ld failed: %s\n", argv[0],key, strerror(errno));
        exit(1);
    }
    if (semctl(s, 0, SETVAL, 1) < 0) {
        perror("semctl failed");
        exit(EXIT_FAILURE);
    }

// create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // bind
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    //listen 
    listen(server_socket, 10);
    printf("Server listening on port %d\n", port);
    // recieve message from client
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &client_len))) {
        // create new thread for client process
        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(client_socket);
            continue;
        }
        pthread_detach(client_thread);
    }

    if (client_socket < 0) {
        perror("Accept failed");
        close(server_socket);
        semctl(s, 0, IPC_RMID);
        exit(EXIT_FAILURE);
    } 
    close(server_socket);
    semctl(s, 0, IPC_RMID);
    printf("Semaphore removed\n");
    return 0;
}
