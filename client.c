#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>
// 缓冲区大小和生产者、消费者数量
#define BUFFER_SIZE 20
#define MAX_PRODUCERS 3

// 共享缓冲区和索引
int in = 0, out = 0;       // 缓冲区的插入和提取位置

// 信号量和互斥锁
sem_t empty, full;
pthread_mutex_t mutex;

// 结构体定义，存储进程ID、线程ID和随机数据
typedef struct {
    pid_t pid;           // 进程 ID
    pthread_t tid;       // 线程 ID
    int random_number;   // 随机数据
} Data;
Data buffer[BUFFER_SIZE];
// 获取并打印进程ID和线程ID
void get_id() {
    printf("进程ID: %d\n", getpid());
    printf("线程ID: %lu\n", pthread_self());
}

double exponential_random(double lambda)
{
    double u=rand()/(RAND_MAX+1.1);
  //  printf("%f\n",u);
    return -log(1.0-u)/lambda;
}


// 生产者线程
void* producer(void *arg) {
    double lambda = *(double *)arg;  // 线程ID
  // printf("%f/n",lambda);
   
    while(1) {
        int data = rand() % 100;  // 生成一个随机数据

        // 创建 Data 结构体，存储进程ID、线程ID和随机数据
        Data data_to_send;

        data_to_send.tid = pthread_self();  // 获取线程 ID
        data_to_send.random_number = data;  // 随机数据

        get_id();  // 打印进程ID和线程ID
        printf(" 生成的数据: %d\n", data);

        // 等待空槽位
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);  // 进入临界区

        // 将数据写入缓冲区
        buffer[in] = data_to_send;
        in = (in + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);  // 离开临界区
        sem_post(&full);  // 通知有新数据可以消费
        double wait_time=exponential_random(lambda);
        printf("producers wait %.2f秒后生产下一个数据\n",wait_time);
        sleep(wait_time);
    }
}

// 管道线程，负责将数据发送到管道
void* pipe_thread(void *arg) {
    int fd = open("./mypipe", O_WRONLY);  // 打开管道
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    while(1) {
        sem_wait(&full);  // 等待满槽位
        pthread_mutex_lock(&mutex);  // 进入临界区

        // 从缓冲区读取数据
        Data data = buffer[out];
        out = (out + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);  // 离开临界区
        sem_post(&empty);  // 通知有空槽位

        // 通过管道发送数据
        write(fd, &data, sizeof(data));
    }

    close(fd);  // 关闭管道文件描述符
    return NULL;
}

int main(int argc ,char *argv[]) {

    if(argc!=2){
        printf("imput the lambda");
        return 1;
    }
    double lambda=atof(argv[1]);
    srand(time(NULL));

    // 初始化信号量和互斥锁
    sem_init(&empty, 0, BUFFER_SIZE);  // 空槽位信号量初始化为 BUFFER_SIZE
    sem_init(&full, 0, 0);             // 满槽位信号量初始化为 0
    pthread_mutex_init(&mutex, NULL);  // 初始化互斥锁

    pthread_t producers[MAX_PRODUCERS];
    pthread_t pipe_thread_id;

    // 创建管道线程
    pthread_create(&pipe_thread_id, NULL, pipe_thread, NULL);

    // 创建生产者线程
    for (int i = 0; i < MAX_PRODUCERS; i++) {
        pthread_create(&producers[i], NULL, producer, &lambda);
    }

    // 等待所有线程执行完成
    pthread_join(pipe_thread_id, NULL);
    for (int i = 0; i < MAX_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    // 销毁信号量和互斥锁
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}

