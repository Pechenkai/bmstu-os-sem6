#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

#define SERVERNAME "serv.sock"

typedef enum
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} operation_type_t;

typedef struct
{
    double num1;
    double num2;
    operation_type_t op;
} operation_t;

int server_sock;

void handle_sigint(int sig)
{
    close(server_sock);
    unlink(SERVERNAME);
    exit(0);
}

void print_recv_op(const operation_t *op)
{
    switch (op->op)
    {
    case OP_ADD:
        printf("%.2f + %.2f\n", op->num1, op->num2);
        break;
    case OP_SUB:
        printf("%.2f - %.2f\n", op->num1, op->num2);
        break;
    case OP_MUL:
        printf("%.2f * %.2f\n", op->num1, op->num2);
        break;
    case OP_DIV:
        printf("%.2f / %.2f\n", op->num1, op->num2);
        break;
    default:
        printf("Неизвестная операция.\n");
        break;
    }
}

int main()
{
    struct sockaddr server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    operation_t op;
    double result;
    ssize_t recv_len;

    server_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (server_sock == -1)
    {
        perror("can`t sock.");
        exit(EXIT_FAILURE);
    }

    server_addr.sa_family = AF_UNIX;
    strncpy(server_addr.sa_data, SERVERNAME, sizeof(server_addr.sa_data) - 1);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("can`t bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        recv_len = recvfrom(server_sock, &op, sizeof(op), 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len == -1)
        {
            perror("can`t recvfrom");
            continue;
        }

        printf("Получено от %s: ", client_addr.sa_data);
        print_recv_op(&op);

        switch (op.op)
        {
            case OP_ADD:
                result = op.num1 + op.num2;
                break;
            case OP_SUB:
                result = op.num1 - op.num2;
                break;
            case OP_MUL:
                result = op.num1 * op.num2;
                break;
            case OP_DIV:
                if (op.num2 != 0)
                    result = op.num1 / op.num2;
                else
                    perror("Деление на 0");
                break;
            default:
                printf("Неизвестная операция.\n");
                continue;
        }

        printf("Результат отправлен %s: %.2f\n", client_addr.sa_data, result);
        if (sendto(server_sock, &result, sizeof(result), 0, (struct sockaddr *)&client_addr, client_len) == -1)
        {
            perror("can`t sendto");
        }
    }

    close(server_sock);
    unlink(SERVERNAME);

    return 0;
}