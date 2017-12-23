#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <semaphore.h>

#define BUFFER_SIZE 10//缓冲区大小为10
#define N_WRITER 20 //写者数目
#define N_READER 5 //读者数目
#define W_SLEEP  1 //控制写频率
#define R_SLEEP  1 //控制读频率
#define P(S) sem_wait(S)
#define V(S) sem_post(S)

pthread_rwlock_t rwlock;//define write and read lock
pthread_t wid[N_WRITER], rid[N_READER];
int readerCnt = 0, writerCnt = 0;
sem_t sem_read;
sem_t sem_write;
pthread_mutex_t write_mutex;
pthread_mutex_t read_mutex;
//生产者消费者问题
char *buffer;
sem_t *mutex, *empty, *full;//三个信号量，互斥信号量mutex，技术信号量empty和full
int x, y;//生产者和消费者在buffer中下标
void output()//输出buffer数组
{
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%c", buffer[i]);
        printf(" ");
    }
    printf("\n");
}

void *produce(void *pVoid)//生产者函数
{
    int j = 0;
    do {
        P(empty);
        P(mutex);
        printf("%lu%s", pthread_self(), " ——————生产—————— ");//输出当前线程的id号，以及正在执行的次数
        buffer[(x++) % BUFFER_SIZE] = 'P';//生产就赋值A
        output();//输出buffer
        j++;
        V(mutex);
        V(full);
    } while (j != 30);//每个线程可以做30次
}

void *consume(void *pVoid)//消费者函数
{
    int j;
    j = 0;
    do {
        P(full);
        P(mutex);
        printf("%lu%s", pthread_self(), " ------消费------ ");
        buffer[(y++) % BUFFER_SIZE] = 'C';//消费时，赋值为B
        output();//输出buffer值
        j++;
        V(mutex);
        V(empty);
    } while (j != 30);//每个线程可以消费30次
}

void ConAndPro() {
    int i;
    x = 0;
    y = 0;
    buffer = (char *) malloc(BUFFER_SIZE * sizeof(char *));
    mutex = static_cast<sem_t *>(malloc(sizeof(sem_t *)));
    empty = static_cast<sem_t *>(malloc(sizeof(sem_t *)));
    full = static_cast<sem_t *>(malloc(sizeof(sem_t *)));
    for (i = 0; i < BUFFER_SIZE; i++)//初始化buffer数组，默认为N
    {
        buffer[i] = 'N';
    }
    //semaphore
    sem_init(mutex, 1, 1);//初始化互斥信号量mutex为1
    sem_init(empty, 0, BUFFER_SIZE);//初始化计数信号量empty为BUFFER_SIZE
    sem_init(full, 0, 0);//初始化计数信号量full为0
    //multipthread
    pthread_t tid[10];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    //创建5个生产者线程和5个消费者线程
    for (i = 0; i < 5; i++) {
        pthread_create(&tid[i], &attr, consume, NULL);
        pthread_create(&tid[i + 5], &attr, produce, NULL);
    }
//让每个线程在主线程main执行前全部执行完毕。
    for (i = 0; i < 10; i++) {
        pthread_join(tid[i], NULL);
    }
}

//读者优先
void *reader(void *arg) {
    printf("编号为%d的读者进入等待中。。。\n", pthread_self());
    sem_wait(&sem_read);
    readerCnt++;
    if (readerCnt == 1) {
        pthread_mutex_lock(&write_mutex);
    }
    sem_post(&sem_read);
    printf("编号为%d的读者开始读\n", pthread_self());
    printf("编号为%d的读者读完\n", pthread_self());
    printf("-----------------------\n");
    sem_wait(&sem_read);
    readerCnt--;
    if (readerCnt == 0) {
        pthread_mutex_unlock(&write_mutex);
    }
    sem_post(&sem_read);
    sleep(R_SLEEP);
    pthread_exit((void *) 0);
}

