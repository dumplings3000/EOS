#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>

#define DEVICE_PATH "/dev/my_dev"
#define minmum_time 3
#define congestion_limit 5 // if congestion is bigger than this value, pedestrain can't switch traffic light

// device parameter 
int fd;
char driver_buffer[256];
int flag = 1;
int signalusr1_handle = 0;

// traffic parameter
int traffic_flow_str = 1;
int traffic_flow_hor = 1;
int congestion_degree_str = 20;
int congestion_degree_hor = 20;
int waiting_time = 0;
int direct; // 1 is horizon , 2 is straight , and 0 is the pedestrian traffic;

pthread_mutex_t mutex;

// thread struct
struct TcpThread {
    int connfd;
    char buffer[256];
};

int compare_bigger(int a, int b){
    return (a > b) ? a : b;
}

int compare_smaller(int a, int b){
    return (a < b) ? a : b;
}

int caluate_average(int a, int b){
    return (a+b)/2;
}

// change the congestion degree
void congestion(){
    if(direct == 2){
        congestion_degree_str -= traffic_flow_str;
        congestion_degree_hor += traffic_flow_hor;
    }else if(direct == 1){
        congestion_degree_str += traffic_flow_str;
        congestion_degree_hor -= traffic_flow_hor;
    }else{
        congestion_degree_str += traffic_flow_str;
        congestion_degree_hor += traffic_flow_hor;
    }
}

// Calculate traffic light time
void caluate_time(){
    int upper_bound = compare_bigger(congestion_degree_hor,congestion_degree_str);
    int lower_bound = compare_smaller(congestion_degree_hor,congestion_degree_str);
    waiting_time = compare_bigger((upper_bound - caluate_average(upper_bound,lower_bound))/2, minmum_time);
    if((waiting_time%10) !=0){
        waiting_time = waiting_time/10 + 1;
    }else{
        waiting_time = waiting_time/10;
    }
}

// process ctrl+c to destroy memory space
void signal_handler(){
    flag = 0;
}

// timer handler
void timer_handler(){
    snprintf(driver_buffer, sizeof(driver_buffer), "%d %d", direct, waiting_time);
    if (write(fd, driver_buffer, sizeof(driver_buffer)) < 0) {
        perror("Failed to write to the device file");
        close(fd);
        exit(EXIT_FAILURE);
    }
    memset(driver_buffer, 0, sizeof(driver_buffer));
    congestion();
    waiting_time--;
    if(signalusr1_handle == 1 && waiting_time == 0) signalusr1_handle = 0;
    return;
}

// process custom signal function
// handler the pedestrian traffic
void sigusr1_handler(){
    while(congestion_limit < compare_bigger(congestion_degree_hor, congestion_degree_str)){
        continue;
    }
    signalusr1_handle = 1;
    while(waiting_time ==0){
        direct = 0;
        waiting_time = 3;
    }
}

// Doing this function after create thread
// swich traffic light when time to goal and recalculate wauting time
void *policy(){
    while(1){
        if(waiting_time == 0 && signalusr1_handle ==0){
            caluate_time();
            if(congestion_degree_hor > congestion_degree_str) direct = 1;
            else if(congestion_degree_hor < congestion_degree_str) direct = 2;
            else{
                if(direct == 1) direct =2;
                else direct = 1;
            }
        }
    }
}

// change traffic light polic
void *tcp_thread_main(void *arg) {
    struct TcpThread *thread = (struct TcpThread *)arg;

    while (1) {
        char *temp;
        char *action_vec[2] = {NULL};
        int i = 0;
        int number;

        // read buffer
        if (read(thread->connfd, thread->buffer, sizeof(thread->buffer)) < 0){
            perror("Error : read()\n");
            close(thread->connfd);
            free(thread);
            return NULL;
        }
        printf("buffer: %s\n", thread->buffer);
        if (!strlen(thread->buffer)) break;
        temp = strtok(thread->buffer, " ");
        while (temp && i < 2) {
            action_vec[i++] = temp;
            temp = strtok(NULL, " ");
        }
        char *pass_dierct = action_vec[0];
        sscanf(action_vec[1], "%d", &number);
        
        // change traffic flow
        if (strcmp(pass_dierct, "straight") == 0) {
            traffic_flow_str = number;
        }else if (strcmp(pass_dierct, "horizontal") == 0) {
            traffic_flow_hor = number;
        }else break;
        memset(thread->buffer, 0, sizeof(thread->buffer));
    }
    close(thread->connfd);
    free(thread);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

// initialize mutex
    pthread_mutex_init(&mutex, NULL);

// creata main thread
    pthread_t main_thread;
    if (pthread_create(&main_thread, NULL, policy, NULL) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }

// create device
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open the device file");
        exit(EXIT_FAILURE);
    }

// create timer
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

// create signal
    // Register handler to signal
    struct sigaction sigusr1;
    memset(&sigusr1, 0 ,sizeof(struct sigaction));
    sigusr1.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sigusr1, NULL);

    signal(SIGALRM, timer_handler);
    signal(SIGINT, signal_handler);

// create socket
    // Create socket
    int server_socket, client_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);
    // Define server struct variable
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen 
    if(listen(server_socket, 10) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", port);

    // accept client data
    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &client_len))) {
        // create new thread use TCP_thread struct
        pthread_t client_thread;
        struct TcpThread *new_thread = malloc(sizeof(struct TcpThread));
        if (new_thread == NULL) {
            perror("Failed to allocate memory for new thread");
            close(client_socket);
            continue;
        }
        // initial thread parameter
        new_thread->connfd = client_socket;
        memset(new_thread->buffer, 0, sizeof(new_thread->buffer));
        // create client thread
        if (pthread_create(&client_thread, NULL, tcp_thread_main, (void *)new_thread) != 0) {
            perror("Could not create thread");
            free(new_thread);
            close(client_socket);
            continue;
        }
        pthread_detach(client_thread);
    }
    if (client_socket < 0) {
        perror("Accept failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

// pause
    while(flag){
        pause();
    }

// finish
    pthread_detach(main_thread);
    close(fd);
    printf("device closed\n");
    close(server_socket);
    printf("Server closed\n");
    return 0;
}