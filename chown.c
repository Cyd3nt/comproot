/* SPDX-License-Identifier: NCSA */
#include <errno.h>  /* errno */
#include <fcntl.h>  /* AT_* */
#include <stdio.h>  /* snprintf */
#include <unistd.h> /* *id_t */

#include "chown.h"
#include "comproot.h"
#include "file.h"
#include "util.h"

int handle_chown_inner(char *pathname, uid_t owner, gid_t group, int follow) {
	int rc = -1;
	char *fullpath;

	if (follow) {
		if (!(fullpath = realpath(pathname, 0))) {
#ifndef NDEBUG
			warn("realpath");
#endif
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

void handle_chown(handler_args) {
	char pathname[PATH_MAX] = {0};
	uid_t owner = req->data.args[1];
	gid_t group = req->data.args[2];

	int rc = -1;

	if (pull_pathname(notifyfd, req, 0, pathname) == -1)
		goto out;
#ifndef NDEBUG
	pwarnx(req->pid, "chown(\"%s\", %d, %d)", pathname, owner, group);
#endif

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(req->pid, AT_FDCWD, procpath))
			goto out;
	}

	rc = handle_chown_inner(pathname, owner, group, 1);
out:
	resp->id = req->id;
	resp->val = rc;
	resp->error = rc ? -errno : 0;
	resp->flags = 0;
}

void handle_lchown(handler_args) {
	char pathname[PATH_MAX] = {0};
	uid_t owner = req->data.args[1];
	gid_t group = req->data.args[2];

	int rc = -1;

	if (pull_pathname(notifyfd, req, 0, pathname) == -1)
		goto out;
#ifndef NDEBUG
	pwarnx(req->pid, "lchown(\"%s\", %d, %d)", pathname, owner, group);
#endif

	if (pathname[0] != '/') {
		char procpath[PATH_MAX];
		if (chdir_to_fd(req->pid, AT_FDCWD, procpath))
			goto out;
	}

	rc = handle_chown_inner(pathname, owner, group, 0);
out:
	resp->id = req->id;
	resp->val = rc;
	resp->error = rc ? -errno : 0;
	resp->flags = 0;
}

void handle_fchown(handler_args) {
	int fd = req->data.args[0];
	uid_t owner = req->data.args[1];
	gid_t group = req->data.args[2];

	(void)notifyfd; /* unused */
	int rc = -1;
	char procpath[PATH_MAX];

#ifndef NDEBUG
	pwarnx(req->pid, "fchown(%d, %d, %d)", fd, owner, group);
#endif

	rc = snprintf(procpath, PATH_MAX, "/proc/%jd/fd/%d", (intmax_t)req->pid, fd);
	if (rc < 0 || rc >= PATH_MAX) {
#ifndef NDEBUG
		pwarn(req->pid, "snprintf(procpath) = %d", rc);
#endif
		errno = EIO;
		goto out;
	}

	rc = handle_chown_inner(procpath, owner, group, 0);
out:
	resp->id = req->id;
	resp->val = rc;
	resp->error = rc ? -errno : 0;
	resp->flags = 0;
}

void handle_fchownat(handler_args) {
	int fd = req->data.args[0];
	char pathname[PATH_MAX] = {0}; int pathname_l = -1;
	uid_t owner = req->data.args[2];
	gid_t group = req->data.args[3];
	int flags = req->data.args[4];

	int rc = -1;

	pathname_l = pull_pathname(notifyfd, req, 1, pathname);
	if (pathname_l == -1)
		goto out;

#ifndef NDEBUG
	if (fd == AT_FDCWD)
		pwarnx(req->pid, "fchownat(AT_FDCWD, \"%s\", %d, %d, %d)", pathname, owner, group, flags);
	else
		pwarnx(req->pid, "fchownat(%d, \"%s\", %d, %d, %d)", fd, pathname, owner, group, flags);
#endif

	if ((flags & (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) == (AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW)) {
		pwarnx(req->pid, "not implemented: AT_EMPTY_PATH|AT_SYMLINK_NOFOLLOW");
		errno = ENOSYS;
		goto out;
	}

	char procpath[PATH_MAX] = {0};
	if (chdir_to_fd(req->pid, req->data.args[0], procpath))
		goto out;

	char *fullpath = 0;
	if (pathname_l == 0 && (flags & AT_EMPTY_PATH))
		fullpath = procpath;
	else
		fullpath = pathname;

	rc = handle_chown_inner(fullpath, owner, group, !(flags & AT_SYMLINK_NOFOLLOW));
out:
	resp->id = req->id;
	resp->val = rc;
	resp->error = rc ? -errno : 0;
	resp->flags = 0;
}
