/* SPDX-License-Identifier: NCSA */
#include <errno.h>   /* errno */
#include <fcntl.h>   /* AT_* */
#include <limits.h>  /* PATH_MAX */
#include <stdio.h>   /* snprintf */
#include <string.h>  /* strnlen */
#include <sys/uio.h> /* process_vm_readv */
#include <unistd.h>  /* chdir */

#include "comproot.h"

int chdir_to_fd(pid_t pid, int fd, char *procpath) {
  int rc;

  errno = 0;
  if (fd == AT_FDCWD)
    rc = snprintf(procpath, PATH_MAX, "/proc/%jd/cwd", (intmax_t)pid);
  else
    rc = snprintf(procpath, PATH_MAX, "/proc/%jd/fd/%d", (intmax_t)pid, fd);
  if (rc < 0 || rc >= PATH_MAX) {
#ifndef NDEBUG
    pwarn(pid, "snprintf(procpath) = %d", rc);
#endif
    errno = EIO;
    return -1;
  }

  if (chdir(procpath)) {
#ifndef NDEBUG
    pwarn(pid, "chdir(%s)", procpath);
#endif
    return -1;
  }

  return 0;
}

int pull_pathname(int notifyfd, struct seccomp_notif *req, int argno, char pathname[PATH_MAX]) {
  int rc;
  struct iovec liov[] = {{pathname, PATH_MAX+1}};
  struct iovec riov[] = {{(void *)req->data.args[argno], PATH_MAX+1}};

  if (process_vm_readv(req->pid, liov, 1, riov, 1, 0) < 0) {
#ifndef NDEBUG
    pwarn(req->pid, "process_vm_readv");
#endif
    errno = EIO;
    return -1;
  }
  if (seccomp_notify_id_valid(notifyfd, req->id)) {
#ifndef NDEBUG
    pwarnx(req->pid, "died during trace!");
#endif
    errno = EIO;
    return -1;
  }

  if ((rc = strnlen(pathname, PATH_MAX+1)) == PATH_MAX+1) {
#ifndef NDEBUG
    pwarnx(req->pid, "pathname is bigger than PATH_MAX");
    errno = ENAMETOOLONG;
    return -1;
#endif
  }
  return rc;
}
