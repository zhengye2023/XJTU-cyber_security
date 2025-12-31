#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int pid1, pid2; // 定义两个进程变量

int main() {
    int fd[2];
    int zi[2];
    int fu[2];
    char InPipe[5000]; // 定义读缓冲区
    char c1 = '1', c2 = '2';
    char buf;

    pipe(fd);
    pipe(zi);
    pipe(fu);

    while ((pid1 = fork()) == -1);
    if (pid1 == 0) { 
        // 子进程1 
        close(fd[0]);  // 不读
        close(zi[1]);  // 不写
        close(fu[0]);  
        lockf(fd[1], 1, 0);                   
        for (int i = 0; i < 2000; i++)        
            write(fd[1], &c1, 1);
        lockf(fd[1], 0, 0);
         write(fu[1], "1", 1);
        // 等待父进程信号
        read(zi[0], &buf, 1);
        close(fd[1]);
        close(zi[0]);
        close(fu[1]);
        printf("child 1 exiting\n");
        exit(0);
    } 

    while ((pid2 = fork()) == -1);
    if (pid2 == 0) { 
        // 子进程2 
        close(fd[0]);  // 不读
        close(zi[1]);  // 不写
        close(fu[0]);  
        lockf(fd[1], 1, 0);              
        for (int i = 0; i < 2000; i++)   
            write(fd[1], &c2, 1);
        lockf(fd[1], 0, 0);
        write(fu[1], "2", 1);
        // 等待父进程信号
        read(zi[0], &buf, 1);
        close(fd[1]);
        close(zi[0]);
        close(fu[1]);
        printf("child 2 exiting\n");
        exit(0);
    } 

    // 父进程部分
    close(fd[1]);  // 不写
    close(zi[0]);  // 不读
    close(fu[1]);
    read(fu[0],&buf,1);
    read(fu[0],&buf,1);
    // 不断读取直到所有写端关闭
    int n = read(fd[0], InPipe, 4000); 
    InPipe[n] = '\0';    
    printf("%s\n", InPipe);

    // 通知两个子进程退出
    write(zi[1], "1", 1);
    write(zi[1], "1", 1);

    close(zi[1]);
    close(fd[0]);
    close(fu[0]);
    wait(0);
    wait(0);
    printf("Parent exiting\n");
    return 0;
}
