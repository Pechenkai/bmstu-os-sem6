#include <bits/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>


#define MAX_EVENTS 10
#define MSGSIZE 256
#define BUFFERSIZE 26
#define SERV_PORT 9877
int fl = 1;
int counter = 0;
long long elapsed_time = 0;
long long med = 0;

clock_t start, end;

typedef enum type
{
    READ,
    WRITE
} type_t;
char buf[BUFFERSIZE];
void reader(int connfd)
{
    // response_t resp = {0};
    int err = 0;
    strncpy(buf, buf, BUFFERSIZE);
    if (write(connfd, buf, BUFFERSIZE) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}
void writer(int connfd)
{
    int index;
    int err = 0;
    int e = 0;
    while (read(connfd, &index, sizeof(index)) < 0)
    {
        // if (errno == EAGAIN)
        // {
        //     continue;
        // }
        perror("read");
        exit(EXIT_FAILURE);
    }
    if (index >= 0 && index < BUFFERSIZE)
    {
        if (buf[index] == ' ')
        {
            // e = 1;
            buf[index] = 'a' + index;
        }
        else
        {
            buf[index] = ' ';
            // printf("Booked buf[%d]\n", index);
        }
    }
    else
    {
        // e = -1;
        index = 0;
    }
    if (write(connfd, &e, sizeof(e)) < 0)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
}

void sigint_handler(int signo)
{
    exit(EXIT_SUCCESS);
}
void request_type(int connfd)
{
    struct timespec start, end;
    int n;
    enum type type;

    // clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 2; i++)
    {
        alarm(5);
        while ((n = read(connfd, &type, sizeof(type))) != sizeof(type))
        {
            // if (errno == EAGAIN)
            // {
            //     continue;
            // }
            exit(EXIT_SUCCESS);
        }
        alarm(0);
        switch (type)
        {
            case READ:
                reader(connfd);
                break;
            case WRITE:
                writer(connfd);
                break;
            default:
                printf("Invalid request type %d\n", type);
                break;
        }
    }
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // counter++;
}
int main(void)
{
    int listenfd, connfd;
    struct sockaddr_in cliaddr, servaddr;
    socklen_t clilen;
    int epollfd, nfds;
    struct epoll_event ev, events[MAX_EVENTS];
    for (int i = 0; i < BUFFERSIZE; i++)
    {
        buf[i] = 'a' + i;
    }

    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigint_handler);
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Ошибка при установке SO_REUSEADDR");
        close(listenfd);
        exit(EXIT_FAILURE);
    }
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 5) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    epollfd = epoll_create(10);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
    {
        perror("epoll_ctl: listenfd");
        exit(EXIT_FAILURE);
    }
    FILE *file = fopen("time/epoll_time.txt", "w");
    // while (1)
    for (int i = 0; i < 10000; i ++)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        // clock_gettime(CLOCK_MONOTONIC, &start);
        start = clock();
        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                if (connfd == -1)
                {
                    perror("accept");
                    continue;
                }
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                // if (fcntl(ev.data.fd, F_SETFL, O_NONBLOCK) == -1)
                // {
                //     perror("fcntl");
                //     close(connfd);
                //     continue;
                // }
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                {
                    perror("epoll_ctl: connfd");
                    close(connfd);
                    continue;
                }
            }
            else
            {
                request_type(events[n].data.fd);
                close(events[n].data.fd);
            }
        }
        // clock_gettime(CLOCK_MONOTONIC, &end);
        // elapsed_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        end = clock();
        elapsed_time = end - start;
        med += elapsed_time;
        counter++;
        fprintf(file,"%lld\n", elapsed_time);
        // printf("%lld\n", med / counter);
    }
    fclose(file);

    
    close(listenfd);
    close(epollfd);
    return 0;
}
