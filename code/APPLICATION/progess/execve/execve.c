#include <stdio.h>
#include <unistd.h>

int main(void){
    char *argv[] = {"./execve_pre", NULL};
    char *envp[] = {NULL};
    int a = execve(argv[0], argv, envp);
    if(a == -1){
        fprintf(stderr, "error\n");
    }
    return 0;
}