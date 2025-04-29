#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define SERVERNAME "serv.sock"

typedef enum operation_type
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} operation_type_t;

typedef struct operation
{
    double num1;
    double num2;
    operation_type_t op;
} operation_t;

char client_sock_name[13];
int client_sock;


void handle_sigint(int sig)
{
    unlink(client_sock_name);
    close(client_sock);
    exit(0);
}

void init_random_op(operation_t *op)
{
    op->num1 = rand() % 100 - 1;
    op->num2 = rand() % 100 + 1;
    op->op = rand() % 4;
}

void print_op(const operation_t *op)
{
    switch (op->op)
    {
    case OP_ADD:
        printf("%.2f + %.2f = ", op->num1, op->num2);
        break;
    case OP_SUB:
        printf("%.2f - %.2f = ", op->num1, op->num2);
        break;
    case OP_MUL:
        printf("%.2f * %.2f = ", op->num1, op->num2);
        break;
    case OP_DIV:
        printf("%.2f / %.2f = ", op->num1, op->num2);
        break;
    default:
        printf("Неизвестаня операция.\n");
        break;
    }
}

int main()
{
    struct sockaddr server_addr, client_addr;
    operation_t op;
    double result;
    srand(time(NULL));

    client_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client_sock == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    client_addr.sa_family = AF_UNIX;
    snprintf(client_sock_name, sizeof(client_sock_name), "%d", getpid());
    strncpy(client_addr.sa_data, client_sock_name, sizeof(client_addr.sa_data) - 1);

    if (bind(client_sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        perror("can`t bind");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    printf("Bind socket: %s\n", client_sock_name);
    server_addr.sa_family = AF_UNIX;
    strncpy(server_addr.sa_data, SERVERNAME, sizeof(server_addr.sa_data) - 1);
    signal(SIGINT, handle_sigint);

    while (1)
    {
        init_random_op(&op);
        if (sendto(client_sock, &op, sizeof(op), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            printf("Сервер закрыт.\n");
            close(client_sock);
            unlink(client_sock_name);
            exit(EXIT_FAILURE);
        }

        if (recvfrom(client_sock, &result, sizeof(result), 0, NULL, NULL) == -1)
        {
            perror("can`t recvfrom");
            close(client_sock);
            unlink(client_sock_name);
            exit(EXIT_FAILURE);
        }

        print_op(&op);
        printf("%.2f\n", result);

        sleep(rand() % 2 + 1);
    }

    close(client_sock);
    unlink(client_sock_name);

    return 0;
}
    