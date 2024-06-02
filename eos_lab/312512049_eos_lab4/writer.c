#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/my_dev"
#define BUFFER_SIZE 1

int main(int argc, char *argv[]) {
    int fd;
    char buffer[BUFFER_SIZE];
    int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return -1;
    }

    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open the device file");
        return -1;
    }

    for (i = 0; i < strlen(argv[1]); i++) {
        buffer[0] = argv[1][i];
                
        // Write to the device file
        if (write(fd, buffer, sizeof(buffer)) < 0) {
            perror("Failed to write to the device file");
            close(fd);
            return -1;
        }
        sleep(1);
    }

    close(fd);

    return 0;
}
