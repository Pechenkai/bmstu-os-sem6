#include "hf.h"
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    int pid_num = atoi(argv[1]); 

    char *pid = argv[1];
    FILE *common = fopen("common.txt", "w");
    if (!common)
    {
        perror("fopen() error");
        exit(1);
    }
    FILE *environ = fopen("environ.txt", "w");
    if (!environ)
    {
        perror("fopen() error");
        exit(1);
    }
    FILE *maps = fopen("maps.txt", "w");
    if (!maps)
    {
        perror("fopen() error");
        exit(1);
    }
    FILE *mem = fopen("mem.txt", "w");
    if (!mem)
    {
        perror("fopen() error");
        exit(1);
    }
    FILE *pagemap = fopen("pagemap.txt", "w");
    if (!pagemap)
    {
        perror("fopen() error");
        exit(1);
    }
    FILE *stat = fopen("stat.txt", "w");
    if (!stat)
    {
        perror("fopen() error");
        exit(1);
    }

    get_cmdline_info(pid, common);
    get_cwd_info(pid, common);
    get_environ_info(pid, environ);
    get_exe_info(pid, common);
    get_fd_info(pid, common);
    get_maps_info(pid, maps);
    fclose(maps);
    get_root_info(pid, common);
    get_stat_info(pid, stat);
    int present = get_pagemap_info(pid, pagemap);
    // printf("present = %d\n", present);
    get_task_info(pid, common);
    get_mem_info(pid_num, mem);
    fclose(mem);

    fclose(common);
    fclose(environ);
    fclose(pagemap);
    fclose(stat);

    return 0;
}