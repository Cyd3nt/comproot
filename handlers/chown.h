#pragma once
/* SPDX-License-Identifier: NCSA */
#include "../comproot.h"

int record_chown(char *pathname, uid_t owner, gid_t group, int follow);

void handle_chown(HANDLER_ARGS);
void handle_lchown(HANDLER_ARGS);
void handle_fchown(HANDLER_ARGS);
void handle_fchownat(HANDLER_ARGS);
