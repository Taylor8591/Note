#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

// #define shmName "/sharename"

int main(void){

    char shmName[] = {"/sharename"};
    int fd = shm_open(shmName, O_CREAT | O_RDWR, 0644);
    if (fd < 0)
    {
        perror("共享内存对象开启失败!\n");
        exit(EXIT_FAILURE);
    }
    ftruncate(fd, 100);
    char* share = mmap(NULL, 100, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (share == MAP_FAILED)
    {
        perror("共享内存对象映射到内存失败!\n");
        exit(EXIT_FAILURE);
    }
    close(fd);

    int pid = fork();
    if(pid == 0){

        strcpy(share, "你是个好人!\n");
    }else{
        sleep(1);
        printf("回信的内容: %s", share);
        wait(NULL);

        int ret = munmap(share, 100);
        if (ret == -1)
        {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
    }
    shm_unlink(shmName);

    return 0;
}
