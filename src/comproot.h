#pragma once
/* SPDX-License-Identifier: NCSA */
#include <err.h>
#include <unistd.h>

#include <seccomp.h>

struct comproot {
	int verbose;
	uid_t uid;
	gid_t gid;
};
extern struct comproot comproot;

#define _LOGFN_E(eval, fn, fmt, ...) fn(eval, fmt, ##__VA_ARGS__);
#define _LOGFN_W(cond, fn, fmt, ...) \
	do { \
	if (cond) fn(fmt, ##__VA_ARGS__); \
	} while(0)

#define ERR err
#define ERRX errx

#define PERR(pid, eval, fmt, ...) _LOGFN_E(eval, err, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PERRX(pid, eval, fmt, ...) _LOGFN_E(eval, errx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define WARN(fmt, ...) _LOGFN_W(comproot.verbose, warn, fmt, ##__VA_ARGS__)
#define WARNX(fmt, ...) _LOGFN_W(comproot.verbose, warnx, fmt, ##__VA_ARGS__)

#define PWARN(pid, fmt, ...) _LOGFN_W(comproot.verbose, warn, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PWARNX(pid, fmt, ...) _LOGFN_W(comproot.verbose, warnx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define DBG(fmt, ...) _LOGFN_W(comproot.verbose > 1, warn, fmt, ##__VA_ARGS__)
#define DBGX(fmt, ...) _LOGFN_W(comproot.verbose > 1, warnx, fmt, ##__VA_ARGS__)

#define PDBG(pid, fmt, ...) _LOGFN_W(comproot.verbose > 1, warn, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PDBGX(pid, fmt, ...) _LOGFN_W(comproot.verbose > 1, warnx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define HANDLER_ARGS int notifyfd, struct seccomp_notif *req, struct seccomp_notif_resp *resp
#define PASS_HANDLER_ARGS notifyfd, req, resp
typedef void (*handler_func)(HANDLER_ARGS);
#define DECL_HANDLER(syscall_name) void handle_##syscall_name(HANDLER_ARGS)
