#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/sem.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/stat.h>


#include "serv.h" 

#define V 1
#define P -1

#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

int flag = 1;

char buf[ARRAY_SIZE];

int semid;

struct sembuf start_read[] = 
{
    {READ_QUEUE, V, 0},
    {WRITE_QUEUE, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READERS, V, 0},
    {READ_QUEUE, P, 0}
};

struct sembuf end_read[] = 
{
    {ACTIVE_READERS, P, 0}
};

struct sembuf start_write[] = 
{
    {WRITE_QUEUE, V, 0},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READERS, 0, 0},
    {ACTIVE_WRITER, V, 0},
    {WRITE_QUEUE, P, 0}
};

struct sembuf end_write[] = 
{
    {ACTIVE_WRITER, P, 0}
};

void signal_handler(int sig)
{
    printf("Получен сигнал %d\n", sig);
    flag = 0;
}

void reader(char *buf, int sockfd, int semid)
{
    char arr[ARRAY_SIZE];
    int err = semop(semid, start_read, 5);
    if (err == -1)
    {
        perror("semop error (start_read)");
        exit(EXIT_FAILURE);
    }

    memcpy(arr, buf, sizeof(arr));

    err = semop(semid, end_read, 1);
    if (err == -1)
    {
        perror("semop error (end_read)");
        exit(EXIT_FAILURE);
    }

    if (write(sockfd, arr, sizeof(arr)) == -1)
    {
        perror("write error (reader)");
        exit(EXIT_FAILURE);
    }

    printf("Обработка запроса на чтение завершена.\n");
}

void writer(char *buf, int sockfd, int semid)
{
    int rc = OK;
    char result = '\0';
    int index;

    int size = read(sockfd, &index, sizeof(index));
    if (size < 0)
    {
        perror("read error (writer)");
        close(sockfd);
        pthread_exit(NULL);
    }

    if (size == 0)
    {
        printf("Соединение закрыто клиентом\n");
        close(sockfd);
        pthread_exit(NULL);
    }

    int endF = 0;
    int err = semop(semid, start_write, 5);
    if (err == -1)
    {
        perror("semop error (start_write)");
        exit(EXIT_FAILURE);
    }

    if (index < 0 || index >= ARRAY_SIZE)
    {
        rc = RANGE_ERROR;
        endF = 1;
    }
    else if (buf[index] == ARRAY_ELEMENT_BUSY)
    {
        rc = WRITE_BUSY_ERROR;
    }
    else
    {
        result = buf[index];
        buf[index] = ARRAY_ELEMENT_BUSY;
    }

    printf("Запрос на запись, индекс %d\n", index);
    err = semop(semid, end_write, 1);
    if (err == -1)
    {
        perror("semop error (end_write)");
        exit(EXIT_FAILURE);
    }

    rc = htonl(rc);
    if (write(sockfd, &rc, sizeof(rc)) == -1)
    {
        perror("write error (writer: rc)");
    }

    if (result != '\0')
    {
        if (write(sockfd, &result, sizeof(result)) == -1)
        {
            perror("write error (writer: result)");
            exit(EXIT_FAILURE);
        }
    }

    if (endF)
    {
        close(sockfd);
        pthread_exit(NULL);
    }
    printf("Обработка запроса на запись завершена.\n");
}


void childfunc(char *buf, int sockfd, int semid)
{
    int size, type;
    while ((size = read(sockfd, &type, sizeof(type))) > 0)
    {
        type = ntohl(type);
        if (type == READ)
        {
            printf("Получен запрос на чтение.\n");
            reader(buf, sockfd, semid);
        }
        else if (type == WRITE)
        {
            printf("Получен запрос на запись.\n");
            writer(buf, sockfd, semid);
        }
        else
        {
            printf("Неизвестный тип запроса: %d\n", type);
            break;
        }
    }
    if (size == -1)
    {
        perror("read error (childfunc)");
    }
    else if (size == 0)
    {
        printf("Соединение закрыто клиентом.\n");
    }

    close(sockfd);
    pthread_exit(NULL);
}

