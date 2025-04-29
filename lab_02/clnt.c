#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "serv.h"

int flag = 1;

int signal_handler(int sig_numb) 
{
    printf("Получен сигнал %d\n", sig_numb);
    flag = 0;
}

int main(int argc, char **argv) 
{
    srand(time(NULL));

    struct sigaction sa = { 0 };
    sa.sa_sigaction = &signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) 
    {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sadr;
    memset(&sadr, 0, sizeof(sadr));
    sadr.sin_family = AF_INET;
    sadr.sin_addr.s_addr = INADDR_ANY;
    sadr.sin_port = htons(PORT);

    while (flag)
    {
        int rc;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
        {
            perror("socket error");
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, (struct sockaddr *) &sadr, sizeof(sadr)) == -1) 
        {
            perror("connect error");
            exit(EXIT_FAILURE);
        }

        int type = htonl(READ);

        if (write(sockfd, &type, sizeof(type)) == -1) 
        {
            perror("write error");
            exit(EXIT_FAILURE);
        }

        char str[ARRAY_SIZE + 1];
        int size = read(sockfd, str, sizeof(str));
        if (size == -1) 
        {
            perror("read error");
            exit(EXIT_FAILURE);
        }

        if (size == 0) 
        {
            printf("Сервер оборвал соединение.\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        str[size] = '\0';

        printf("Массив: %s\n", str);
        sleep(1);

        int freeindex = -1;
        for (int i = 0; i < ARRAY_SIZE; i++) 
        {
            if (str[i] != ARRAY_ELEMENT_BUSY) 
            {
                freeindex = i;
                break;
            }
        }

        type = htonl(WRITE);
        if (write(sockfd, &type, sizeof(type)) == -1) 
        {
            perror("write error");
            exit(EXIT_FAILURE);
        }

        if (write(sockfd, &freeindex, sizeof(freeindex)) == -1) 
        {
            perror("write error");
            exit(EXIT_FAILURE);
        }

        size = read(sockfd, &rc, sizeof(rc));
        if (size == -1) 
        {
            perror("read error");
            exit(EXIT_FAILURE);
        }

        if (size == 0) 
        {
            printf("Сервер оборвал соединение.\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        rc = ntohl(rc);
        if (rc == WRITE_BUSY_ERROR)
            printf("Элемент с индексом %d занят\n", freeindex);
        else if (rc != OK) 
        {
            printf("Error: %d\n", rc);
            close(sockfd);
            exit(EXIT_FAILURE);
        } 
        else 
        {
            char result;
            size = read(sockfd, &result, sizeof(result));
            if (size == -1) 
            {
                perror("read error");
                exit(EXIT_FAILURE);
            }

            if (size == 0) 
            {
                printf("Сервер оборвал соединение.\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            }

            printf("Элемент %d: %c\n", freeindex, result);
        }

        close (sockfd);

        usleep(rand() % 6000000);
    }

    return 0;
}