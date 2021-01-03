#pragma once
/* SPDX-License-Identifier: NCSA */
#include <err.h>
#include <unistd.h>

#include <seccomp.h>

struct comproot {
	int unknown_is_real;
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

#define DBG(fmt, ...) _LOGFN_W(comproot.verbose >= 2, warn, fmt, ##__VA_ARGS__)
#define DBGX(fmt, ...) _LOGFN_W(comproot.verbose >= 2, warnx, fmt, ##__VA_ARGS__)

#define PDBG(pid, fmt, ...) _LOGFN_W(comproot.verbose >= 2, warn, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PDBGX(pid, fmt, ...) _LOGFN_W(comproot.verbose >= 2, warnx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define A_HANDLER_FD int HANDLER_FD
#define A_HANDLER_REQ struct seccomp_notif *HANDLER_REQ
#define A_HANDLER_RESP struct seccomp_notif_resp *HANDLER_RESP
#define HANDLER_ARGS A_HANDLER_FD, A_HANDLER_REQ, A_HANDLER_RESP
#define PASS_HANDLER_ARGS HANDLER_FD, HANDLER_REQ, HANDLER_RESP
typedef void (*handler_func)(HANDLER_ARGS);
#define DECL_HANDLER(syscall_name) void handle_##syscall_name(HANDLER_ARGS)

#define HANDLER_ID HANDLER_REQ->id
#define HANDLER_PID HANDLER_REQ->pid
#define HANDLER_ARG(i) HANDLER_REQ->data.args[i]
#define HANDLER_END \
	do { \
	HANDLER_RESP->val = rc; \
	HANDLER_RESP->error = rc ? -errno : 0; \
	} while(0)
