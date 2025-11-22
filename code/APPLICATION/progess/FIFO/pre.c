#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

int main(void){

    char *pipe_path = "/tmp/myfifo";
    if(mkfifo(pipe_path, 0664) != 0)
    {
        perror("mkfifo failed");
        if (errno != 17)
        {
        exit(EXIT_FAILURE);
        }
    }

    return 0;
}