#include <stdio.h>
#include <unistd.h>

int main(void){

    int pid = fork();
    if(pid == 0){
        fprintf(stdout, "son:%d\n", getpid());
    }
    else{
        fprintf(stdout, "dad:%d\n", getpid());
    }
    return 0;
}