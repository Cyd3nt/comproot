/* SPDX-License-Identifier: NCSA */
#include <stdio.h>   /* printf */

#include "comproot.h"
#include "file.h"

struct file *files = 0;

int file_cmp(const void *a, const void *b) {
  dev_t dev = ((struct file *)b)->stat.st_dev - ((struct file *)a)->stat.st_dev;
  if (dev)
    return dev;
  return ((struct file *)b)->stat.st_ino - ((struct file *)a)->stat.st_ino;
}

struct file *file_get(char *path, int follow, int update) {
  struct file *f = 0, *f2 = malloc(sizeof(struct file));
  if (!f2) {
#ifndef NDEBUG
    warn("malloc");
#endif
    goto out;
  }

  int rc;
  if (follow)
    rc = stat(path, &f2->stat);
  else
    rc = lstat(path, &f2->stat);
  if (rc) {
#ifndef NDEBUG
    warn("l?stat");
#endif
    goto out;
}

  if (update) {
    f = f2;
    file_delete(f);
    file_search(f);
  } else if ((f = file_find(f2))) {
    file_free(f2);
  } else {
    f = f2;
    file_search(f);
  }
out:
  return f;
}

void dump_files(const void *node, VISIT visit, int level) {
  (void)level; /* unused */

  if (!node || (visit != preorder && visit != leaf))
    return;
  const struct file *f = *(const struct file **)node;
  if (!f)
    return;

  printf("dev=%jd ino=%jd uid=%d gid=%d\n", (intmax_t)f->stat.st_dev, (intmax_t)f->stat.st_ino, f->stat.st_uid, f->stat.st_gid);
}
