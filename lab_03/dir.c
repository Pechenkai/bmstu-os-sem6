#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#ifdef PATH_MAX
static int pathmax = PATH_MAX;
#else
static int pathmax = 0;
#endif

#define SUSV3 200112L

static long posix_version = 0;

#define PATH_MAX_GUESS 1024

#define FTW_F   1
#define FTW_D   2
#define FTW_DNR 3
#define FTW_NS  4

typedef int Myfunc(const char *, const struct stat *, int);

static long nreg, ndir, nblk, nchr, nfifo, nslink, nsock;
static char *fullpath;
static int  pathlen;

static int  g_depth = 0;

static Myfunc myfunc;
static int myftw(char *, Myfunc *);
static int dopath(Myfunc *, const char *);

char *path;

char *
path_alloc(int *sizep) 
{
    char *ptr;
    int size;
    if (posix_version == 0)
        posix_version = sysconf(_SC_VERSION);

    if (pathmax == 0) { 
        errno = 0;

        if ((pathmax = pathconf("/", _PC_PATH_MAX)) < 0) {
            if (errno == 0)
                pathmax = PATH_MAX_GUESS; 
            else
            {
                perror("ошибка вызова pathconf с параметром _PC_PATH_MAX");
                exit(EXIT_FAILURE);
            }
        }   
        else {
            pathmax++;
        }
    }

    if (posix_version < SUSV3)
        size = pathmax + 1;
    else
        size = pathmax;

    if ((ptr = malloc(size)) == NULL)
    {
        perror("ошибка функции malloc");
        exit(EXIT_FAILURE);
    }
\
    if (sizep != NULL)
        *sizep = size;

    return(ptr);
}

static int myftw(char *pathname, Myfunc *func)
{
    int len;
    fullpath = path_alloc(&len); 

    strncpy(fullpath, pathname, len);
    fullpath[len - 1] = 0;
    return(dopath(func, fullpath));
}

static void print_indent(void)
{
    for (int i = 0; i < g_depth; i++)
    {
        printf("    ");
    }
}

static int dopath(Myfunc* func, const char *path)
{
    struct stat statbuf;
    DIR *dp;
    struct dirent *dirp;
    int ret;

    if (lstat(path, &statbuf) < 0) {
        perror("Ошибка lstat.\n");
        exit(EXIT_FAILURE);
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        return func(path, &statbuf, FTW_F);
    }

    if ((ret = func(path, &statbuf, FTW_D)) != 0) {
        return ret;
    }

    g_depth++;

    if ((dp = opendir(path)) == NULL) {
        g_depth--;
        return func(path, &statbuf, FTW_DNR);
    }

    if (chdir(path) != 0) {
        perror("chdir(path)");
        closedir(dp);
        g_depth--;
        return -1; 
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 ||
            strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

        ret = dopath(func, dirp->d_name);
        if (ret != 0) {
            break;
        }
    }

    printf("XXX\n");

    closedir(dp);

    if (chdir("..") != 0) {
        perror("chdir(..)");
        return -1;
    }

    g_depth--;

    return ret;
}


static int
myfunc(const char *pathname, const struct stat *statptr, int type)
{
    print_indent();
    printf("%s\n", pathname);

    switch (type) {
    case FTW_F:
        switch (statptr->st_mode & S_IFMT) {
        case S_IFREG: nreg++; break;
        case S_IFBLK: nblk++; break;
        case S_IFCHR: nchr++; break;
        case S_IFIFO: nfifo++; break;
        case S_IFLNK: nslink++; break;
        case S_IFSOCK: nsock++; break;
        case S_IFDIR:
            printf("признак S_IFDIR для %s", pathname);
    }
    break;
    case FTW_D:
        ndir++;
        break;
    case FTW_DNR:
        printf("закрыт доступ к каталогу %s", pathname);
        break;
    case FTW_NS:
        printf("ошибка вызова функции stat для %s", pathname);
        break;
    default:
        printf("неизвестный тип %d для файла %s", type, pathname);
    }

    return(0);
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <начальный_каталог>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int ret = myftw(argv[1], myfunc);

    long ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
    if (ntot == 0)
        ntot = 1;

    printf("\n=== Статистика ===\n");
    printf("Обычные файлы         = %7ld, %5.2f %%\n", nreg,   nreg   * 100.0 / ntot);
    printf("Каталоги              = %7ld, %5.2f %%\n", ndir,   ndir   * 100.0 / ntot);
    printf("Блочные устройства    = %7ld, %5.2f %%\n", nblk,   nblk   * 100.0 / ntot);
    printf("Символьные устройства = %7ld, %5.2f %%\n", nchr,   nchr   * 100.0 / ntot);
    printf("FIFO                  = %7ld, %5.2f %%\n", nfifo,  nfifo  * 100.0 / ntot);
    printf("Символические ссылки  = %7ld, %5.2f %%\n", nslink, nslink * 100.0 / ntot);
    printf("Сокеты                = %7ld, %5.2f %%\n", nsock,  nsock  * 100.0 / ntot);

    free(fullpath);
    return ret;
}