typedef struct {
    int connfd;
    int cpuID;
} thread_data_t;

void *client_handler(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int connfd = data->connfd;
    int cpuID = data->cpuID;
    free(data);

    int num = sysconf(_SC_NPROCESSORS_CONF);
    if (cpuID < 0 || cpuID >= num) {
        fprintf(stderr, "cpuID %d is out of range (0 - %d). Using 0 instead.\n", cpuID, num - 1);
        cpuID = 0;
    }

    cpu_set_t mask;
    cpu_set_t get;
    CPU_ZERO(&mask);

    CPU_ZERO(&get);
    int s = pthread_getaffinity_np(pthread_self(), sizeof(get), &get);
    if (s != 0)
        fprintf(stderr, "pthread_getaffinity_np error: %s\n", strerror(s));

    printf("Поток %lu обслуживает клиента ", (unsigned long)pthread_self());
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);

    if (getpeername(connfd, (struct sockaddr *)&peer, &len) != -1)
        printf("%d\n", ntohs(peer.sin_port));
    else
        perror("getpeername");

    // printf(" on CPU(s): ");
    // for (int i = 0; i < num; i++)
        // if (CPU_ISSET(i, &get))
            // printf("%d ", i);
    int cpu = sched_getcpu();
    printf("Thread %lu is running on CPU: %d\n", (unsigned long)pthread_self(), cpu);

    childfunc(buf, connfd, semid);
    pthread_exit(NULL);
}


int main(void)
{
    signal(SIGINT, signal_handler);

    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    semid = semget(IPC_PRIVATE, 4, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget error");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, ACTIVE_READERS, SETVAL, 0) == -1)
    {
        perror("semctl error (ACTIVE_READERS)");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, ACTIVE_WRITER, SETVAL, 0) == -1)
    {
        perror("semctl error (ACTIVE_WRITER)");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, WRITE_QUEUE, SETVAL, 0) == -1)
    {
        perror("semctl error (WRITE_QUEUE)");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, READ_QUEUE, SETVAL, 0) == -1)
    {
        perror("semctl error (READ_QUEUE)");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        buf[i] = 'a' + i % 26;
    }

    printf("Инициализирован массив: ");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        printf("%c", buf[i]);
    }

    printf("\n");

    int lstnfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lstnfd == -1)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sadr;
    memset(&sadr, 0, sizeof(sadr));
    sadr.sin_family = AF_INET;
    sadr.sin_port = htons(PORT);
    sadr.sin_addr.s_addr = INADDR_ANY;
    if (bind(lstnfd, (struct sockaddr *) &sadr, sizeof(sadr)) == -1)
    {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(lstnfd, 5) == -1)
    {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    printf("Сервер запущен на порту %d.\n", PORT);

    pthread_attr_t attr;
    int s_attr = pthread_attr_init(&attr);
    if (s_attr != 0)
    {
        fprintf(stderr, "pthread_attr_init error: %s\n", strerror(s_attr));
        exit(EXIT_FAILURE);
    }

    s_attr = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (s_attr != 0)
    {
        fprintf(stderr, "pthread_attr_setdetachstate error: %s\n", strerror(s_attr));
        exit(EXIT_FAILURE);
    }

    while (flag)
    {
        int connfd = accept(lstnfd, NULL, NULL);
        if (connfd == -1)
        {
            perror("accept error");
            continue;
        }

        thread_data_t *data = malloc(sizeof(thread_data_t));
        if (!data)
        {
            perror("malloc error");
            close(connfd);
            continue;
        }

        data->connfd = connfd;
        data->cpuID = 0;
        pthread_t tid;
        int rc = pthread_create(&tid, &attr, client_handler, data);
        if (rc != 0)
        {
            fprintf(stderr, "pthread_create error: %s\n", strerror(rc));
            free(data);
            close(connfd);
            continue;
        }

        // printf("flag: %d", flag);
    }

    pthread_attr_destroy(&attr);
    close(lstnfd);
    return 0;
}
