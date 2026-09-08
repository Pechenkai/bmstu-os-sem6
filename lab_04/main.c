#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <limits.h>

#define BUF_SIZE 4096

static FILE* open_section(const char *base, const char *section, char *out_path, size_t out_sz)
{
    snprintf(out_path, out_sz, "%s_%s.txt", base, section);
    FILE *f = fopen(out_path, "w");
    if (!f) {
        fprintf(stderr, "Ошибка при открытии %s: %m\n", out_path);
    }
    return f;
}

void parse_environ(const char *env_var, FILE *out)
{
    char var_name[256];
    int i = 0;
    while (env_var[i] != '=' && env_var[i] != '\0' && i < 255)
    {
        var_name[i] = env_var[i];
        i++;
    }
    var_name[i] = '\0';
    fprintf(out, "%s = %s\n", var_name, env_var + i + 1);
}

void print_environ(const char *pid, FILE *out)
{
    fprintf(out, "\nПеременные окружения (regular file proc/<pid>/environ)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/environ", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла environ: %m\n");
        return;
    }

    char buf[BUF_SIZE];
    size_t bytes;
    char *env_var = NULL;
    size_t env_var_len = 0;

    while ((bytes = fread(buf, 1, BUF_SIZE, f)) > 0)
    {
        for (size_t i = 0; i < bytes; i++)
        {
            if (buf[i] == '\0')
            {
                if (env_var)
                {
                    parse_environ(env_var, out);
                    free(env_var);
                    env_var = NULL;
                    env_var_len = 0;
                }
            }
            else
            {
                env_var = realloc(env_var, env_var_len + 2);
                env_var[env_var_len++] = buf[i];
                env_var[env_var_len] = '\0';
            }
        }
    }

    if (env_var)
    {
        parse_environ(env_var, out);
        free(env_var);
    }

    fclose(f);
}

const char *get_state_description(char state)
{
    switch (state)
    {
    case 'R':
        return "Running";
    case 'S':
        return "Sleeping";
    case 'D':
        return "Disk sleep";
    case 'Z':
        return "Zombie";
    case 'T':
        return "Stopped";
    case 't':
        return "Tracing stop";
    case 'X':
        return "Dead";
    case 'I':
        return "Idle";
    case 'P':
        return "Parked";
    default:
        return "Unknown";
    }
}

