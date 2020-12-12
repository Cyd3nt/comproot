#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h> /* PATH_MAX */

#include "comproot.h"

int chdir_to_fd(pid_t pid, int fd, char *procpath);
int pull_pathname(int notifyfd, struct seccomp_notif *req, int argno, char pathname[PATH_MAX]);
