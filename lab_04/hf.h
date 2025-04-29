#ifndef PROC_INFO_H
#define PROC_INFO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>

#define PATH_LEN 1000
#define BUF_SIZE 1024

void print_page(uint64_t address, uint64_t data, FILE *out);
int get_pagemap_info(const char *proc, FILE *out);
void get_maps_info(const char *proc, FILE *out);
void get_stat_info(const char *proc, FILE *out);
void get_task_info(const char *proc, FILE *out);
void get_cmdline_info(const char *proc, FILE *out);
void get_cwd_info(const char *proc, FILE *out);
void get_environ_info(const char *proc, FILE *out);
void get_exe_info(const char *proc, FILE *out);
void get_fd_info(const char *proc, FILE *out);
void get_mem_info(int pid, FILE *out);
void get_mem_info_2(const char *proc, FILE *out);
void get_root_info(const char *proc, FILE *out);

#endif // PROC_INFO_H