/* SPDX-License-Identifier: NCSA */
#include <errno.h>   /* errno */
#include <fcntl.h>   /* AT_*, FD_*, fcntl */
#include <stdio.h>   /* snprintf */
#include <string.h>  /* strnlen */

#include "comproot.h"
#include "util.h"

int get_fd_path(int fd, char procpath[PATH_MAX]) {
	int rc = -1;

	errno = 0;
	if (fd == AT_FDCWD) {
		strcpy(procpath, "cwd");
		rc = 0;
	} else
		rc = snprintf(procpath, PATH_MAX, "fd/%d", fd);
	if (rc < 0 || rc >= PATH_MAX) {
		WARN("snprintf(\"fd/%%d\") = %d", rc);
		errno = EIO;
		return -1;
	}

	return 0;
}

int chdir_to_fd(int fd, char procpath[PATH_MAX]) {
	if (get_fd_path(fd, procpath) == -1)
		return -1;

	if (chdir(procpath) == -1) {
		WARN("chdir(procpath)");
		return -1;
	}

	return 0;
}

int tx_data(A_HANDLER_PROC, void *buf, uint64_t addr, size_t len, int push) {
	int rc = -1;
	int procfd = openat(HANDLER_PROC, "mem", O_RDWR|O_CLOEXEC);
	if (procfd == -1) {
		WARNX("process died during trace!");
		errno = EIO;
		goto out;
	}

	if (push)
		rc = pwrite(procfd, buf, len, addr);
	else
		rc = pread(procfd, buf, len, addr);
	close(procfd);
out:
	return rc;
}

int check_pathname(char pathname[PATH_MAX]) {
	int rc;

	if ((rc = strnlen(pathname, PATH_MAX+1)) == PATH_MAX+1) {
		WARNX("pathname is bigger than PATH_MAX");
		errno = ENAMETOOLONG;
		return -1;
	}
	return rc;
}

int pull_pathname(A_HANDLER_PROC, A_HANDLER_REQ, int argno, char pathname[PATH_MAX]) {
	int rc = -1;

	if ((rc = tx_data(HANDLER_PROC, pathname, HANDLER_ARG(argno), PATH_MAX+1, 0)) == -1)
		goto out;

	rc = check_pathname(pathname);
out:
	return rc;
}

void set_cloexec(int fd) {
	int flags;

	if ((flags = fcntl(fd, F_GETFD)) == -1)
		ERR(2, "fcntl(F_GETFD)");
	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
		ERR(2, "fcntl(F_SETFD, FD_CLOEXEC)");
}
