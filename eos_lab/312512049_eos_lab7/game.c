#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int guess;
    char result[8];
} data;

data *shm;
int target_number;
int flag = 1;

// process ctrl+c to clear share memory space
void signal_handler(){
    flag = 0;
}

// customize signal handler 1
void sigusr1_handler(int signum) {
    int number = shm->guess;
    if (number > target_number) {
        strcpy(shm->result, "bigger");
    } else if (number < target_number) {
        strcpy(shm->result, "smaller");
    } else if(number == target_number) {
        strcpy(shm->result, "bingo");
        
    }else{
        printf("error\n");
    }
    printf("[game] Guess %d, %s\n", shm->guess, shm->result);
}

int main(int argc, char *argv[]) {
// accept arg papameter
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <key> <guess>\n", argv[0]);
        exit(1);
    }
    
    key_t key = atoi(argv[1]);
    target_number = atoi(argv[2]);
    int shmid;

    struct sigaction sigusr1;

// share memory
    // create the segment
    if((shmid = shmget(key, sizeof(data), 0666 | IPC_CREAT)) < 0){
        perror("shmget");
        exit(1);
    }
    // attach the segment to data space
    if((shm = (data *)shmat(shmid, NULL, 0)) == (data *) -1){
        perror("shmat");
        exit(1);
    }
    shm->guess = 0;
    strcpy(shm->result,"unstart");

// signal
    // Register handler to signal
    memset(&sigusr1, 0 ,sizeof(struct sigaction));
    sigusr1.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sigusr1, NULL);

// for ctrl+c to clean up
    signal(SIGINT, signal_handler);
    
    printf("[game] Game PID: %d.\n", getpid());
    
// ctrl + c
    while(flag){
        pause();
    };
    // detach share memory segment
    shmdt(shm);
    // free share memory segment
    if((shmctl(shmid,IPC_RMID, NULL)) < 0){
        fprintf(stderr, "Server remove share memory failed\n");
        exit(1);
    }

    return 0;
}