void *writer(void *arg) {
    //let the reader run first
    printf("写者%d线程进入等待中...\n", pthread_self());
    pthread_mutex_lock(&write_mutex);
    printf("写者%d开始写文件\n", pthread_self());
    printf("写者%d结束写文件\n", pthread_self());
    printf("-----------------------\n");
    pthread_mutex_unlock(&write_mutex);
    sleep(W_SLEEP);
    pthread_exit((void *) 0);
}

void readerFirst() {
    readerCnt = 0, writerCnt = 0;
    printf("多线程,读者优先\n");
    pthread_mutex_init(&write_mutex, NULL);
    pthread_mutex_init(&read_mutex, NULL);

    sem_init(&sem_read, 0, 1);
    int i = 0;
    for (i = 0; i < N_WRITER; i++) {
        pthread_create(&wid[i], NULL, writer, NULL);
    }
    for (i = 0; i < N_READER; i++) {
        pthread_create(&rid[i], NULL, reader, NULL);
    }
    sleep(1);
}

//写者优先
void *mywriter(void *arg) {
    printf("写者%d线程进入等待中...\n", pthread_self());
    sem_wait(&sem_write);
    writerCnt++;
    if(writerCnt==1){
        pthread_mutex_lock(&read_mutex);
    }
    sem_post(&sem_write);
    //执行写操作
    pthread_mutex_lock(&write_mutex);
    printf("写者%d开始写文件\n", pthread_self());
    printf("写者%d结束写文件\n", pthread_self());
    printf("---------------------\n");
    pthread_mutex_unlock(&write_mutex);
    //写完以后，写着退出
    sem_wait(&sem_write);
    writerCnt--;
    if (writerCnt==0){
        pthread_mutex_unlock(&read_mutex);
    }
    sem_post(&sem_write);
}

void *myreader(void *arg) {

    printf("编号为%d的读者进入等待中。。。\n", pthread_self());
    pthread_mutex_lock(&read_mutex);
    sem_wait(&sem_read);
    readerCnt++;
    if (readerCnt == 1) {
        pthread_mutex_lock(&write_mutex);
    }
    sem_post(&sem_read);
    printf("编号为%d的读者开始读\n", pthread_self());
    printf("编号为%d的读者读完\n", pthread_self());
    printf("---------------------\n");

    pthread_mutex_unlock(&read_mutex);


    sem_wait(&sem_read);
    readerCnt--;
    if (readerCnt==0){
        pthread_mutex_unlock(&write_mutex);
    }
    sem_post(&sem_read);
    sleep(R_SLEEP);
    pthread_exit((void *) 0);
}

void writerFirst() {
    readerCnt=0;
    writerCnt=0;
    printf("多线程,写者优先\n");
    pthread_mutex_init(&write_mutex, NULL);
    pthread_mutex_init(&read_mutex, NULL);
    sem_init(&sem_write, 0, 1);

    int i = 0;
    for (i = 0; i < N_READER; i++) {
        pthread_create(&rid[i], NULL, myreader, NULL);
    }
    for (i = 0; i < N_WRITER; i++) {
        pthread_create(&wid[i], NULL, mywriter, NULL);
    }
    sleep(1);
}

void menu() {
    printf("======================================\n");
    printf("15140A01 第一组 基于Linux的线程与进程控制\n");
    printf("1.生产者消费者\n");
    printf("2.读者写者问题之读者优先\n");
    printf("3.读者写者问题之写者优先\n");
    printf("4.退出程序\n");
    printf("======================================\n");
    printf("请输入选项：");
    int a;
    std::cin >> a;
    switch (a) {
        case 1:
            ConAndPro();
            menu();
            break;
        case 2:
            readerFirst();
            menu();
            break;
        case 3:
            writerFirst();
            menu();
            break;
        case 4:
            printf("谢谢使用");
            exit(0);
    }
}

int main() {

    menu();
    return 0;
}