void print_stat(const char *pid, FILE *out)
{
    fprintf(out, "\nСостояние процесса (Regular file /proc/<pid>/stat)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла stat: %m\n");
        return;
    }

    char buf[BUF_SIZE];
    if (fgets(buf, BUF_SIZE, f))
    {
        char *tokens[53];
        int token_count = 0;

        char *comm_start = strchr(buf, '(');
        char *comm_end = strrchr(buf, ')');

        if (comm_start && comm_end && comm_end > comm_start)
        {
            *comm_start = '\0';
            *comm_end = '\0';

            tokens[token_count++] = buf;

            tokens[token_count++] = comm_start + 1;

            char *rest = comm_end + 2;
            char *token = strtok(rest, " ");

            while (token && token_count < 52)
            {
                tokens[token_count++] = token;
                token = strtok(NULL, " ");
            }
        }

        fprintf(out, "1) pid: %s\n",
                token_count > 0 ? tokens[0] : "N/A");

        fprintf(out, "2) comm: %s\n",
                token_count > 1 ? tokens[1] : "N/A");

        fprintf(out, "3) state: %s - %s\n",
                token_count > 2 ? tokens[2] : "N/A",
                token_count > 2 ? get_state_description(tokens[2][0]) : "N/A");

        fprintf(out, "4) ppid: %s\n",
                token_count > 3 ? tokens[3] : "N/A");

        fprintf(out, "5) pgrp: %s\n",
                token_count > 4 ? tokens[4] : "N/A");

        fprintf(out, "6) session: %s\n",
                token_count > 5 ? tokens[5] : "N/A");

        fprintf(out, "7) tty_nr: %s\n",
                token_count > 6 ? tokens[6] : "N/A");

        fprintf(out, "8) tpgid: %s\n",
                token_count > 7 ? tokens[7] : "N/A");

        fprintf(out, "9) flags: %s\n",
                token_count > 8 ? tokens[8] : "N/A");

        fprintf(out, "10) minflt: %s\n",
                token_count > 9 ? tokens[9] : "N/A");

        fprintf(out, "11) cminflt: %s\n",
                token_count > 10 ? tokens[10] : "N/A");

        fprintf(out, "12) majflt: %s\n",
                token_count > 11 ? tokens[11] : "N/A");

        fprintf(out, "13) cmajflt: %s\n",
                token_count > 12 ? tokens[12] : "N/A");

        fprintf(out, "14) utime: %s\n",
                token_count > 13 ? tokens[13] : "N/A");

        fprintf(out, "15) stime: %s\n",
                token_count > 14 ? tokens[14] : "N/A");

        fprintf(out, "16) cutime: %s\n",
                token_count > 15 ? tokens[15] : "N/A");

        fprintf(out, "17) cstime: %s\n",
                token_count > 16 ? tokens[16] : "N/A");

        fprintf(out, "18) priority: %s\n",
                token_count > 17 ? tokens[17] : "N/A");

        fprintf(out, "19) nice: %s\n",
                token_count > 18 ? tokens[18] : "N/A");

        fprintf(out, "20) num_threads: %s\n",
                token_count > 19 ? tokens[19] : "N/A");

        fprintf(out, "21) itrealvalue: %s\n",
                token_count > 20 ? tokens[20] : "N/A");

        fprintf(out, "22) starttime: %s\n",
                token_count > 21 ? tokens[21] : "N/A");

        fprintf(out, "23) vsize: %s размер виртуальной памяти\n",
                token_count > 22 ? tokens[22] : "N/A");

        fprintf(out, "24) rss: %s\n",
                token_count > 23 ? tokens[23] : "N/A");

        fprintf(out, "25) rsslim: %s\n",
                token_count > 24 ? tokens[24] : "N/A");

        char hex_addr[32];
        unsigned long addr_value;

        if (token_count > 25 && sscanf(tokens[25], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[25], addr_value);
            fprintf(out, "26) startcode: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "26) startcode: %s\n",
                    token_count > 25 ? tokens[25] : "N/A");
        }

        if (token_count > 26 && sscanf(tokens[26], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[26], addr_value);
            fprintf(out, "27) endcode: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "27) endcode: %s\n",
                    token_count > 26 ? tokens[26] : "N/A");
        }

        // startstack
        if (token_count > 27 && sscanf(tokens[27], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[27], addr_value);
            fprintf(out, "28) startstack: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "28) startstack: %s\n",
                    token_count > 27 ? tokens[27] : "N/A");
        }

        fprintf(out, "29) kstkesp: %s\n",
                token_count > 28 ? tokens[28] : "N/A");

        fprintf(out, "30) kstkeip: %s\n",
                token_count > 29 ? tokens[29] : "N/A");

        fprintf(out, "31) signal: %s\n",
                token_count > 30 ? tokens[30] : "N/A");

        fprintf(out, "32) blocked: %s\n",
                token_count > 31 ? tokens[31] : "N/A");

        fprintf(out, "33) sigignore: %s\n",
                token_count > 32 ? tokens[32] : "N/A");

        fprintf(out, "34) sigcatch: %s\n",
                token_count > 33 ? tokens[33] : "N/A");

        fprintf(out, "35) wchan: %s\n",
                token_count > 34 ? tokens[34] : "N/A");

        fprintf(out, "36) nswap: %s\n",
                token_count > 35 ? tokens[35] : "N/A");

        fprintf(out, "37) cnswap: %s\n",
                token_count > 36 ? tokens[36] : "N/A");

        fprintf(out, "38) exit_signal: %s\n",
                token_count > 37 ? tokens[37] : "N/A");

        fprintf(out, "39) processor: %s\n",
                token_count > 38 ? tokens[38] : "N/A");

        fprintf(out, "40) rt_priority: %s\n",
                token_count > 39 ? tokens[39] : "N/A");

        fprintf(out, "41) policy: %s\n",
                token_count > 40 ? tokens[40] : "N/A");

        fprintf(out, "42) delayacct_blkio_ticks: %s\n",
                token_count > 41 ? tokens[41] : "N/A");

        fprintf(out, "43) guest_time: %s\n",
                token_count > 42 ? tokens[42] : "N/A");

        fprintf(out, "44) cguest_time: %s\n",
                token_count > 43 ? tokens[43] : "N/A");

        // start_data
        if (token_count > 44 && sscanf(tokens[44], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[44], addr_value);
            fprintf(out, "45) start_data: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "45) start_data: %s\n",
                    token_count > 44 ? tokens[44] : "N/A");
        }

        // end_data
        if (token_count > 45 && sscanf(tokens[45], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[45], addr_value);
            fprintf(out, "46) end_data: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "46) end_data: %s\n",
                    token_count > 45 ? tokens[45] : "N/A");
        }

        // start_brk
        if (token_count > 46 && sscanf(tokens[46], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[46], addr_value);
            fprintf(out, "47) start_brk: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "47) start_brk: %s\n",
                    token_count > 46 ? tokens[46] : "N/A");
        }

        // arg_start
        if (token_count > 47 && sscanf(tokens[47], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[47], addr_value);
            fprintf(out, "48) arg_start: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "48) arg_start: %s\n",
                    token_count > 47 ? tokens[47] : "N/A");
        }

        // arg_end
        if (token_count > 48 && sscanf(tokens[48], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[48], addr_value);
            fprintf(out, "49) arg_end: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "49) arg_end: %s\n",
                    token_count > 48 ? tokens[48] : "N/A");
        }

        // env_start
        if (token_count > 49 && sscanf(tokens[49], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[49], addr_value);
            fprintf(out, "50) env_start: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "50) env_start: %s\n",
                    token_count > 49 ? tokens[49] : "N/A");
        }

        // env_end
        if (token_count > 50 && sscanf(tokens[50], "%lu", &addr_value) == 1)
        {
            snprintf(hex_addr, sizeof(hex_addr), "%s (0x%lx)", tokens[50], addr_value);
            fprintf(out, "51) env_end: %s\n", hex_addr);
        }
        else
        {
            fprintf(out, "51) env_end: %s\n",
                    token_count > 50 ? tokens[50] : "N/A");
        }

        fprintf(out, "52) exit_code: %s - статус завершения потока в форме, возвращаемой waitpid()\n",
                token_count > 51 ? tokens[51] : "N/A");
    }

    fclose(f);
}

