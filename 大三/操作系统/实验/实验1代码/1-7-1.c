#include <stdio.h>
#include <pthread.h>
#include<semaphore.h>
int count = 0; // 共享变量
sem_t signal;

void* my_add(void* arg) {
    for (int i = 0; i < 10000; i++) {
        sem_wait(&signal); // 等待信号量
        count += 100; // 加100
        sem_post(&signal); // 发送信号量    
    }
    return NULL;
}

void* my_sub(void* arg) {
    for (int i = 0; i < 10000; i++) {
        sem_wait(&signal); // 等待信号量
        count -= 100; // 减100
        sem_post(&signal); // 发送信号量
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    // 创建两个线程
    sem_init(&signal, 0, 1); 
    pthread_create(&t1, NULL, my_add, NULL);
    printf("t1 created successfully\n");
    pthread_create(&t2, NULL, my_sub, NULL);
    printf("t2 created successfully\n");
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("count = %d\n", count);
    sem_destroy(&signal); 
    return 0;
}
