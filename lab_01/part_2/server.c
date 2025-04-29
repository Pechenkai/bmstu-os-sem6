#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define SOCK_NAME "socket.socket"
#define BUFFER_SIZE 64

int fd;
time_t start_time;

void sigint_handler()
{
    close(fd);
    unlink(SOCK_NAME);
    exit(0);
}

void alarm_handler(int sig)
{
    printf("Получен сигнал: %d\n", sig);

    time_t current_time = time(NULL);
    close(fd);
    unlink(SOCK_NAME);
    alarm(0);
    exit(0);
}

int main() 
{
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("cant socket");
        exit(1);
    }
    
    struct sockaddr sockaddr = {.sa_family = AF_UNIX};
    strcpy(sockaddr.sa_data, SOCK_NAME);

    if (bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) == -1)
    {
        perror("cant bind");
        exit(1);
    }

    if (signal(SIGINT, sigint_handler) == (void *)-1)
    {
        perror("cant signal");
        exit(1);
    }

    if (signal(SIGALRM, alarm_handler) == (void *)-1)
    {
        perror("cant signal");
        exit(1);
    }

    start_time = time(NULL);
    alarm(10);

    char buf[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1)
    {
        int bytes_received = recvfrom(fd, buf, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);

        if (bytes_received == -1) 
        {
            perror("Can't recvfrom()");
            exit(1);
        }    
        else
        {
            buf[bytes_received] = '\0';
            printf("Received message: %s\n", buf);
        }
    }

    return 0;
}