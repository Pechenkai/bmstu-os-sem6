#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
struct file_struct {
    int fd;
    char start;
    char end;
};
void* writer(void* arg) {
    struct file_struct* f = (struct file_struct*)arg;
    for (char c = f->start; c <= f->end; c += 2)
        write(f->fd, &c, 1);
    return NULL;
}
int main(void)
{
    pthread_t odd_thread, even_thread;
    int fd1 = open("abc2.txt", O_RDWR | O_CREAT);
    int fd2 = open("abc2.txt", O_RDWR | O_CREAT);
    struct file_struct odd_args = {fd1, 'a', 'z'};
    struct file_struct even_args = {fd2, 'b', 'z'};
    pthread_create(&odd_thread, NULL, writer, &odd_args);
    pthread_create(&even_thread, NULL, writer, &even_args);
    if (pthread_join(odd_thread, NULL))
        return 1;
    if (pthread_join(even_thread, NULL))
        return 1;
    close(fd1);
    close(fd2);
    return 0;
}