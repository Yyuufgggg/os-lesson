#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>

// 缓冲区大小和消费者线程数
#define BUFFER_SIZE 20
#define MIN_CONSUMERS 3
#define MAX_CONSUMERS 10
// 共享缓冲区，存储从管道接收的数据
typedef struct {
    pid_t pid;           // 进程 ID
    pthread_t tid;       // 线程 ID
    int random_number;   // 随机数据
} Data;

double exponential_random(double lambda){
    double u=rand()/(RAND_MAX+1.0);
    if(u==1)
        u=0.99999;
    return -log(1.0-u)/lambda;
}




Data buffer[BUFFER_SIZE];  // 缓冲区
int in = 0, out = 0;       // 缓冲区的插入和提取位置

sem_t empty, full;  // 空槽位和满槽位信号量
pthread_mutex_t mutex;  // 互斥锁，用于保护缓冲区
    pthread_t consumers[MAX_CONSUMERS];
void* consumer(void* arg) {                                                                          
           double lambda=*(double*)arg;                                                              
    while(1) {                                                                                       
        sem_wait(&full);  // 等待满槽位，确保有数据可以消费                                          
        pthread_mutex_lock(&mutex);  // 进入临界区，保护缓冲区                                       
                                                                                                     
        // 从缓冲区提取数据                                                                          
        Data data = buffer[out];                                                                     
        out = (out + 1) % BUFFER_SIZE;                                                               
                                                                                                     
        pthread_mutex_unlock(&mutex);  // 离开临界区                                                 
        sem_post(&empty);  // 通知有空槽位                                                           
                                                                                                     
        // 消费数据，进行处理                                                                        
        printf(" 提取的数据: 进程ID: %d, 线程ID: %lu, 随机数: %d\n",                                 
                 data.pid, (unsigned long)data.tid, data.random_number);                             
       double wait_time=exponential_random(lambda);                                                  
       printf("consumers wait %.2f秒后消费下一个数据\n",wait_time);                                  
       sleep(wait_time);                                                                             
    }                                                                                                
}         
void *thread_pool_manager(void* arg){
    int current_thread_count=MIN_CONSUMERS;
    int max_threads=MAX_CONSUMERS;
    int min_threads=MIN_CONSUMERS;
    double lambda=*(double*)arg;
    for (int i = 0; i < MIN_CONSUMERS; i++) {
        pthread_create(&consumers[i], NULL, consumer, &lambda);
    }
    while(1){
        int sval;
        int full_count=BUFFER_SIZE-sem_getvalue(&empty,&sval);
       // printf("%d\n",full_count);
        if(full_count<5&&current_thread_count<max_threads){
            pthread_create(&consumers[current_thread_count],NULL,consumer,&lambda);
            current_thread_count++;
            printf("增加了一个消费者线程，当前线程数目为:%d\n",current_thread_count);
        }
        else if(full_count>10 &&current_thread_count>min_threads)
        {
           pthread_cancel(consumers[current_thread_count-1]);
           pthread_join(consumers[current_thread_count-1],NULL);
           current_thread_count--;
           printf("减少了一个消费者线程，当前线程数目是:%d\n",current_thread_count);
        }
    }
}





void* pipe_thread(void* arg) {
    int fd = open("./mypipe", O_RDONLY);  // 打开管道
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    while (1) {
        Data received_data;
        ssize_t s = read(fd, &received_data, sizeof(received_data));  // 从管道读取数据

        if (s > 0) {
            sem_wait(&empty);  // 等待空槽位，确保缓冲区有空间
            pthread_mutex_lock(&mutex);  // 进入临界区

            // 将接收到的数据放入缓冲区
            buffer[in] = received_data;
            in = (in + 1) % BUFFER_SIZE;

            pthread_mutex_unlock(&mutex);  // 离开临界区
            sem_post(&full);  // 通知有满槽位
        }
    }

    close(fd);  // 关闭管道文件描述符
    return NULL;
}

int main(int argc,char *argv[]) {
    if(argc!=2){
        printf("please input lambda");
        return 1;
    }
    double lambda=atof(argv[1]);
    srand(time(NULL));  // 设置随机数种子
    pthread_t pipe_thread_id;
    pthread_t manager_thread;
    // 初始化信号量
    sem_init(&empty, 0, BUFFER_SIZE);  // 空槽位初始化为缓冲区大小
    sem_init(&full, 0, 0);             // 满槽位初始化为0
    pthread_mutex_init(&mutex, NULL);  // 初始化互斥锁

    // 创建管道线程
    pthread_create(&pipe_thread_id, NULL, pipe_thread, NULL);
    pthread_create(&manager_thread,NULL,thread_pool_manager,&lambda);
    // 创建消费者线程
    

    // 等待线程执行完成
    pthread_join(manager_thread,NULL);
    pthread_join(pipe_thread_id, NULL);
    for (int i = 0; i < MAX_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    // 销毁信号量和互斥锁
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}

