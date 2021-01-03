/* SPDX-License-Identifier: NCSA */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../comproot.h"
#include "../file.h"
#include "../util.h"

static void handle_stat_inner(char *syscall_name, HANDLER_ARGS, int follow) {
	char pathname[PATH_MAX] = {0};
	struct stat statbuf;

	int rc = -1;

	if (pull_pathname(notifyfd, req, 0, pathname) == -1)
		goto out;

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(req->pid, AT_FDCWD, procpath))
			goto out;
	}

	rc = stat_upsert_path(&statbuf, pathname, follow);
	if (rc == -1)
		goto out;

	PDBGX(req->pid, "%s(\"%s\", {st_uid = %jd, st_gid = %jd})", syscall_name, pathname, (intmax_t)statbuf.st_uid, (intmax_t)statbuf.st_gid);

	struct iovec liov[] = {{&statbuf, sizeof(statbuf)}};
	struct iovec riov[] = {{(void *)req->data.args[1], sizeof(statbuf)}};
	if ((rc = tx_data(notifyfd, req, liov, 1, riov, 1, 1)) == -1)
		goto out;

	rc = 0;
out:
	resp->val = rc;
	resp->error = rc ? -errno : 0;
}

DECL_HANDLER(stat) {
	handle_stat_inner("stat", PASS_HANDLER_ARGS, 1);
}

DECL_HANDLER(lstat) {
	handle_stat_inner("lstat", PASS_HANDLER_ARGS, 0);
}

DECL_HANDLER(fstat) {
	int fd = req->data.args[0];
	struct stat statbuf;

	int rc = -1;

	rc = stat_upsert_fd(&statbuf, req->pid, fd);
	if (rc == -1)
		goto out;

	PDBGX(req->pid, "fstat(%d, {st_uid = %jd, st_gid = %jd})", fd, (intmax_t)statbuf.st_uid, statbuf.st_gid);

	struct iovec liov[] = {{&statbuf, sizeof(statbuf)}};
	struct iovec riov[] = {{(void *)req->data.args[1], sizeof(statbuf)}};

	if ((rc = tx_data(notifyfd, req, liov, 1, riov, 1, 1)) == -1)
		goto out;

	rc = 0;
out:
	resp->val = rc;
	resp->error = rc ? -errno : 0;
}
