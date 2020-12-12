#pragma once
/* SPDX-License-Identifier: NCSA */
#include <limits.h>   /* PATH_MAX */
#include <search.h>   /* tdelete, tfind, tsearch, twalk */
#include <stdlib.h>   /* free */
#include <sys/stat.h> /* struct stat */

#include <seccomp.h>

struct file {
	struct stat stat;
};
extern struct file *files;

int file_cmp(const void *a, const void *b);
struct file *file_get(char *path, int follow, int update);
void dump_files(const void *node, VISIT visit, int level);
#define file_search(key) tsearch(key, (void **)&files, file_cmp)
#define file_find(key) tfind(key, (void *const *)&files, file_cmp)
#define file_delete(key) tdelete(key, (void **)&files, file_cmp)
#define file_walk(action) twalk(files, action)
#define file_free(file) do { \
	free(file); \
	file = 0; \
} while(0);
