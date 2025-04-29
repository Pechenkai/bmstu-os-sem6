#include "hf.h"

void print_page(uint64_t address, uint64_t data, FILE *out)
{
    fprintf(out, "0x%-15lx : %-10lx %-10ld %-10ld %-10ld %-10ld\n", address,
            data & (((uint64_t)1 << 55) - 1), (data >> 55) & 1, (data >> 61) & 1, (data >> 62) & 1, (data >> 63) & 1);
}

int get_pagemap_info(const char *proc, FILE *out)
{
    fprintf(out, "     addr         pfn  soft-dirty    file/shared    swapped     present\n");

    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/maps", proc);
    FILE *maps = fopen(path, "r");

    snprintf(path, PATH_LEN, "/proc/%s/pagemap", proc);
    int pm_fd = open(path, O_RDONLY);

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    int present = 0;

    // чтение maps
    while ((len = fread(buf, 1, BUF_SIZE, maps)) > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (buf[i] == 0)
            {
                buf[i] = '\n';
            }
        }
        buf[len] = '\0';

        // проход по строкам из maps
        char *save_row;
        char *row = strtok_r(buf, "\n", &save_row);

        while (row)
        {
            // получение столбца участка адресного пространства
            char *addresses = strtok(row, " ");
            char *start_str, *end_str;

            // получение начала и конца участка адресного пространства
            if ((start_str = strtok(addresses, "-")) && (end_str = strtok(NULL, "-")))
            {
                uint64_t start = strtoul(start_str, NULL, 16);
                uint64_t end = strtoul(end_str, NULL, 16);

                for (uint64_t i = start; i < end; i += sysconf(_SC_PAGE_SIZE))
                {
                    uint64_t offset;
                    // поиск смещения, по которому в pagemap находится информация о текущей странице
                    uint64_t index = i / sysconf(_SC_PAGE_SIZE) * sizeof(offset);

                    pread(pm_fd, &offset, sizeof(offset), index);
                    print_page(i, offset, out);
                    if (((offset >> 63) & 1) == 1)
                    {
                        present++;
                    }
                }
            }

            row = strtok_r(NULL, "\n", &save_row);
        }
    }

    fclose(maps);
    close(pm_fd);
    return present;
}
void get_maps_info(const char *proc, FILE *out)
{
    fprintf(out, "address                   n_pages  perms  offset  dev   inode                 pathname\n");
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/maps", proc);
    FILE *maps = fopen(path, "r");
    if (!maps) {
        perror("Failed to open maps file");
        return;
    }

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    while ((len = fread(buf, 1, BUF_SIZE, maps)) > 0)
    {
        for (int i = 0; i < len; i++)
            if (buf[i] == 0)
                buf[i] = '\n';
        buf[len] = '\0';
        if (buf[1] == '\n' && buf[0] == ' ')
            buf[1] = ' ';

        char *save_row;
        char *row = strtok_r(buf, "\n", &save_row);

        while (row)
        {
            char *addresses = strtok(row, " ");
            char *perms = strtok(NULL, " ");
            char *offset = strtok(NULL, " ");
            char *dev = strtok(NULL, " ");
            char *inode = strtok(NULL, " ");
            char *pathname = strtok(NULL, "\n");

            char *start_str = strtok(addresses, "-");
            char *end_str = strtok(NULL, "-");

            if (start_str && end_str)
            {
                uint64_t start = strtoul(start_str, NULL, 16);
                uint64_t end = strtoul(end_str, NULL, 16);
                uint64_t n_pages = (end - start) / 4096; // Размер страницы обычно 4096 байт
                fprintf(out, "%s-%s  %-7lu  %s  %s  %s  %s  %s\n", start_str, end_str, n_pages, perms, offset, dev, inode, pathname ? pathname : "");
            }
            else
            {
                fprintf(out, "%s\n", row);
            }

            row = strtok_r(NULL, "\n", &save_row);
        }
    }

    fclose(maps);
}

