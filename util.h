#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h>  /* PATH_MAX */
#include <sys/uio.h> /* struct iovec */
#include <unistd.h>  /* pid_t */

#include "comproot.h"

int chdir_to_fd(pid_t pid, int fd, char *procpath);

#define PROC_VM_BUFS const struct iovec liov[], unsigned long nl, const struct iovec riov[], unsigned long nr
int tx_data(int notifyfd, struct seccomp_notif *req, PROC_VM_BUFS, int push);

int check_pathname(char pathname[PATH_MAX]);
int pull_pathname(int notifyfd, struct seccomp_notif *req, int argno, char pathname[PATH_MAX]);
