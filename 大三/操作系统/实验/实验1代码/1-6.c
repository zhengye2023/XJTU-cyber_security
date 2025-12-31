#include <stdio.h>
#include <pthread.h>
int count = 0; // 共享变量

void*my_add(void* arg) {
    for (int i = 0; i < 5000; i++) {
        count += 100; // 加100
    }
    return NULL;
}

void* my_sub(void* arg) {
    for (int i = 0; i < 5000; i++) {
        count -= 100; // 减100
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    // 创建两个线程
    pthread_create(&t1, NULL, my_add, NULL);
    pthread_create(&t2, NULL, my_sub, NULL);
    // 等待两个线程执行完毕
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("count = %d\n", count);
    return 0;
}