void print_cmdline(const char *pid, FILE *out)
{
    fprintf(out, "\nСтрока запуска процесса (Regular file /proc/<pid>/cmdline)n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/cmdline", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла cmdline: %m\n");
        return;
    }

    char buf[BUF_SIZE];
    size_t bytes = fread(buf, 1, BUF_SIZE - 1, f);

    if (bytes > 0)
    {
        buf[bytes] = '\0'; 

        for (size_t i = 0; i < bytes; i++)
        {
            if (buf[i] == '\0')
            {
                buf[i] = ' ';
            }
        }

        fprintf(out, "Командная строка: %s\n", buf);
    }
    else
    {
        fprintf(out, "Командная строка пуста (процесс может быть зомби)\n");
    }

    fclose(f);
}

void print_fd(const char *pid, FILE *out)
{
    fprintf(out, "\nОткрытые файловые дескрипторы (Directory /proc/<pid>/fd)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/fd", pid);

    DIR *dir = opendir(path);
    if (!dir)
    {
        fprintf(out, "Ошибка при открытии директории fd: %m\n");
        return;
    }

    struct dirent *entry;
    char link_path[PATH_MAX];
    char link_target[PATH_MAX];

    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        snprintf(link_path, PATH_MAX, "/proc/%s/fd/%s", pid, entry->d_name);
        ssize_t len = readlink(link_path, link_target, PATH_MAX - 1);

        if (len != -1)
        {
            link_target[len] = '\0';
            fprintf(out, "fd %s -> %s\n", entry->d_name, link_target);
        }
    }

    closedir(dir);
}

