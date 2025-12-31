#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    pid_t pid, pid1;
    pid = fork();
    if (pid < 0) {/* error occurred */
     fprintf(stderr,"Fork Failed");
     return 1;   
    }
    else if (pid == 0) {
        /* child process */
        pid1 = getpid();
        printf("child: pid = %d, pid1 = %d\n", pid, pid1);
        printf("\n--- 子进程调用 system() ---\n");
        system("./system_call");  // 使用 system 执行外部程序
        printf("\n--- 子进程调用 exec() ---\n");
        execl("./system_call", "system_call", NULL); // exec 执行外部程序
    }
    else {/* parent process */
        pid1 = getpid();
        printf("parent: pid = %d, pid1 = %d\n", pid, pid1);
        wait(NULL);
    }
    return 0;
}
