/* SPDX-License-Identifier: NCSA */
#include <errno.h>  /* errno */
#include <fcntl.h>  /* AT_* */
#include <stdio.h>  /* snprintf */
#include <unistd.h> /* *id_t */

#include "../comproot.h"
#include "../file.h"
#include "../util.h"

static int record_chown(char *pathname, uid_t owner, gid_t group, int follow) {
	char *fullpath;

	if (follow) {
		if (!(fullpath = realpath(pathname, 0))) {
			WARN("realpath");
			return -1;
		}
	} else
		fullpath = pathname;

	struct file *f = file_upsert_path(fullpath, follow);
	if (!f)
		return -1;
	if (owner != (uid_t)-1)
		f->st_uid = owner;
	if (group != (gid_t)-1)
		f->st_gid = group;

	return 0;
}

static void handle_chown_inner(char *syscall_name, HANDLER_ARGS, int follow) {
	char pathname[PATH_MAX] = {0};
	uid_t owner = HANDLER_ARG(1);
	gid_t group = HANDLER_ARG(2);

	int rc = -1;

	if (pull_pathname(HANDLER_FD, HANDLER_REQ, 0, pathname) == -1)
		goto out;
	PDBGX(HANDLER_PID, "%s(\"%s\", %d, %d)", syscall_name, pathname, owner, group);

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(HANDLER_PID, AT_FDCWD, procpath))
			goto out;
	}

	rc = record_chown(pathname, owner, group, follow);
out:
	HANDLER_END(rc);
}

DECL_HANDLER(chown) {
	handle_chown_inner("chown", PASS_HANDLER_ARGS, 1);
}

DECL_HANDLER(lchown) {
	handle_chown_inner("lchown", PASS_HANDLER_ARGS, 0);
}

DECL_HANDLER(fchown) {
	int fd = HANDLER_ARG(0);
	uid_t owner = HANDLER_ARG(1);
	gid_t group = HANDLER_ARG(2);

	(void)HANDLER_FD; /* unused */
	int rc = -1;
	char procpath[PATH_MAX];

	PDBGX(HANDLER_PID, "fchown(%d, %d, %d)", fd, owner, group);

	if (get_fd_path(HANDLER_PID, fd, procpath) == -1)
		goto out;

	rc = record_chown(procpath, owner, group, 0);
out:
	HANDLER_END(rc);
}

DECL_HANDLER(fchownat) {
	int fd = HANDLER_ARG(0);
	char pathname[PATH_MAX] = {0}; int pathname_l = -1;
	uid_t owner = HANDLER_ARG(2);
	gid_t group = HANDLER_ARG(3);
	int flags = HANDLER_ARG(4);

	int rc = -1;

	pathname_l = pull_pathname(HANDLER_FD, HANDLER_REQ, 1, pathname);
	if (pathname_l == -1)
		goto out;

	if (fd == AT_FDCWD)
		PDBGX(HANDLER_PID, "fchownat(AT_FDCWD, \"%s\", %d, %d, %d)", pathname, owner, group, flags);
	else
		PDBGX(HANDLER_PID, "fchownat(%d, \"%s\", %d, %d, %d)", fd, pathname, owner, group, flags);

	if ((flags & (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) == (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) {
		PWARNX(HANDLER_PID, "not implemented: AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW");
		errno = ENOSYS;
		goto out;
	}

	char procpath[PATH_MAX] = {0};
	if (chdir_to_fd(HANDLER_PID, fd, procpath))
		goto out;

	char *fullpath = 0;
	if (pathname_l == 0 && (flags & AT_EMPTY_PATH))
		fullpath = procpath;
	else
		fullpath = pathname;

	rc = record_chown(fullpath, owner, group, !(flags & AT_SYMLINK_NOFOLLOW));
out:
	HANDLER_END(rc);
}
