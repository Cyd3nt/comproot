#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h>  /* PATH_MAX */
#include <sys/uio.h> /* struct iovec */
#include <unistd.h>  /* pid_t */

#include "comproot.h"

int get_fd_path(int fd, char procpath[PATH_MAX]);
int chdir_to_fd(int fd, char procpath[PATH_MAX]);

int tx_data(A_HANDLER_PROC, void *buf, uint64_t addr, size_t len, int push);

int check_pathname(char pathname[PATH_MAX]);
int pull_pathname(A_HANDLER_PROC, A_HANDLER_REQ, int argno, char pathname[PATH_MAX]);
void set_cloexec(int fd);
