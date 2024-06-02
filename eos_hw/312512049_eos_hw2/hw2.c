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

#define MAX_FOOD_ITEMS 6
#define MAX_ORDER_LEN 256
#define MAX_CALLBACK_LEN 256
#define MAX_IP_LEN 16

struct food {
    int index;
    int shop_index;
    int price;
    const char *name;
};

struct TcpThread {
    int connfd;
    int shop_index;
    char buffer[256];
    char callback[256];
    struct food food_list[MAX_FOOD_ITEMS];
    int order_list[MAX_FOOD_ITEMS];
};

int store_dist[] = {3, 5, 8};

struct food food_list[] = {
    {0, 0, 60, "cookie"},   
    {1, 0, 80, "cake"},   
    {2, 1, 40, "tea"},   
    {3, 1, 70, "boba"},   
    {4, 2, 120, "fried-rice"},  
    {5, 2, 50, "Egg-drop-soup"} 
};

const char *get_food_name(int index) {
    for (size_t i = 0; i < sizeof(food_list) / sizeof(food_list[0]); ++i) {
        if (food_list[i].index == index) {
            return food_list[i].name;
        }
    }
    return "Unknown";
}

int get_shop_index(const char *food_name) {
    for (size_t i = 0; i < sizeof(food_list) / sizeof(food_list[0]); ++i) {
        if (strcmp(food_name, food_list[i].name) == 0) {
            return food_list[i].shop_index;
        }
    }
    return -1;
}

int get_index(const char *food_name) {
    for (size_t i = 0; i < sizeof(food_list) / sizeof(food_list[0]); ++i) {
        if (strcmp(food_name, food_list[i].name) == 0) {
            return food_list[i].index;
        }
    }
    return -1;
}

void errexit(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void callback_order_list(struct TcpThread *thread) {
    int count = 0;
    char temp_key[50];
    thread->callback[0] = '\0';

    for (int i = 0; i < MAX_FOOD_ITEMS; i++) {
        if (thread->order_list[i]) {
            if (count) strcat(thread->callback, "|");
            const char *food_name = get_food_name(i);
            sprintf(temp_key, "%s %d", food_name, thread->order_list[i]);
            strcat(thread->callback, temp_key);
            count++;
        }
    }
    strcat(thread->callback, "\n");
}

void *tcp_thread_main(void *arg) {
    struct TcpThread *thread = (struct TcpThread *)arg;

    while (1) {
        char *temp;
        char *action_vec[3] = {NULL};
        int i = 0;

        if (read(thread->connfd, thread->buffer, sizeof(thread->buffer)) < 0){

            errexit("Error : read()\n");
            close(thread->connfd);
            free(thread);
            return NULL;
        }

        printf("buffer: %s\n", thread->buffer);

        if (!strlen(thread->buffer)) break;

        temp = strtok(thread->buffer, " ");

        while (temp && i < 3) {
            action_vec[i++] = temp;
            temp = strtok(NULL, " ");
        }

        char *action = action_vec[0];
        char *food_name = action_vec[1];

        if (strcmp(action, "order") == 0) {
            int number;
            int shop_index_temp = get_shop_index(food_name);

            sscanf(action_vec[2], "%d", &number);
            printf("shop_index_temp : %d\n",shop_index_temp);
            printf("thread->shop_index : %d\n",thread->shop_index);

            if (thread->shop_index < 0) {
                thread->shop_index = shop_index_temp;
            } else if ((thread->shop_index >= 0 ) && (thread->shop_index != shop_index_temp)) {
                printf("User can only order in one shop!!\n");
                callback_order_list(thread);
                if ((write(thread->connfd, thread->callback, sizeof(thread->callback)) == -1))
                    errexit("Error : write()\n");
                continue;
            }

            thread->order_list[get_index(food_name)] += number;
            callback_order_list(thread);
        } else if (strcmp(action, "confirm\n") == 0) {
            int total = 0;
            for (int i = 0; i < MAX_FOOD_ITEMS; i++) {
                total += thread->food_list[i].price * thread->order_list[i];
            }
            if(total == 0 ){
                sprintf(thread->callback, "Please order some meals\n");
            }
            else{
                sprintf(thread->callback, "Please wait a few minutes...\n");
                if ((write(thread->connfd, thread->callback, sizeof(thread->callback)) == -1))
                    errexit("Error : write()\n");

                printf("sleep for %d\n", store_dist[thread->shop_index]);
                sleep(store_dist[thread->shop_index]);

                sprintf(thread->callback, "Delivery has arrived and you need to pay %d$\n", total);

                if ((write(thread->connfd, thread->callback, sizeof(thread->callback)) == -1))
                    errexit("Error : write()\n");
                break;
            }
        } else if (strcmp(action, "cancel") == 0) {
            break;
        } else if (strcmp(action, "shop")== 0){
            sprintf(thread->callback, "Dessert shop:3km\n- cookie:$60|cake:$80\nBeverage shop:5km\n- tea:$40|boba:$70\nDiner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
        }else break;

        if ((write(thread->connfd, thread->callback, sizeof(thread->callback)) == -1))
            errexit("Error : write()\n");

        memset(thread->buffer, 0, sizeof(thread->buffer));
        memset(thread->callback, 0, sizeof(thread->callback));
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
    int server_socket, client_socket;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);

    // Define server struct variable
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen 
    listen(server_socket, 10);
    printf("Server listening on port %d\n", port);

    while ((client_socket = accept(server_socket, (struct sockaddr *)&client, &client_len))) {
        pthread_t client_thread;
        struct TcpThread *new_thread = malloc(sizeof(struct TcpThread));
        if (new_thread == NULL) {
            perror("Failed to allocate memory for new thread");
            close(client_socket);
            continue;
        }
        new_thread->connfd = client_socket;
        new_thread->shop_index = -1;
        memset(new_thread->buffer, 0, sizeof(new_thread->buffer));
        memset(new_thread->callback, 0, sizeof(new_thread->callback));
        memset(new_thread->order_list, 0, sizeof(new_thread->order_list));
        memcpy(new_thread->food_list, food_list, sizeof(food_list));

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

    close(server_socket);
    printf("Server closed\n");
    return 0;
}