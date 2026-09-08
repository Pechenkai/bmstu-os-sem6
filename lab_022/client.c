#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFERSIZE 26
#define SERV_PORT 9877
#define SERV_IP "127.0.0.1"

typedef enum type {
    READ,
    WRITE
} type_t;

void child_process() {
    int clntsock;
    char buffer[BUFFERSIZE];
    struct sockaddr_in servaddr;
    int index;
    int errorcode = 0;
    enum type type;
    clock_t start, end;
    long long all_time;
    
    char filename[256];
    snprintf(filename, sizeof(filename), "time/client_%d.txt", getpid());
    
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, SERV_IP, &servaddr.sin_addr) <= 0)
    {
        perror("Неверный IP-адрес или ошибка inet_pton");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL) ^ getpid());
    while (1)
    {
        clntsock = socket(AF_INET, SOCK_STREAM, 0);
        if (clntsock == -1)
        {
            perror("socket");
            break;
        }
        type = READ;
        start = clock();
        if (connect(clntsock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        {
            printf("Server is closed\n");
            break;
        }
        if (write(clntsock, &type, sizeof(type)) < 0)
        {
            perror("write_reader");
            break;
        }
        if (read(clntsock, buffer, sizeof(buffer)) < 0)
        {
            perror("read_writer");
            break;
        }
        index = -1;
        for (int i = 0; i < BUFFERSIZE; i++)
        {
            if (buffer[i] != ' ')
            {
                index = i;
                break;
            }
        }
        if (index == -1)
        {
            // break;
            index = 0;
        }
        type = WRITE;
        if (write(clntsock, &type, sizeof(type)) < 0)
        {
            perror("write_writer");
            break;
        }
        if (write(clntsock, &index, sizeof(index)) < 0)
        {
            perror("write_writer");
            break;
        }
        if (read(clntsock, &errorcode, sizeof(errorcode)) < 0)
        {
            perror("read_writer");
            break;
        }
        if (errorcode == 1)
        {
            // printf("[PID %d] Index %d occupied\n", getpid(), index);
        }
        errorcode = 0;
        close(clntsock);
        end = clock();
        all_time = end - start;
        fprintf(file, "%lld\n", all_time);
    }
    fclose(file);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number_of_processes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int num_processes = atoi(argv[1]);
    if (num_processes <= 0)
    {
        fprintf(stderr, "Number of processes must be positive\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_processes; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            sleep(30);
            child_process();
        }
    }
    return 0;
}