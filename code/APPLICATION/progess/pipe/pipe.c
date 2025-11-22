#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void){

    int pipdes[2];
    int a = pipe(pipdes);
    if(a == -1){
        fprintf(stdout, "error\n");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if(pid < 0){
        fprintf(stdout, "error\n");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0){
        char buf2[100] = {0};
        close(pipdes[1]);
        fprintf(stdout, "read %ld bytes:\n", read(pipdes[0], buf2, sizeof(buf2)));
        write(STDOUT_FILENO, buf2, sizeof(buf2));
        fprintf(stdout, "\n");
    }
    else{
        char buf[] = {"Hello world"};
        close(pipdes[0]);
        int i = 0;
        while(buf[i] != '\0'){
            write(pipdes[1], &buf[i], 1);
            i++;
        }
        fprintf(stdout, "write %d bytes:\n", i);
    }
    return 0;
}