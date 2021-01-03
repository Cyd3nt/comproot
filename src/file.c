/* SPDX-License-Identifier: NCSA */
#include <stdio.h>   /* printf */

#include "comproot.h"
#include "file.h"
#include "util.h"

#define FILEPTR(f) *(struct file **)f
void *files = 0;

static int file_cmp(const void *a, const void *b) {
	dev_t dev = ((struct file *)b)->st_dev - ((struct file *)a)->st_dev;
	if (dev)
		return dev;
	return ((struct file *)b)->st_ino - ((struct file *)a)->st_ino;
}

static void *file_find(struct file *f) {
	return tfind((void *)f, &files, file_cmp);
}

static void *file_search(struct file *f) {
	return tsearch((void *)f, &files, file_cmp);
}

struct file *file_get(dev_t dev, ino_t ino) {
	struct file *f = 0, f2;

	f2.st_dev = dev;
	f2.st_ino = ino;

	if ((f = file_find(&f2)))
		f = FILEPTR(f);
	return f;
}

static int upsert_path(struct stat *st, struct file **f, char *path, int follow) {
	int rc = -1;
	*f = 0;

	if (follow)
		rc = stat(path, st);
	else
		rc = lstat(path, st);
	if (rc == -1) {
		DBG("l?stat: %s", path);
		goto out;
	}

	if ((*f = file_get(st->st_dev, st->st_ino))) {
		st->st_uid = (*f)->st_uid;
		st->st_gid = (*f)->st_gid;
	} else {
		*f = malloc(sizeof(struct file));
		if (!*f) {
			WARN("malloc");
			rc = -1;
			goto out;
		}

		(*f)->st_dev = st->st_dev;
		(*f)->st_ino = st->st_ino;
		if (st->st_uid == my_uid)
			st->st_uid = 0;
		if (st->st_gid == my_gid)
			st->st_gid = 0;
		(*f)->st_uid = st->st_uid;
		(*f)->st_gid = st->st_gid;
		file_search(*f);
	}

	rc = 0;
out:
	return rc;
}

/* Fills *st with stat information on path, optionally following
 * terminal symlinks if follow is nonzero. Returns 0 on success, -1 on
 * error. The results returned are cached and may have been modified by
 * other syscall handlers.
 */
int stat_upsert_path(struct stat *st, char *path, int follow) {
	struct file *f;
	return upsert_path(st, &f, path, follow);
}

/* Same as the above but with a file descriptor. */
int stat_upsert_fd(struct stat *st, pid_t pid, int fd) {
	char procpath[PATH_MAX];

	if (get_fd_path(pid, fd, procpath) == -1)
		return -1;

	return stat_upsert_path(st, procpath, 0);
}

/* Returns a filled *file concerning path, optionally following terminal
 * symlinks if follow is nonzero. Returns a null pointer on error. The
 * results returned are cached and may have been modified by other
 * syscall handlers.
 */
struct file *file_upsert_path(char *path, int follow) {
	struct file *f;
	struct stat st;
	return upsert_path(&st, &f, path, follow) == -1 ? 0 : f;
}

void file_walk(void (*action)(const void *, VISIT, int)) {
	twalk(files, action);
}

void dump_files(const void *node, VISIT visit, int level) {
	(void)level; /* unused */

	if (!node || (visit != preorder && visit != leaf))
		return;
	const struct file *f = FILEPTR(node);
	if (!f)
		return;

	printf("dev=%jd,ino=%jd,uid=%d,gid=%d\n", (intmax_t)f->st_dev, (intmax_t)f->st_ino, f->st_uid, f->st_gid);
}