void print_symlink(const char *pid, const char *link_name, const char *description, FILE *out)
{
    fprintf(out, "\n %s (%s)\n\n", description, link_name);

    char link_path[PATH_MAX];
    snprintf(link_path, PATH_MAX, "/proc/%s/%s", pid, link_name);

    char target[PATH_MAX];
    ssize_t len = readlink(link_path, target, PATH_MAX - 1);

    if (len != -1)
    {
        target[len] = '\0';
        fprintf(out, "%s: %s\n", link_name, target);
    }
    else
    {
        fprintf(out, "Ошибка при чтении символической ссылки: %m\n");
    }
}

void print_maps(const char *pid, FILE *out)
{
    fprintf(out, "\nВиртуальное адресное пространство (/proc/<pid>/maps) ===\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/maps", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла maps: %m\n");
        return;
    }

    fprintf(out, "%-6s %-16s %-8s %-8s %-6s %-8s %s\n",
            "# pages", "Address (beg-end)", "Prem", "Offset", "Dev (major:minor)", "inode", "path");
    fprintf(out, "--------------------------------------------------------------------------------\n");

    char line[BUF_SIZE];
    uint64_t page_size = sysconf(_SC_PAGE_SIZE);
    uint64_t total_pages = 0;
    uint64_t total_size = 0;

    while (fgets(line, BUF_SIZE, f))
    {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }

        uint64_t start_addr, end_addr;
        char remaining[BUF_SIZE];

        if (sscanf(line, "%lx-%lx %[^\n]", &start_addr, &end_addr, remaining) == 3)
        {
            uint64_t pages = (end_addr - start_addr) / page_size;
            total_pages += pages;
            total_size += (end_addr - start_addr);

            fprintf(out, "%-6lu %lx-%lx %s\n", pages, start_addr, end_addr, remaining);
        }
        else
        {
            fprintf(out, "%s\n", line);
        }
    }

    fprintf(out, "--------------------------------------------------------------------------------\n");
    fprintf(out, "Total pages: %lu\n", total_pages);
    fprintf(out, "Total vsize: %lu\n", total_size);

    fclose(f);
}

void print_pagemap(const char *pid, FILE *out)
{
    fprintf(out, "%-16s %-16s %-10s %-10s %-10s %-10s %s %s\n",
            "addr", "pfn", "soft-dirty", "file/shared", "swapped", "present", "library", "path");

    char maps_path[PATH_MAX];
    snprintf(maps_path, PATH_MAX, "/proc/%s/maps", pid);
    FILE *maps = fopen(maps_path, "r");
    if (!maps)
    {
        fprintf(out, "Ошибка при открытии файла maps: %m\n");
        return;
    }

    char pagemap_path[PATH_MAX];
    snprintf(pagemap_path, PATH_MAX, "/proc/%s/pagemap", pid);
    int pm_fd = open(pagemap_path, O_RDONLY);
    if (pm_fd < 0)
    {
        fprintf(out, "Ошибка при открытии файла pagemap: %m\n");
        fclose(maps);
        return;
    }

    char line[BUF_SIZE];
    uint64_t start_addr, end_addr;
    int page_count = 0;

    while (fgets(line, BUF_SIZE, maps))
    {
        char perms[5], dev[8], pathname[PATH_MAX];
        pathname[0] = '\0';
        uint64_t offset, inode;

        int items = sscanf(line, "%lx-%lx %4s %lx %7s %lu %s",
                           &start_addr, &end_addr, perms, &offset, dev, &inode, pathname);

        if (items < 6)
        {
            continue;
        }

        page_count = (end_addr - start_addr) / sysconf(_SC_PAGE_SIZE);

        fprintf(out, "Pages: %d\n", page_count);

        uint64_t page_size = sysconf(_SC_PAGE_SIZE);

        for (uint64_t addr = start_addr; addr < end_addr; addr += page_size)
        {
            uint64_t index = addr / page_size * sizeof(uint64_t);
            uint64_t data;

            if (pread(pm_fd, &data, sizeof(data), index) != sizeof(data))
            {
                continue;
            }

            uint64_t pfn = data & (((uint64_t)1 << 55) - 1);
            uint64_t soft_dirty = (data >> 55) & 1;
            uint64_t file_shared = (data >> 61) & 1;
            uint64_t swapped = (data >> 62) & 1;
            uint64_t present = (data >> 63) & 1;

            fprintf(out, "%-16lx\t%-16lx\t%-10lu\t%-10lu\t%-10lu\t%-10lu\t%s\n",
                    addr, pfn, soft_dirty, file_shared, swapped, present, pathname);
        }
    }

    fclose(maps);
    close(pm_fd);
}

