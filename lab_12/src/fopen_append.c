#include <stdio.h>
#include <sys/stat.h>

static inline void file_info(FILE *fs)
{
	struct stat statbuf;
	stat("res.txt", &statbuf);
	printf("st_ino: %ld  ", statbuf.st_ino);
	printf("st_size: %ld  ", statbuf.st_size);
	printf("pos: %ld\n", ftell(fs));
}
int main(void)
{
    FILE *fs1 = fopen("res.txt", "a");
    file_info(fs1);
    FILE *fs2 = fopen("res.txt", "a");
    file_info(fs2);
    for (char ch = 'a'; ch <= 'z'; ++ch)
    {
        if (ch % 2)
        {
            fprintf(fs1, "%c", ch);
            file_info(fs1);
        }
        else
        {
            fprintf(fs2, "%c", ch);
            file_info(fs2);
        }
    }
    fclose(fs1);
    file_info(fs1);
    fclose(fs2);
    file_info(fs2);

    return 0;
}