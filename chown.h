#pragma once
/* SPDX-License-Identifier: NCSA */
#include "comproot.h"

int handle_chown_inner(char *pathname, uid_t owner, gid_t group, int follow);

void handle_chown(handler_args);
void handle_lchown(handler_args);
void handle_fchown(handler_args);
void handle_fchownat(handler_args);
