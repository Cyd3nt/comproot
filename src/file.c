/* SPDX-License-Identifier: NCSA */
#include <stdio.h>   /* printf */

#include "comproot.h"
#include "file.h"

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

struct file *file_upsert_path(char *path, int follow) {
	struct file *f = 0;
	struct stat st;
	int rc;

	if (follow)
		rc = stat(path, &st);
	else
		rc = lstat(path, &st);
	if (rc) {
		WARN("l?stat");
		goto out;
	}

	if ((f = file_get(st.st_dev, st.st_ino))) {
		goto out;
	} else {
		f = malloc(sizeof(struct file));
		if (!f) {
			WARN("malloc");
			goto out;
		}

		f->st_dev = st.st_dev;
		f->st_ino = st.st_ino;
		f->st_uid = st.st_uid;
		f->st_gid = st.st_gid;
		file_search(f);
	}
out:
	return f;
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
