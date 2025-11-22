#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char* buf;

void *read_(void *argv){
    char temp;
    int i, m;
    while(1){
        temp = fgetc(stdin);
        
        if(temp && temp != '\n'){
            buf[i++] = temp;

            // for(m=0; m<i; m++){
            //     fputc(buf[m], stdout);
            // }
            write(STDOUT_FILENO, buf, i);

            // temp = 0;
        }
    }
}

int main(void){
    buf = malloc(1024);
    pthread_t tid;
    pthread_create(&tid, NULL, read_, NULL);
    
    pthread_join(tid, NULL);
    return 0;
}