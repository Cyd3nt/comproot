#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h>   /* PATH_MAX */
#include <search.h>   /* tdelete, tfind, tsearch, twalk */
#include <stdlib.h>   /* free */
#include <sys/stat.h> /* struct stat */
#include <unistd.h>   /* pid_t */

#include <seccomp.h>

struct file {
	dev_t st_dev;
	ino_t st_ino;
	/* nlink_t st_nlink; */
	/* mode_t st_mode; */
	uid_t st_uid;
	gid_t st_gid;
	/* dev_t st_rdev; */
};

struct file *file_get(dev_t dev, ino_t ino);

int stat_upsert_path(struct stat *st, char *path, int follow);
int stat_upsert_fd(struct stat *st, pid_t pid, int fd);

struct file *file_upsert_path(char *path, int follow);

void file_walk(void (*action)(const void *, VISIT, int));
void dump_files(const void *node, VISIT visit, int level);