void get_stat_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/stat", proc);
    FILE *stat = fopen(path, "r");
    if (!stat)
    {
        perror("Failed to open stat file");
        return;
    }

    char *headers[] = {"pid", "comm", "state", "ppid", "pgrp", "session", "tty_nr", "tpgid", "flags", "minflt", "cminflt", "majflt", "cmajflt", "utime", "stime", "cutime", "cstime", "priority", "nice", "num_threads", "itrealvalue", "starttime", "vsize", "rss", "rsslim", "startcode", "endcode", "startstack", "kstkesp", "kstkeip", "signal", "blocked", "sigignore", "sigcatch", "wchan", "nswap", "cnswap", "exit_signal", "processor", "rt_priority", "policy", "delayacct_blkio_ticks", "guest_time", "cguest_time", "start_data", "end_data", "start_brk", "arg_start", "arg_end", "env_start", "env_end", "exit_code"};

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    while ((len = fread(buf, 1, BUF_SIZE, stat)) > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (buf[i] == 0)
            {
                buf[i] = '\n';
            }
        }
        buf[len] = '\0';

        char *save_row;
        char *row = strtok_r(buf, " ", &save_row);
        int i = 0;
        while (row && i < sizeof(headers) / sizeof(headers[0]))
        {
            fprintf(out, "%s  %s", headers[i], row);

            if (strcmp(headers[i], "vsize") == 0)
            {
                unsigned long vsize_bytes = strtoul(row, NULL, 10);
                double vsize_mb = vsize_bytes / (1024.0 * 1024.0);
                fprintf(out, " (%.2f MB) (%d pages)", vsize_mb, vsize_bytes / 4096);
            }
            else if (strcmp(headers[i], "rss") == 0)
            {
                unsigned long rss_pages = strtoul(row, NULL, 10);
                double rss_mb = (rss_pages * 4096) / (1024.0 * 1024.0);
                fprintf(out, " (%.2f MB)", rss_mb);
            }
            fprintf(out, "\n");
            row = strtok_r(NULL, " ", &save_row);
            i++;
        }
    }

    fclose(stat);
}

void get_task_info(const char *proc, FILE *out)
{
    DIR *d;
    struct dirent *dir;
    char dir_path[PATH_LEN];
    snprintf(dir_path, PATH_LEN, "/proc/%s/task/", proc);
    d = opendir(dir_path);
    if (!d)
    {
        perror("Failed to open task directory");
        return;
    }
    fprintf(out, "\nTHREADS (User TID -> Kernel TID)\n");

    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_name[0] == '.')
            continue;

        char *user_tid = dir->d_name;

        char status_path[PATH_LEN];
        snprintf(status_path, PATH_LEN, "/proc/%s/task/%s/status", proc, user_tid);

        FILE *status_file = fopen(status_path, "r");
        if (!status_file)
        {
            perror("Failed to open status file");
            continue;
        }
        char kernel_tid[64] = "N/A"; // По умолчанию, если не найдено
        char line[BUF_SIZE];
        while (fgets(line, sizeof(line), status_file))
        {
            if (strncmp(line, "Pid:", 4) == 0)
            {
                sscanf(line, "Pid:\t%s", kernel_tid);
                break;
            }
        }
        fclose(status_file);
        fprintf(out, "%s -> %s\n", user_tid, kernel_tid);
    }

    closedir(d);
}

void get_cmdline_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/cmdline", proc);
    FILE *cmdline = fopen(path, "r");
    if (!cmdline) {
        perror("Failed to open cmdline file");
        return;
    }

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    while ((len = fread(buf, 1, BUF_SIZE, cmdline)) > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (buf[i] == 0)
            {
                buf[i] = '\n';
            }
        }
        buf[len] = '\0';
        fprintf(out, "%s", buf);
    }

    fclose(cmdline);
}

void get_cwd_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/cwd", proc);
    char buf[BUF_SIZE];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) {
        perror("Failed to read cwd link");
        return;
    }
    buf[len] = '\0';
    fprintf(out, "cwd: %s\n", buf);
}

void get_environ_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/environ", proc);
    FILE *environ = fopen(path, "r");
    if (!environ) {
        perror("Failed to open environ file");
        return;
    }

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    while ((len = fread(buf, 1, BUF_SIZE, environ)) > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (buf[i] == 0)
            {
                buf[i] = '\n';
            }
        }
        buf[len] = '\0';
        fprintf(out, "%s", buf);
    }

    fclose(environ);
}

void get_exe_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/exe", proc);
    char buf[BUF_SIZE];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) {
        perror("Failed to read exe link");
        return;
    }
    buf[len] = '\0';
    fprintf(out, "exe: %s\n", buf);
}

void get_fd_info(const char *proc, FILE *out)
{
    DIR *d;
    struct dirent *dir;
    char dir_path[PATH_LEN];
    snprintf(dir_path, PATH_LEN, "/proc/%s/fd/", proc);
    d = opendir(dir_path);
    if (!d) {
        perror("Failed to open fd directory");
        return;
    }
    fprintf(out, "\nFILE DESCRIPTORS\n");
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_name[0] == '.') // Пропускаем "." и ".."
            continue;

        char fd_path[PATH_LEN];
        snprintf(fd_path, PATH_LEN, "/proc/%s/fd/%s", proc, dir->d_name);

        char buf[BUF_SIZE];
        ssize_t len = readlink(fd_path, buf, sizeof(buf) - 1);
        if (len == -1) {
            perror("Failed to read fd link");
            continue;
        }
        buf[len] = '\0';
        fprintf(out, "%s -> %s\n", dir->d_name, buf);
    }
    closedir(d);
}

