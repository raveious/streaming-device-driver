#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 2

volatile bool terminated = false;

void signal_handler(int sig) {
    printf("Exiting program (%d)...\n", sig);

    terminated = true;
}

int main(int argc, char const *argv[])
{
    char buffer[BUFFER_SIZE];
    ssize_t written = 0;
    struct timespec ts;
    int foo = 0;

    ts.tv_sec = 2;
    ts.tv_nsec = 0;

    signal(SIGINT, signal_handler);

    int fd = open("/dev/myStreamer", O_RDONLY);

    // Init buffer
    memset(buffer, 0x0, BUFFER_SIZE);

    // Double check that the driver exists...
    if (fd < 0) {
        printf("Failed to open the device.\n");
        return -1;
    }

    // Read from module into userspace
    while (!terminated) {
        written = read(fd, buffer, BUFFER_SIZE);

        printf("Recieved %d bytes from device.\n", written);

        if (written < 0) {
            printf("Read failed.\n");
            break;
        }

        for (unsigned int i = 0; i < written; i++) {
            printf("0x%02X\n", buffer[i]);
        }
        
        nanosleep(&ts, NULL);
    }

    close(fd);

    return 0;
}
