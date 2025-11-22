#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(void){
    char *pipe_path = "/tmp/myfifo";
    int fd = open(pipe_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open failed");
        exit(EXIT_FAILURE);
    }
    char buf_receive[100] = {0};
    int num = read(fd, buf_receive, sizeof(buf_receive));
    fprintf(stdout, "receive %d bytes\n", num);
    write(STDOUT_FILENO, buf_receive, num);

    return 0;
}