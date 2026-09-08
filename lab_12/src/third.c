#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    int fd1 = open("abc2.txt", O_RDWR | O_CREAT);
    int fd2 = open("abc2.txt", O_RDWR | O_CREAT);
    for (char c = 'a'; c <= 'z'; c++)
    {
        if (c % 2)
            write(fd1, &c, 1);
        else
            write(fd2, &c, 1);
    }
    close(fd1);
    close(fd2);
    return 0;
}