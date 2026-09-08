#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
struct file_struct {
    FILE* fs;
    char start;
    char end;
};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void file_info(FILE *fs)
{
	struct stat statbuf;
    pthread_mutex_lock(&mutex);
	stat("res.txt", &statbuf);
	printf("st_ino: %ld  ", statbuf.st_ino);
	printf("st_size: %ld  ", statbuf.st_size);
	printf("pos: %ld\n", ftell(fs));
    pthread_mutex_unlock(&mutex);
}
void* writer(void* arg) {
    struct file_struct* f = (struct file_struct*)arg;
    for (char c = f->start; c <= f->end; c += 2)
    {
        fprintf(f->fs, "%c", c);
        file_info(f->fs);
    }
    return NULL;
}
int main(void)
{
    pthread_t odd_thread, even_thread;
    FILE *fs1 = fopen("res.txt", "w");
    file_info(fs1);
    FILE *fs2 = fopen("res.txt", "w");
    file_info(fs2);
    struct file_struct odd_args = {fs1, 'a', 'z'};
    struct file_struct even_args = {fs2, 'b', 'z'};
    pthread_create(&odd_thread, NULL, writer, &odd_args);
    pthread_create(&even_thread, NULL, writer, &even_args);
    if (pthread_join(odd_thread, NULL))
        return 1;
    if (pthread_join(even_thread, NULL))
        return 1;
    fclose(fs1);
    file_info(fs1);
    fclose(fs2);
    file_info(fs2);
    return 0;
}