void get_mem_info_2(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/mem", proc);
    FILE *mem = fopen(path, "r");
    if (!mem)
    {
        perror("Failed to open mem file");
        fprintf(out, "Cannot access memory of process %s. Ensure you have sufficient permissions (run as root).\n", proc);
        return;
    }

    char buf[BUF_SIZE + 1] = "\0";
    int len;

    while ((len = fread(buf, 1, BUF_SIZE, mem)) > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (buf[i] == 0)
            {
                buf[i] = '\n';
            }
        }
        buf[len] = '\0';
        fprintf(out, "%s", buf);
    }

    fclose(mem);
}

typedef struct {
    unsigned long start_addr;
    unsigned long end_addr;
} MemoryRegion;

void print_region(int mem_fd, unsigned long start_addr,
    unsigned long end_addr, FILE * stream)
{
    size_t region_size = end_addr - start_addr;
    void * buffer = malloc (region_size);
    if (!buffer)
    {
        perror ("malloc");
        return;
    }
    if (lseek (mem_fd, start_addr, SEEK_SET) == (off_t) -1)
    {
        perror ("lseek");
        free (buffer);
        return;
    }
    ssize_t bytes_read = read(mem_fd, buffer, region_size);
    if (bytes_read == -1)
    {
        perror ("read");
        free (buffer);
        return;
    }
    fprintf(stream, " Memory region: %lx-%lx\n", start_addr, end_addr);
    for (size_t i = 0; i < region_size; i ++)
    {
        fprintf (stream, "%02x ", ((unsigned char *) buffer) [i]);
        if ((i + 1) % 16 == 0)
            fprintf (stream, "\n");
    }
    fprintf (stream, "\n");
    free (buffer);
}

MemoryRegion * get_memory_regions(int pid, size_t * region_count)
{
    char path [PATH_MAX];
    snprintf (path, PATH_MAX, "/proc/%d/maps", pid);
    FILE *file = fopen(path, "r");
    if (!file)
    {
        perror ("fopen");
        return NULL;
    }
    size_t capacity = 10;
    MemoryRegion * regions = malloc(capacity * sizeof (MemoryRegion));
    if (!regions)
    {
        perror ("malloc");
        fclose (file);
        return NULL;
    }
    char *line = NULL;
    size_t line_size = 0;
    size_t count = 0;
    while (getline (& line, & line_size, file) != -1)
    {
        unsigned long start_addr, end_addr;
        if (sscanf (line, "%lx-%lx", &start_addr, &end_addr) == 2)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                MemoryRegion * new_regions = realloc (regions, capacity * sizeof (MemoryRegion));
                if (!new_regions)
                {
                    perror ("realloc");
                    free (regions);
                    fclose (file);
                    free (line);
                    return NULL;
                }
                regions = new_regions;
            }
            regions [count]. start_addr = start_addr;
            regions [count]. end_addr = end_addr;
            count ++;
        }
    }
    fclose (file);
    free (line);
    *region_count = count;
    return regions;
}

void get_mem_info(int pid, FILE * stream)
{
    size_t region_count = 0;
    MemoryRegion *regions = get_memory_regions(pid, &region_count);
    if (!regions)
    {
        fprintf (stderr, "Failed to get memory regions\n");
        return;
    }
    char mem_path [PATH_MAX];
    snprintf(mem_path, PATH_MAX, "/proc/%d/mem", pid);
    int mem_fd = open(mem_path, O_RDONLY);
    if (mem_fd == -1)
    {
        perror ("open");
        free (regions);
        return;
    }
    for (size_t i = 1; i < 3; i ++) {
        fprintf (stream, "Region %zu: %lx-%lx\n", i, regions[i].start_addr, regions[i].end_addr);
        print_region (mem_fd, regions[i]. start_addr, regions[i].end_addr, stream);
    }
    close (mem_fd);
    free (regions);
}

void get_root_info(const char *proc, FILE *out)
{
    char path[PATH_LEN];
    snprintf(path, PATH_LEN, "/proc/%s/root", proc);
    char buf[BUF_SIZE];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) {
        perror("Failed to read root link");
        return;
    }
    buf[len] = '\0';
    fprintf(out, "root: %s\n", buf);
}