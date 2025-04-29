#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/sem.h>

#include "serv.h"

#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define V 1
#define P -1

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
    {ACTIVE_READERS, 0, 0},
    {ACTIVE_WRITER, 0, 0},
    {WRITE_QUEUE, P, 0},
    {ACTIVE_WRITER, V, 0}
};

struct sembuf end_write[] = 
{
    {ACTIVE_WRITER, P, 0}
};

int flag = 1;

int signal_handler(int sig_numb) 
{
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
}

void reader(char *str, int sockfd, int semid) 
{
    char arr[ARRAY_SIZE];
    
    int err = semop(semid, start_read, 5);
    if (err == -1) 
    {
        perror("semop error\n");
        exit(EXIT_FAILURE);
    }

    memcpy(arr, str, sizeof(arr));

    err = semop(semid, end_read, 1);
    if (err == -1) 
    {
        perror("semop error\n");
        exit(EXIT_FAILURE);
    }

    if (write(sockfd, &arr, sizeof(arr)) == -1) 
    {
        perror("write error");
        exit(EXIT_FAILURE);
    }

    printf("Обработка запроса на чтение завершена.\n");
}

void writer(char *str, int sockfd, int semid, int ppid) 
{
    int rc = OK;
    char result = '\0';
    int index;
    int size = 0;

    if ((size = read(sockfd, &index, sizeof(index))) < 0) 
    {
        perror("read error");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 
    
    if (size == 0)
    {
        printf("Соединение закрыто\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int endF = 0;

    int err = semop(semid, start_write, 5);
    if (err == -1) 
    {
        perror("semop error\n");
        exit(EXIT_FAILURE);
    }

    if (index < 0 || index >= ARRAY_SIZE)
    {
        rc = RANGE_ERROR;
        endF = 1;
    }
    else if (str[index] == ARRAY_ELEMENT_BUSY) 
        rc = WRITE_BUSY_ERROR;
    else 
    {
        result = str[index];
        str[index] = ARRAY_ELEMENT_BUSY;
    }

    printf("Запрос на запись, индекс %d\n", index);

    err = semop(semid, end_write, 1);
    if (err == -1)
    {
        perror("semop error\n");
        exit(EXIT_FAILURE);
    }
    
    rc = htonl(rc);
    if (write(sockfd, &rc, sizeof(rc)) == -1)
        perror("write error");

    if (result != '\0')
    {
        if (write(sockfd, &result, sizeof(result)) == -1)
        {
            perror("write error");
            exit(EXIT_FAILURE);
        }
    }

    if (endF)
    {
        kill(ppid, SIGINT);
        close(sockfd);
        flag = 0;
        exit(EXIT_SUCCESS);
    }
    
    printf("Обработка запроса на запись завершена.\n");
}

void rw_func(char *str, int sockfd, int semid, int ppid) 
{
    int size;
    int type;

    while ((size = read(sockfd, &type, sizeof(type))) > 0)
    {
        type = ntohl(type);
        if (type == READ)
        {
            printf("Запрос чтения\n");
            reader(str, sockfd, semid);
        }
        else if (type == WRITE)
        {
            printf("Запрос записи\n");
            writer(str, sockfd, semid, ppid);
        }
        else
        {
            printf("Неизвестный запрос\n");
            exit(EXIT_FAILURE);
        }
    }

    close(sockfd);

    if (size == -1) 
    {
        perror("read error");
        exit(EXIT_FAILURE);
    }

    if (size == 0)
        printf("Соединение закрыто\n");

    exit(EXIT_SUCCESS);
}

int main() 
{
    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }

    key_t shmid, semid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;

    signal(SIGCHLD, SIG_IGN);

    shmid = shmget(IPC_PRIVATE, 4096, IPC_CREAT | perms);
    if (shmid == -1) 
    {
        perror("shmget error");
        exit(EXIT_FAILURE);
    }

    char *str = shmat(shmid, NULL, 0);
    if (str == (void *)-1) 
    {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    memset(str, ARRAY_ELEMENT_FREE, ARRAY_SIZE);

    for (int i = 0; i < ARRAY_SIZE; i++)
        str[i] = 'a' + i % 26;

    printf("Создан сегмент разделяемой памяти.\n");

    semid = semget(IPC_PRIVATE, 4, perms);
    if (semid == -1) 
    {
        perror("semget error");
        exit(EXIT_FAILURE);
    }
   
    if (semctl(semid, ACTIVE_READERS, SETVAL, 0) == -1) 
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, ACTIVE_WRITER, SETVAL, 0) == -1) 
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, WRITE_QUEUE, SETVAL, 0) == -1) 
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }


    if (semctl(semid, READ_QUEUE, SETVAL, 0) == -1) 
    {
        perror("semctl error");
        exit(EXIT_FAILURE);
    }

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

    while(flag) 
    {
        int connfd = accept(lstnfd, NULL, NULL);
        if (connfd == -1) 
        {
            perror("accept error");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid == -1) 
        {
            perror("fork error");
            close(connfd);
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) 
        {
            close(lstnfd);
            rw_func(str, connfd, semid, getppid());
        }
    }
    close (lstnfd);

    return 0;
}