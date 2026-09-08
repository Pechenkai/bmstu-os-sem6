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
int main(void) {
    pthread_t td;
    FILE *fs1 = fopen("res.txt", "w");
    FILE *fs2 = fopen("res.txt", "w");
    struct file_struct args1 = { .fs = fs1, .start = 'a', .end = 'z'};
    struct file_struct args2 = { .fs = fs2, .start = 'b', .end = 'z'};
    file_info(fs1);
    file_info(fs2);
    pthread_create(&td, NULL, writer, &args1);
    writer(&args2);
    if (pthread_join(td, NULL))
        return 1;
    fclose(fs1);
    file_info(fs1);
    fclose(fs2);
    file_info(fs2);
    return 0;
}
