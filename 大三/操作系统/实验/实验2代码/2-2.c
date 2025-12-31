#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/wait.h>

int pid1, pid2; // 定义两个子进程ID

int main() {
    int fd[2]; 
    int wa[2];
    int fi[2];
    char InPipe[1000]; // 定义读缓冲区
    char OutPipe[1000]; // 定义写缓冲区
    char buf;
    char sig;

    pipe(fd); // 创建通信管道
    pipe(wa); // 父进程提醒子进程结束信号
    pipe(fi);  //子进程提醒父进程开始信号

    // 创建第一个子进程
    while ((pid1 = fork()) == -1); 
    if (pid1 == 0) { 
        // 子进程1
        close(fd[0]);   // 不读通信管道
        close(wa[1]);   // 不写控制管道
        close(fi[0]);
       // lockf(fd[1], 1, 0); // 锁定写端
        strcpy(OutPipe, "Child Process 1 is sending message\n");
        for(int i=0;i<strlen(OutPipe);i++){
        write(fd[1], OutPipe+i,1);
        }
        //lockf(fd[1], 0, 0); // 解锁
        write(fi[1],"1",1);
        read(wa[0], &buf, 1); // 等待父进程通知
        printf("child process 1 is exiting\n");
        close(fd[1]); 
        close(wa[0]); 
        close(fi[1]);
        exit(0); 
    } 
    else { 
        // 创建第二个子进程
        while ((pid2 = fork()) == -1);    
        if (pid2 == 0) { 
            close(fd[0]);
            close(wa[1]);
            close(fi[0]);
           // lockf(fd[1], 1, 0);
            strcpy(OutPipe, "Child Process 2 is sending message\n");
            for(int i=0;i<strlen(OutPipe);i++) {
            write(fd[1], OutPipe+i,1);
            }
           // lockf(fd[1], 0, 0);
            write(fi[1],"1",1);
            read(wa[0], &buf, 1);
            printf("child process 2 is exiting\n");
            close(fd[1]);
            close(wa[0]);
            close(fi[1]);
            exit(0);
        } 
        else { 
            // 父进程
            close(fd[1]); // 不写通信管道
            close(wa[0]); // 不读控制管道
            close(fi[1]);
           read(fi[0],&sig,1);
           read(fi[0],&sig,1);
            int n = read(fd[0], InPipe, sizeof(InPipe)-1);    
            InPipe[n] = '\0';          
            printf("%s\n", InPipe);

            // 通知两个子进程可以结束
            write(wa[1], "1", 1);
            write(wa[1], "1", 1);

            wait(0);  // 等待子进程1结束
            wait(0);  // 等待子进程2结束
            close(fi[0]);
            close(fd[0]);
            close(wa[1]);
            printf("Parent process exit\n");
            exit(0); 
        } 
    }
}
