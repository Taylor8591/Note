#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(void){
    char *pipe_path = "/tmp/myfifo";
    fprintf(stdout, "opening\n");
    int fd = open(pipe_path, O_WRONLY);//注：这里如果以只读或只写打开，则会阻塞等到另一边以只写或只读打开
    if (fd == -1)
    {
        perror("open failed");
        exit(EXIT_FAILURE);
    }
    char buf_send[] = {"Hello world!\n"};
    write(fd, buf_send, sizeof(buf_send)-1);
    fprintf(stdout, "send %ld\n bytes", sizeof(buf_send)-1);
    return 0;
}