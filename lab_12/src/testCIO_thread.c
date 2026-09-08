#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
struct args_struct
{
    FILE* fs;
    char buf[20];
};
void *read_buf(void *args)
{
    struct args_struct *cur_args = (struct args_struct *) args;
    FILE *fs = cur_args->fs;
    setvbuf(fs, cur_args->buf, _IOFBF, 20);
    int flag = 1;
    while (flag == 1)
    {
        char c;
        if ((flag = fscanf(fs, "%c", &c)) == 1)
            fprintf(stdout, "%c", c);
    }
    return NULL;
}
int main(void)
{
    pthread_t td1, td2, td3;
    int fd = open("alphabet.txt", O_RDONLY);
    FILE *fs1 = fdopen(fd, "r");
    FILE *fs2 = fdopen(fd, "r");
    char buff1[20];
    char buff2[20];
    setvbuf(fs1, buff1, _IOFBF, 20);
    setvbuf(fs2, buff2, _IOFBF, 20);
    struct args_struct args1 = { .fs = fs1 };
    struct args_struct args2 = { .fs = fs2 };
    pthread_create(&td1, NULL, read_buf, &args1);
    pthread_create(&td2, NULL, read_buf, &args2);
    if (pthread_join(td1, NULL))
        return 1;
    if (pthread_join(td2, NULL))
        return 1;
    fclose(fs1);
    fclose(fs2);
    return 0;
}