#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h>  /* PATH_MAX */
#include <sys/uio.h> /* struct iovec */
#include <unistd.h>  /* pid_t */

#include "comproot.h"

int get_fd_path(pid_t pid, int fd, char procpath[PATH_MAX]);
int chdir_to_fd(pid_t pid, int fd, char procpath[PATH_MAX]);

#define A_PROC_VM_BUFS const struct iovec liov[], unsigned long nl, const struct iovec riov[], unsigned long nr
int tx_data(A_HANDLER_FD, A_HANDLER_REQ, A_PROC_VM_BUFS, int push);

int check_pathname(char pathname[PATH_MAX]);
int pull_pathname(A_HANDLER_FD, A_HANDLER_REQ, int argno, char pathname[PATH_MAX]);
void set_cloexec(int fd);
