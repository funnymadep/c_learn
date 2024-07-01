#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 共享资源
int shared_resource = 0;

// 互斥锁
pthread_mutex_t mutex;

void *thread_function(void *arg) {
    int thread_num = *(int *)arg;
    printf("Thread %d: Starting...\n", thread_num);

    // 锁定互斥锁
    pthread_mutex_lock(&mutex);

    // 访问和修改共享资源
    printf("Thread %d: Accessing shared resource.\n", thread_num);
    shared_resource += 1;
    printf("Thread %d: Shared resource value: %d\n", thread_num, shared_resource);

    // 解锁互斥锁
    pthread_mutex_unlock(&mutex);

    printf("Thread %d: Finished.\n", thread_num);
    return NULL;
}

int main() {
    pthread_t thread;
    int thread_arg = 1;

    // 初始化互斥锁
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        fprintf(stderr, "Error: Unable to initialize mutex\n");
        return -1;
    }

    // 创建线程
    printf("Main: Creating thread %d\n", thread_arg);
    int rc = pthread_create(&thread, NULL, thread_function, (void *)&thread_arg);
    if (rc) {
        fprintf(stderr, "Error: Unable to create thread, %d\n", rc);
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    // 主线程访问和修改共享资源
    pthread_mutex_lock(&mutex);
    printf("Main: Accessing shared resource.\n");
    shared_resource += 1;
    printf("Main: Shared resource value: %d\n", shared_resource);
    pthread_mutex_unlock(&mutex);

    // 等待子线程完成
    pthread_join(thread, NULL);
    printf("Main: Joined thread %d\n", thread_arg);

    // 销毁互斥锁
    pthread_mutex_destroy(&mutex);

    printf("Main: Program finished.\n");
    return 0;
}

