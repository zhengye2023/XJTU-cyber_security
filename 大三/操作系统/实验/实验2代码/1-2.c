#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

int flag = 0;  // 父进程是否收到 信号
void inteer_handlerf(int sig){
    falg=0;
}
void inter_handler(int sig) {
    flag = 1;
}
// 子进程1 信号 16
void child1_handler(int sig) {
    printf("\nChild process1 is killed by parent!!\n");
    exit(0);
}
// 子进程2 信号 17
void child2_handler(int sig) {
    printf("\nChild process2 is killed by parent!!\n");
    exit(0);
}
// 子进程等待父进程信号
void waiting() {
    while (1) pause();
}
int main() {
    signal(SIGINT, inter_handler);
    signal(SIGQUIT, inter_handler);
    signal(SIGALRM, inter_handlerf) ;

    int pipefd[2];
    pipe(pipefd); // 用于父子进程同步

    pid_t pid1, pid2;

    pid1 = fork();

    if (pid1 == 0) {
        // 子进程1
        close(pipefd[0]); // 不读
        signal(16, child1_handler); // 安装信号处理器

        // 通知父进程ready
        write(pipefd[1], "1", 1);

        waiting();
        return 0;
    }
    pid2 = fork();

    if (pid2 == 0) {
        // 子进程2
        close(pipefd[0]); // 不读
        signal(17, child2_handler); // 安装信号处理器

        // 通知父进程ready
        write(pipefd[1], "2", 1);

        waiting();
        return 0;
    }

    // 父进程

    char buf;

    read(pipefd[0], &buf, 1);
    read(pipefd[0], &buf, 1);
    close(pipefd[0]);

    printf("Parent waiting 5 seconds\n");
    alarm(5);
    pause();

    if (flag == 1)
        printf("Parent received DELETE or QUIT signal\n");
    else
        printf("No signal received in 5 seconds.\n");

    // 发送信号给子进程
    kill(pid1, 16);
    kill(pid2, 17);

    // 等待子进程退出
    wait(NULL);
    wait(NULL);

    printf("\nParent process is killed!!\n");
    return 0;
}
