#include "stdlib.h"
#include "fcntl.h"
#include "sys/select.h"
#include "stdio.h"

int main(int argc, char const *argv[]) {
    int fd1 = open("./in1", O_RDONLY);
    int fd2 = open("./in2", O_RDONLY);
    int sum;
    while (1) {
        fd_set fds;
        int maxfd;
        int res;
        char buf[1024];
        int in1_done, in2_done;

        FD_ZERO(&fds); // Clear FD set for select
        FD_SET(fd1, &fds);
        FD_SET(fd2, &fds);
        printf("%d, %d", fd1, fd2);
        maxfd = fd1 > fd2 ? fd1 : fd2;
        printf("%d", maxfd);

        select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(fd1, &fds)) {
            // We can read from fd1
            res = read(fd1, buf, sizeof(buf));
            if (res > 0) {
                printf("fd1");
                printf("Buf: %s\n",buf);
                sum += atoi(buf);
                printf("Buf: %d\n", sum);
                continue;
            }
            else
            {
                in1_done = 1;
            }
        }
        if (FD_ISSET(fd2, &fds)) {
            // We can read from fd2
            res = read(fd2, buf, sizeof(buf));
            if (res > 0) {
                printf("fd2");
                printf("Buf: %s\n",buf);
                sum += atoi(buf);
                printf("Buf: %d\n", sum);
                continue;
            }
            else
            {
                in2_done = 1;
            }
        }
        if (in1_done > 0 & in2_done >0)
        {
            printf("%d\n", sum);
            exit(0);
        }
    }
    return 0;
}