#pragma once
/* SPDX-License-Identifier: NCSA */
#include "../comproot.h"

int record_chown(char *pathname, uid_t owner, gid_t group, int follow);
