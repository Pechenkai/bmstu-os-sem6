#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <pthread.h>

#include "serv.h"

#define WRITE_QUEUE 0
#define READ_QUEUE 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define V 1
#define P -1

#define THREAD_OK 0
#define THREAD_EXIT_SUCCESS 1
#define THREAD_EXIT_FAILURE 2
#define THREAD_EXIT_ARRAY_FULL 3

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

int listenfd;
int array_counter = 0;
char array[ARRAY_SIZE];

int reader(char *buf, int socketfd, int semid) 
{
    char buffer[ARRAY_SIZE];
    
    int err = semop(semid, start_read, 5);
    if (err == -1) 
    {
        perror("semop\n");
        return THREAD_EXIT_FAILURE;
    }

    memcpy(buffer, buf, sizeof(buffer));
    err = semop(semid, end_read, 1);
    if (err == -1) 
    {
        perror("semop\n");
        return THREAD_EXIT_FAILURE;
    }

    int rc = htonl(OK);
    if (write(socketfd, &rc, sizeof(rc)) == -1) 
    {
        perror("write()");
        return THREAD_EXIT_FAILURE;
    }

    if (write(socketfd, &buffer, sizeof(buffer)) == -1) 
    {
        perror("write()");
        return THREAD_EXIT_FAILURE;
    }

    return THREAD_OK;
}

int writer(char *buf, int socketfd, int semid, int index) 
{
    int rc = OK;
    char result = '\0';
    int done = THREAD_OK;

    int err = semop(semid, start_write, 5);
    if (err == -1) 
    {
        perror("semop\n");
        return THREAD_EXIT_FAILURE;
    }

    if (index < 0 || index >= ARRAY_SIZE)
        rc = RANGE_ERROR;
    else if (buf[index] == ARRAY_ELEMENT_BUSY)
        rc = WRITE_BUSY_ERROR;
    else 
    {
        result = buf[index];
        buf[index] = ARRAY_ELEMENT_BUSY;
        array_counter++;
    }
    if (array_counter == ARRAY_SIZE)
        done = THREAD_EXIT_ARRAY_FULL;
    else
        printf("write request, index %d\n", index);
    
    err = semop(semid, end_write, 1);
    if (err == -1) 
    {
        perror("semop\n");
        return THREAD_EXIT_FAILURE;
    }

    rc = htonl(rc);
    if (write(socketfd, &rc, sizeof(rc)) == -1)
        perror("write()");
    
    if (result != '\0')
        if (write(socketfd, &result, sizeof(result)) == -1)
            perror("write()");
    
    return done;
}

typedef  void * (*threadfunc_t)(void *);
int threadfunc(void * arg) 
{
    struct thread_args *args = (struct thread_args *)arg;
    int semid = args->semid;
    int ppid = args->ppid;
    int sockfd = args->sockfd;
    int size;
    int type;

    for (; (size = read(sockfd, &type, sizeof(type))) > 0; ) 
    {
        type = ntohl(type);
        if (type == READ)
        {
            printf("Запрос чтения\n");
            int rc = reader(array, sockfd, semid);
            if (rc != THREAD_OK) 
            {
                close(sockfd);
                return rc;
            }
        }
        else if (type == WRITE)
        {
            int index;
            if ((size = read(sockfd, &index, sizeof(index))) < 0) 
            {
                perror("read error");
                close(sockfd);

                return THREAD_EXIT_FAILURE;
            } 
            else if (size == 0) 
            {
                printf("conn closed\n");
                close(sockfd);
                return THREAD_EXIT_SUCCESS;
            }

            int done = writer(array, sockfd, semid, index);
            if (done == THREAD_EXIT_ARRAY_FULL) 
            {
                close(sockfd);
                kill(ppid, SIGINT);
            } 
            else if (done != THREAD_OK) 
            {
                close(sockfd);
                return done;
            }
        }
        else
        {
            printf("Неизвестный запрос\n");
            int err = htonl(OP_ERROR);
            if (write(sockfd, &err, sizeof(err)) == -1) 
            {
                perror("write error");
                close(sockfd);
                return THREAD_EXIT_FAILURE;
            }
        }
    }

    close(sockfd);
    if (size == -1) 
    {
        perror("read error");
        return THREAD_EXIT_FAILURE;
    }
    if (size == 0) 
        printf("Соединение закрыто\n");
    
    return THREAD_EXIT_SUCCESS;
}

void sigint_handler() 
{
    if (close(listenfd) == -1) 
    {
        perror("close error");
        exit(EXIT_FAILURE);
    }

    printf("Сервер остановлен\n");
    exit(EXIT_SUCCESS);
}


int main() 
{
    key_t semid;

    memset(array, ARRAY_ELEMENT_FREE, ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) 
        array[i] = 'a' + i % 26;
    

    semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666);
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

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) 
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(PORT);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr *) &sa, sizeof(sa)) == -1) 
    {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 5) == -1) 
    {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на порту %d.\n", PORT);
    if (signal(SIGINT, sigint_handler) == (__sighandler_t) -1) 
    {
        perror("signal error");
        exit(EXIT_FAILURE);
    }

    while (1) 
    {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd == -1) 
        {
            perror("accept error");
            exit(EXIT_FAILURE);
        }

        pthread_t thread;
        struct thread_args *args = malloc(sizeof(struct thread_args));
        if (args == NULL) 
        {
            perror("malloc error");
            close(connfd);
            close(listenfd);
            exit(EXIT_FAILURE);
        }

        args->semid = semid;
        args->ppid = getpid();
        args->sockfd = connfd;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        int err = pthread_create(&thread, &attr, (threadfunc_t)threadfunc, args);
        if (err!= 0) 
        {
            perror("pthread_create error");
            free(args);
            close(connfd);
        }
    }
}