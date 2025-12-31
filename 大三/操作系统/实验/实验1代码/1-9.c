#include <stdio.h>
#include <pthread.h>

// 自旋锁结构体
typedef struct {
    int flag;
} spinlock_t;

// 初始化自旋锁
void spinlock_init(spinlock_t *lock) {
    lock->flag = 0;
}

// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->flag, 1)) {
        // 自旋等待
    }
}

// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->flag);
}

// 全局共享变量
int value = 0;

// 线程1：对变量递增
void *my_inc(void *arg) {
    spinlock_t *lock = (spinlock_t *)arg;
    for (int i = 0; i < 5000; ++i) {
        spinlock_lock(lock);
        value+=100;
        spinlock_unlock(lock);
    }
    pthread_exit(NULL);
}

// 线程2：对变量递减
void *my_dec(void *arg) {
    spinlock_t *lock = (spinlock_t *)arg;
    for (int i = 0; i < 5000; ++i) {
        spinlock_lock(lock);
        value-=50;
        spinlock_unlock(lock);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2;
    spinlock_t lock;

    // 初始化自旋锁
    spinlock_init(&lock);

    // 给共享变量赋初值
    value = 100;

    printf("first value: %d\n", value);

    // 创建两个线程
    pthread_create(&t1, NULL, my_inc, &lock);
    pthread_create(&t2, NULL, my_dec, &lock);
    printf("thread1 and thread2 created successfully\n");
    // 等待两个线程结束
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // 输出最终结果
    printf("final value: %d\n", value);
    return 0;
}
