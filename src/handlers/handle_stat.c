/* SPDX-License-Identifier: NCSA */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../comproot.h"
#include "../file.h"
#include "../util.h"

#define STAT_FMT "{st_uid = %jd, st_gid = %jd}"
#define STAT_ARG(st) (intmax_t)st.st_uid, (intmax_t)st.st_gid

static void handle_stat_inner(char *syscall_name, HANDLER_ARGS, int follow) {
	char pathname[PATH_MAX] = {0};
	struct stat statbuf;

	int rc = -1;

	if (pull_pathname(HANDLER_PROC, HANDLER_REQ, 0, pathname) == -1)
		goto out;

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(AT_FDCWD, procpath))
			goto out;
	}

	if (stat_upsert_path(&statbuf, pathname, follow) == -1)
		goto out;

	PDBGX(HANDLER_PID, "%s(\"%s\", "STAT_FMT")", syscall_name, pathname, STAT_ARG(statbuf));

	if (tx_data(HANDLER_PROC, &statbuf, HANDLER_ARG(1), sizeof(statbuf), 1) == -1)
		goto out;

	rc = 0;
out:
	HANDLER_END(rc);
}

DECL_HANDLER(stat) {
	handle_stat_inner("stat", PASS_HANDLER_ARGS, 1);
}

DECL_HANDLER(lstat) {
	handle_stat_inner("lstat", PASS_HANDLER_ARGS, 0);
}

DECL_HANDLER(fstat) {
	int fd = HANDLER_ARG(0);
	struct stat statbuf;

	int rc = -1;

	if (stat_upsert_fd(&statbuf, fd) == -1)
		goto out;

	PDBGX(HANDLER_PID, "fstat(%d, "STAT_FMT")", fd, STAT_ARG(statbuf));

	if (tx_data(HANDLER_PROC, &statbuf, HANDLER_ARG(1), sizeof(statbuf), 1) == -1)
		goto out;

	rc = 0;
out:
	HANDLER_END(rc);
}
