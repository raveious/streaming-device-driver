#include <fcntl.h>
#include <stdio.h>

#define BUFFER_SIZE 1

int main(int argc, char const *argv[])
{
    char buffer[BUFFER_SIZE];
    int fd = open("/dev/myStreamer", O_RDONLY);
    size_t written = 0;

    // Init buffer
    memset(buffer, 0x0, BUFFER_SIZE);

    // Double check that the driver exists...
    if (fd == NULL) {
        printf("Failed to open the device.\n");
        return -1;
    }

    // Read from module into userspace
    while (1 == 1) {
        written = read(fd, buffer, BUFFER_SIZE);

        for (unsigned int i = 0; i < written, i++)
        {
            printf("0x%02X\n", buffer[i]);
        }
    }

    close(fd);

    return 0;
}
