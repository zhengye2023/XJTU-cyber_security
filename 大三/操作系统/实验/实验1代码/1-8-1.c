#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/syscall.h>
// 线程1：调用 system()
void* thread1_func(void* arg) {
    pid_t pid = getpid();          // 当前进程号
    pid_t tid = syscall(SYS_gettid); // 当前线程号
    printf("Thread1 PID = %d, TID = %d\n", pid, tid);
    printf("Thread1 调用 system() 执行外部程序...\n");
    system("./system_call");       // 调用外部可执行文件
    printf("Thread1 system() 返回主程序。\n");
    return NULL;
}

// 线程2：调用 system()
void* thread2_func(void* arg) {
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid); 
    printf("Thread2 PID = %d, TID = %d\n", pid, tid);
    printf("Thread2调用 system() 执行外部程序...\n");
    system("./system_call");
    printf("Thread2 system() 返回主程序。\n");
    return NULL;
}

int main() {
    pthread_t t1, t2;

    printf("Main Thread:PID = %d, TID = %d\n", getpid(),syscall(SYS_gettid) );

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("all threads have finished.\n");
    return 0;
}
