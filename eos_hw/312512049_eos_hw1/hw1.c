#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdbool.h>

#define DEVICE_PATH "/dev/etx_device"

typedef struct{
    char name[30];
    int value;
    int num;
}item;

typedef struct{
    char name[30];
    int distance;
    item item[2];
}restaurant;

restaurant res_list[3] = {
    {"Dessert shop", 3, {{"cookie", 60, 0},{"cookie", 80, 0}}},
    {"Beverage shop", 5, {{"tea", 40, 0},{"boba", 70, 0}}},
    {"Diner", 8, {{"fired rice", 120, 0},{"egg-drop", 50, 0}}}
};

static bool A = true;

int set_gpio(char *str) {
    int fd;

    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open the device file");
        return -1;
    }
        
    // Write to the device file
    if (write(fd, str, sizeof(str)) < 0) {
        perror("Failed to write to the device file");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

// main menu
int mainlist(){
    int value = 0;
    char c;
    printf("---------------------------------\n");
    printf("1. shop list \n");
    printf("2. order \n");
    printf("---------------------------------\n");
    if(A){
        A = false;
    }
    else{
        while ((c = getchar()) != '\n' && c != EOF);
    }

    c = getchar();
    while(c < '1' || c > '2' ){
        printf("invalid choice, please try again");
        printf("\n");
        while ((c = getchar()) != '\n' && c != EOF);
        c = getchar();
    }
    value = c - '0';
    return value;
}

//show restaurant 
void shopinfo(){
    int num = sizeof(res_list)/sizeof(res_list[0]);
    printf("---------------------------------\n");
    for(int i=0 ; i<num; i++){
        printf("%s: %d km \n", res_list[i].name, res_list[i].distance);
    }
    printf("Press ENTER to exit\n");
    printf("---------------------------------\n");
    A = true;
    getchar();
    getchar();
}

// choose restaurant
int order(){
    char choice;
    int value = 0;
    int num = sizeof(res_list)/sizeof(res_list[0]);
    printf("---------------------------------\n");
    printf("please choose from 1~3\n");
    for(int i=0 ; i<num; i++){
        printf("%d. %s\n", i+1, res_list[i].name);
    }
    printf("---------------------------------\n");

    while ((choice = getchar()) != '\n' && choice != EOF);
    choice = getchar();
    while(choice < '1' || choice > ('1'+num-1) ){
        printf("invalid input, input number again \n");
        printf("\n");
        while ((choice = getchar()) != '\n' && choice != EOF);
        choice = getchar();
    }
    value = choice - '1';
    return value;
}

// begin to order 
int shoplist(int n){
    char choice;
    int value;
    int total_price = 0;
    int num = 0;
    restaurant res = res_list[n];
    int len = sizeof(res.item)/sizeof(res.item[0]);

    while(1){
        printf("---------------------------------\n");
        for(int i = 0 ; i < len; i++){
            printf("%d. %s: $%d\n" ,i+1 ,res.item[i].name, res.item[i].value);
        }
        printf("%d. confirm\n",len+1);
        printf("%d. cancel\n",len+2);
        printf("---------------------------------\n");
        while ((choice = getchar()) != '\n' && choice != EOF);
        choice = getchar();
        while(choice < '1' || choice > ('1' + len + 1) ){
            printf("invalid input, input number again \n");
            printf("\n");
            while ((choice = getchar()) != '\n' && choice != EOF);
            choice = getchar();
        }
        value = choice - '1';
        if(value < len){
            printf("---------------------------------\n");
            printf("How many\n");
            scanf("%d",&num);
            printf("---------------------------------\n");
            printf("choose %d %s\n", num, res.item[value].name);
            total_price += (res.item[value].value * num);
            printf("current total price is %d \n", total_price);
        }
        else if(value == len){
            return total_price;
        }
        else{
            return -1;
        }
    }

}

void main(){
    int num;
    int res_num = 0;
    int price =0;
    int distance = 0;

    while(1){
        char str1[20] ; 
        char str2[20] ;
        num = mainlist();
        switch(num){
            case 1:
                shopinfo();
                break;
            case 2:
                res_num = order();
                distance = res_list[res_num].distance;
                price = shoplist(res_num);
                if(price < 0){
                    break;
                }
                printf("​​​​Please wait for a few minutes...\n\n");
                // convert distance and price
                snprintf(str1, sizeof(str1), "%d", distance);
                snprintf(str2, sizeof(str2), "%d", price);
                strcat(str1, str2);
                set_gpio(str1);

                printf("​​​​please pick up your meal\n\n");
                printf("Press ENTER to exit\n");
                A = true;
                getchar();
                getchar();
                break;
            default:
                printf("invalid choice, please try again\n");
        }
    }
}