void print_io(const char *pid, FILE *out)
{
    fprintf(out, "\nСтатистика ввода вывода (Regular file /proc/<pid>/io)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/io", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла io: %m\n");
        return;
    }

    char line[BUF_SIZE];
    while (fgets(line, BUF_SIZE, f))
    {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }

        if (strncmp(line, "rchar:", 6) == 0)
        {
            fprintf(out, "%s - прочитанные символы\n", line);
        }
        else if (strncmp(line, "wchar:", 6) == 0)
        {
            fprintf(out, "%s - записанные символы\n", line);
        }
        else if (strncmp(line, "syscr:", 6) == 0)
        {
            fprintf(out, "%s - системные вызовы чтения\n", line);
        }
        else if (strncmp(line, "syscw:", 6) == 0)
        {
            fprintf(out, "%s - системные вызовы записи\n", line);
        }
        else if (strncmp(line, "read_bytes:", 11) == 0)
        {
            fprintf(out, "%s - прочитанные байты с хранилища\n", line);
        }
        else if (strncmp(line, "write_bytes:", 12) == 0)
        {
            fprintf(out, "%s - записанные байты на хранилище\n", line);
        }
        else if (strncmp(line, "cancelled_write_bytes:", 22) == 0)
        {
            fprintf(out, "%s - отмененные операции записи в байтах\n", line);
        }
        else
        {
            fprintf(out, "%s\n", line);
        }
    }

    fclose(f);
}

void print_comm(const char *pid, FILE *out)
{
    fprintf(out, "\nКоманда запуска (Regular file /proc/<pid>/comm)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/comm", pid);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(out, "Ошибка при открытии файла comm: %m\n");
        return;
    }

    char buf[BUF_SIZE];
    if (fgets(buf, BUF_SIZE, f))
    {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
        }

        fprintf(out, "Имя команды: %s\n", buf);
    }

    fclose(f);
}

void print_task(const char *pid, FILE *out)
{
    fprintf(out, "\n(/proc/<pid>/task)\n\n");

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%s/task", pid);

    DIR *dir = opendir(path);
    if (!dir)
    {
        fprintf(out, "Ошибка при открытии директории task: %m\n");
        return;
    }

    struct dirent *entry;
    int task_count = 0;

    fprintf(out, "ID потоков процесса %s:\n", pid);

    while ((entry = readdir(dir)))
    {
        if (entry->d_name[0] == '.')
        {
            continue; // Пропускаем . и ..
        }

        task_count++;

        char task_comm_path[PATH_MAX];
        snprintf(task_comm_path, PATH_MAX, "/proc/%s/task/%s/comm", pid, entry->d_name);

        FILE *comm_file = fopen(task_comm_path, "r");
        if (comm_file)
        {
            char comm[BUF_SIZE] = "";
            if (fgets(comm, BUF_SIZE, comm_file))
            {
                // Удаляем символ новой строки
                size_t len = strlen(comm);
                if (len > 0 && comm[len - 1] == '\n')
                {
                    comm[len - 1] = '\0';
                }

                fprintf(out, "  %s (tid: %s)\n", comm, entry->d_name);
            }
            else
            {
                fprintf(out, "  tid: %s\n", entry->d_name);
            }

            fclose(comm_file);
        }
        else
        {
            fprintf(out, "  tid: %s\n", entry->d_name);
        }
    }

    fprintf(out, "\nВсего потоков: %d\n", task_count);

    closedir(dir);
}

