#pragma once
/* SPDX-License-Identifier: NCSA */
#include <err.h>

#include <seccomp.h>

#define perr(pid, eval, fmt, ...) err(eval, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define perrx(pid, eval, fmt, ...) errx(eval, "[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define pwarn(pid, fmt, ...) warn("[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)
#define pwarnx(pid, fmt, ...) warnx("[%jd] "fmt, (intmax_t)pid, ##__VA_ARGS__)

#define HANDLER_ARGS int notifyfd, struct seccomp_notif *req, struct seccomp_notif_resp *resp
#define PASS_HANDLER_ARGS notifyfd, req, resp
typedef void (*handler_func)(HANDLER_ARGS);
