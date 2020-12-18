/* SPDX-License-Identifier: NCSA */
#include <errno.h>  /* errno */
#include <fcntl.h>  /* AT_* */
#include <stdio.h>  /* snprintf */
#include <unistd.h> /* *id_t */

#include "../comproot.h"
#include "../file.h"
#include "../util.h"

static int record_chown(char *pathname, uid_t owner, gid_t group, int follow) {
	int rc = -1;
	char *fullpath;

	if (follow) {
		if (!(fullpath = realpath(pathname, 0))) {
			WARN("realpath");
			goto out;
		}
	} else
		fullpath = pathname;

	struct file *f = file_upsert_path(fullpath, follow);
	if (!f)
		goto out;
	if (owner != (uid_t)-1)
		f->st_uid = owner;
	if (group != (gid_t)-1)
		f->st_gid = group;

	rc = 0;
out:
	return rc;
}

static void handle_chown_inner(char *syscall_name, HANDLER_ARGS, int follow) {
	char pathname[PATH_MAX] = {0};
	uid_t owner = req->data.args[1];
	gid_t group = req->data.args[2];

	int rc = -1;

	if (pull_pathname(notifyfd, req, 0, pathname) == -1)
		goto out;
	PDBGX(req->pid, "%s(\"%s\", %d, %d)", syscall_name, pathname, owner, group);

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(req->pid, AT_FDCWD, procpath))
			goto out;
	}

	rc = record_chown(pathname, owner, group, follow);
out:
	resp->val = rc;
	resp->error = rc ? -errno : 0;
}

DECL_HANDLER(chown) {
	handle_chown_inner("chown", PASS_HANDLER_ARGS, 1);
}

DECL_HANDLER(lchown) {
	handle_chown_inner("lchown", PASS_HANDLER_ARGS, 0);
}

DECL_HANDLER(fchown) {
	int fd = req->data.args[0];
	uid_t owner = req->data.args[1];
	gid_t group = req->data.args[2];

	(void)notifyfd; /* unused */
	int rc = -1;
	char procpath[PATH_MAX];

	PDBGX(req->pid, "fchown(%d, %d, %d)", fd, owner, group);

	rc = snprintf(procpath, PATH_MAX, "/proc/%jd/fd/%d", (intmax_t)req->pid, fd);
	if (rc < 0 || rc >= PATH_MAX) {
		PWARN(req->pid, "snprintf(procpath) = %d", rc);
		errno = EIO;
		goto out;
	}

	rc = record_chown(procpath, owner, group, 0);
out:
	resp->val = rc;
	resp->error = rc ? -errno : 0;
}

DECL_HANDLER(fchownat) {
	int fd = req->data.args[0];
	char pathname[PATH_MAX] = {0}; int pathname_l = -1;
	uid_t owner = req->data.args[2];
	gid_t group = req->data.args[3];
	int flags = req->data.args[4];

	int rc = -1;

	pathname_l = pull_pathname(notifyfd, req, 1, pathname);
	if (pathname_l == -1)
		goto out;

	if (fd == AT_FDCWD)
		PDBGX(req->pid, "fchownat(AT_FDCWD, \"%s\", %d, %d, %d)", pathname, owner, group, flags);
	else
		PDBGX(req->pid, "fchownat(%d, \"%s\", %d, %d, %d)", fd, pathname, owner, group, flags);

	if ((flags & (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) == (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) {
		PWARNX(req->pid, "not implemented: AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW");
		errno = ENOSYS;
		goto out;
	}

	char procpath[PATH_MAX] = {0};
	if (chdir_to_fd(req->pid, fd, procpath))
		goto out;

	char *fullpath = 0;
	if (pathname_l == 0 && (flags & AT_EMPTY_PATH))
		fullpath = procpath;
	else
		fullpath = pathname;

	rc = record_chown(fullpath, owner, group, !(flags & AT_SYMLINK_NOFOLLOW));
out:
	resp->val = rc;
	resp->error = rc ? -errno : 0;
}
