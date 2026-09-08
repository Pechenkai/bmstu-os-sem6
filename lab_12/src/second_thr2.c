#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

struct args_struct 
{ 
    int fd; 
};

void *read_buf(void *args)
{
    struct args_struct *cur_args = (struct args_struct *) args;
    int fd = cur_args->fd;
    int flag = 1;
    while (flag == 1)
    {
        char c;
        if ((flag = read(fd, &c, 1)) == 1)
            fprintf(stdout, "%c", c);
    }
    return NULL;
}

int main()
{ 
    pthread_t td1, td2;
    int fd1 = open("abc.txt", O_RDONLY);
    struct args_struct args1 = { .fd = fd1 };
    int fd2 = open("abc.txt", O_RDONLY);
    struct args_struct args2 = { .fd = fd2 };
    pthread_create(&td1, NULL, read_buf, &args1);
    pthread_create(&td2, NULL, read_buf, &args2);
    if (pthread_join(td1, NULL))
        return 1;
    if (pthread_join(td2, NULL))
        return 1;
    close(fd1);
    close(fd2);
    puts("");
    return 0;
}