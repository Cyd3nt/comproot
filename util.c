/* SPDX-License-Identifier: NCSA */
#include <errno.h>   /* errno */
#include <fcntl.h>   /* AT_* */
#include <stdio.h>   /* snprintf */
#include <string.h>  /* strnlen */

#include "comproot.h"
#include "util.h"

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

int tx_data(int notifyfd, struct seccomp_notif *req, PROC_VM_BUFS, int push) {
	int rc;
	ssize_t (*func)(pid_t pid, PROC_VM_BUFS, unsigned long flags) = 0;

	if (push)
		func = process_vm_writev;
	else
		func = process_vm_readv;

	if ((rc = func(req->pid, liov, nl, riov, nr, 0)) == -1) {
#ifndef NDEBUG
		pwarn(req->pid, "tx_data");
#endif
		errno = EIO;
		goto out;
	}
	if (seccomp_notify_id_valid(notifyfd, req->id)) {
#ifndef NDEBUG
		pwarnx(req->pid, "died during trace!");
#endif
		rc = -1;
		errno = EIO;
		goto out;
	}

out:
	return rc;
}

int check_pathname(char pathname[PATH_MAX]) {
	int rc;

	if ((rc = strnlen(pathname, PATH_MAX+1)) == PATH_MAX+1) {
#ifndef NDEBUG
		warnx("pathname is bigger than PATH_MAX");
#endif
		errno = ENAMETOOLONG;
		return -1;
	}
	return rc;
}

int pull_pathname(int notifyfd, struct seccomp_notif *req, int argno, char pathname[PATH_MAX]) {
	int rc;
	struct iovec liov[] = {{pathname, PATH_MAX+1}};
	struct iovec riov[] = {{(void *)req->data.args[argno], PATH_MAX+1}};

	if ((rc = tx_data(notifyfd, req, liov, 1, riov, 1, 0)) == -1)
		goto out;

	rc = check_pathname(pathname);
out:
	return rc;
}