void print_mem(const char *pid, FILE *out)
{
    char mem_path[PATH_MAX], maps_path[PATH_MAX];
    snprintf(mem_path, sizeof(mem_path), "/proc/%s/mem", pid);
    snprintf(maps_path, sizeof(maps_path), "/proc/%s/maps", pid);

    int mem_fd = open(mem_path, O_RDONLY);
    FILE *maps = fopen(maps_path, "r");

    if (mem_fd < 0 || !maps)
    {
        fprintf(out, "Ошибка при открытии файлов: %m\n");
        if (mem_fd >= 0)
            close(mem_fd);
        if (maps)
            fclose(maps);
        return;
    }

    char line[256];
    uint64_t start, end;
    char perm[5];
    int i = 0;
    while (fgets(line, sizeof(line), maps))
    {
        i++;
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perm) != 3 || !(strstr(line, "[stack]") || i == 2 || i == 3))
            continue;

        if (perm[0] != 'r')
            continue;

        if (lseek(mem_fd, start, SEEK_SET) == -1)
        {
            fprintf(out, "Ошибка при смещении указателя в файле mem: %m\n");
            continue;
        }

        uint64_t size = end - start;
        unsigned char buffer[BUFSIZ];

        while (size > 0)
        {
            ssize_t bytes_read = read(mem_fd, buffer, (size > BUFSIZ) ? BUFSIZ : size);
            if (bytes_read <= 0)
                break;

            for (ssize_t i = 0; i < bytes_read; i++)
            {
                fprintf(out, "%.2x ", buffer[i]);
                if ((i + 1) % 16 == 0)
                    fprintf(out, "\n");
            }

            size -= bytes_read;
        }
        fprintf(out, "\n");
    }

    close(mem_fd);
    fclose(maps);
}

int main(int argc, char *argv[])
{
    char pid[16] = "self";
    const char *base_name = "proc";  // фиксированный префикс

    if (argc > 1) {
        strncpy(pid, argv[1], sizeof(pid) - 1);
        pid[sizeof(pid) - 1] = '\0';
    }

    char pathbuf[PATH_MAX];
    FILE *f;

    /* common.txt: comm, cmdline, cwd/exe/root */
    f = open_section(base_name, "common", pathbuf, sizeof(pathbuf));
    if (f) {
        fprintf(f, "Информация о PID=%s:\n", pid);
        print_comm(pid, f);
        print_cmdline(pid, f);
        print_symlink(pid, "cwd", "Текущая рабочая директория", f);
        print_symlink(pid, "exe", "Исполняемый файл", f);
        print_symlink(pid, "root", "Корневая директория", f);
        fclose(f);
    }

    /* environ.txt */
    f = open_section(base_name, "environ", pathbuf, sizeof(pathbuf));
    if (f) { print_environ(pid, f); fclose(f); }

    /* maps.txt */
    f = open_section(base_name, "maps", pathbuf, sizeof(pathbuf));
    if (f) { print_maps(pid, f); fclose(f); }

    /* mem.txt */
    f = open_section(base_name, "mem", pathbuf, sizeof(pathbuf));
    if (f) { print_mem(pid, f); fclose(f); }

    /* pagemap.txt */
    f = open_section(base_name, "pagemap", pathbuf, sizeof(pathbuf));
    if (f) { print_pagemap(pid, f); fclose(f); }

    /* stat.txt */
    f = open_section(base_name, "stat", pathbuf, sizeof(pathbuf));
    if (f) { print_stat(pid, f); fclose(f); }

    /* all.txt: fd, io, task */
    f = open_section(base_name, "all", pathbuf, sizeof(pathbuf));
    if (f) {
        fprintf(f, "Информация о PID=%s — прочие разделы\n", pid);
        print_fd(pid, f);
        print_io(pid, f);
        print_task(pid, f);
        fclose(f);
    }

    printf("Готово. Файлы: %s_common.txt, %s_environ.txt, %s_maps.txt, %s_mem.txt, %s_pagemap.txt, %s_stat.txt, %s_all.txt\n",
           base_name, base_name, base_name, base_name, base_name, base_name, base_name);
    return 0;
}