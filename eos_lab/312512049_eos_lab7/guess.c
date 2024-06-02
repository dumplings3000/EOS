#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct {
    int guess;
    char result[8];
} data;

data *shm;
int upper_bound;
int lower_bound = 0;
pid_t game_pid;
int flag = 1;

void guess_number() {
    printf("[game] Guess: %d\n", shm->guess);
    kill(game_pid, SIGUSR1);
    return;
}

void timer_handler(int signum) {
    if(strcmp(shm->result, "bigger") == 0){
        upper_bound = shm->guess;
        shm->guess = (shm->guess + lower_bound)/2;
    }else if(strcmp(shm->result, "smaller") == 0){
        lower_bound = shm->guess;
        shm->guess = (shm->guess + upper_bound)/2;
    }else if(strcmp(shm->result, "bingo") == 0){
        flag = 0;
        return;
    }else{
        printf("error\n");
        return;
    }
    guess_number();
    return;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <key> <upper_bound> <pid>\n", argv[0]);
        exit(1);
    }

    key_t key = atoi(argv[1]);
    upper_bound = atoi(argv[2]);
    game_pid = atoi(argv[3]);
    int shmid;

// share memory
    // locate the segment
    if((shmid = shmget(key,sizeof(data), 0666)) < 0){
        perror("shgmet");
        exit(1);
    }
    // attach the segment to data space
    if((shm = (data *)shmat(shmid, NULL, 0)) == (data *) -1){
        perror("shmat");
        exit(1);
    }
    shm->guess = (upper_bound + lower_bound)/2;
    guess_number();

// timer
    struct itimerval timer;
    struct sigaction sa;
    // timer handler create
    memset(&sa,0 ,sizeof(struct sigaction));
    sa.sa_handler = timer_handler;
    sigaction(SIGALRM, &sa, NULL);
    //  
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    // 
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    signal(SIGALRM, timer_handler);

    while (flag){
        pause();
    };

    return 0;
}
