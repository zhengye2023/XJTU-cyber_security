#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

int flag = 0;  // 标志父进程

void inter_handler(int sig) {
    flag = 1;
}

void child1_handler(int sig) {
    printf("\nChild process 1 is killed by parent!!\n");
    exit(0);
}

void child2_handler(int sig) {
    printf("\nChild process 2 is killed by parent!!\n");
    exit(0);
}

// 子进程等待父进程信号
void waiting() {
    while (1) pause();
}

int main() {
    signal(SIGINT, inter_handler);
    signal(SIGQUIT, inter_handler);

    int pipefd[2];
    pipe(pipefd); // 父进程等待两个子进程 ready

    pid_t pid1, pid2;

    pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]); 
        signal(16, child1_handler); 
        write(pipefd[1], "1", 1);
        waiting();
        return 0;
    }

    pid2 = fork();
    if (pid2 == 0) {
        //  子进程2 
        close(pipefd[0]); 
        signal(17, child2_handler);
        write(pipefd[1], "2", 1); 
        waiting();
        return 0;
    }

    //  父进程 
    close(pipefd[1]); // 不写
    char buf;

    read(pipefd[0], &buf, 1);
    read(pipefd[0], &buf, 1);
    close(pipefd[0]);

    printf("Parent waiting 5 seconds\n");
    sleep(5);

    if (flag == 1)
        printf("Parent received DELETE  or QUIT signal\n");
    else
        printf("No signal received in 5 seconds.\n");

    // 向两个子进程发送信号
    kill(pid1, 16);
    kill(pid2, 17);

    // 等待子进程结束
    wait(NULL);
    wait(NULL);

    printf("\nParent process is killed!!\n");
    return 0;
}
