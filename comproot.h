#pragma once
/* SPDX-License-Identifier: NCSA */
#include <err.h>

#include <seccomp.h>

extern int verbose;
#define _LOGFN_E(eval, fn, fmt, ...) fn(eval, fmt, ##__VA_ARGS__);
#define _LOGFN_W(cond, fn, fmt, ...) \
	do { \
	if (cond) fn(fmt, ##__VA_ARGS__); \
	} while(0)

#define ERR err
#define ERRX errx

#define PERR(pid, eval, fmt, ...) _LOGFN_E(eval, err, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PERRX(pid, eval, fmt, ...) _LOGFN_E(eval, errx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define WARN(fmt, ...) _LOGFN_W(verbose, warn, fmt, ##__VA_ARGS__)
#define WARNX(fmt, ...) _LOGFN_W(verbose, warnx, fmt, ##__VA_ARGS__)

#define PWARN(pid, fmt, ...) _LOGFN_W(verbose, warn, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PWARNX(pid, fmt, ...) _LOGFN_W(verbose, warnx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define DBG(fmt, ...) _LOGFN_W(verbose > 1, warn, fmt, ##__VA_ARGS__)
#define DBGX(fmt, ...) _LOGFN_W(verbose > 1, warnx, fmt, ##__VA_ARGS__)

#define PDBG(pid, fmt, ...) _LOGFN_W(verbose > 1, warn, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define PDBGX(pid, fmt, ...) _LOGFN_W(verbose > 1, warnx, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define HANDLER_ARGS int notifyfd, struct seccomp_notif *req, struct seccomp_notif_resp *resp
#define PASS_HANDLER_ARGS notifyfd, req, resp
typedef void (*handler_func)(HANDLER_ARGS);
