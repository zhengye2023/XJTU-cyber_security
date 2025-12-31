#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

int count = 0; 
sem_t signal1, signal2;

void* my_add(void* arg) {
    for (int i = 0; i < 5; i++) {
        sem_wait(&signal1); // 等待信号量1
        count += 100;
        printf("t1: count = %d\n", count);
        sem_post(&signal2); // 通知线程2可以执行
    }
    return NULL;
}

void* my_sub(void* arg) {
    for (int i = 0; i < 5; i++) {
        sem_wait(&signal2); // 等待线程1释放信号量
        count -= 50;
        printf("t2: count = %d\n", count);
        sem_post(&signal1); // 通知线程1继续
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    // 初始化信号量：sem1=1，sem2=0
    sem_init(&signal1, 0, 1); // t1 先执行
    sem_init(&signal2, 0, 0); // t2 等待

    pthread_create(&t1, NULL, my_add, NULL);
    pthread_create(&t2, NULL, my_sub, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("count = %d\n", count);

    sem_destroy(&signal1);
    sem_destroy(&signal2);
    return 0;
}
