/* SPDX-License-Identifier: NCSA */
#include <poll.h>    /* ppoll */
#include <signal.h>  /* sigaddset, SIGCHLD, sigemptyset */
#include <sys/signalfd.h> /* SFD_CLOEXEC, signalfd */
#include <unistd.h>  /* execvp, fork */

#include "chown.h"
#include "comproot.h"
#include "file.h"

#define add_handler(syscall_name) \
  [SCMP_SYS(syscall_name)] = handle_##syscall_name,

handler_func handlers[] = {
  add_handler(chown)
  add_handler(fchownat)
  add_handler(lchown)
};
#undef add_handler

void new_notification(short revents, int notifyfd) {
  struct seccomp_notif *req;
  struct seccomp_notif_resp *resp;
  handler_func handler;

  if (!(revents & POLLIN))
    errx(2, "notifyfd has status %d", revents);

  if (seccomp_notify_alloc(&req, &resp))
    err(3, "seccomp_notify_alloc");
  if (seccomp_notify_receive(notifyfd, req))
    err(3, "seccomp_notify_receive");

  handler = handlers[req->data.nr];
  if (!handler)
    errx(3, "received syscall %d with no handler", req->data.nr);

  handler(notifyfd, req, resp);
  if (seccomp_notify_respond(notifyfd, resp))
    err(3, "seccomp_notify_respond");

  seccomp_notify_free(req, resp);
}

int new_signal(short revents, int sfd, int want_pid) {
  struct signalfd_siginfo si;
  ssize_t rc;

  if (!(revents & POLLIN))
    errx(2, "sfd has status %d", revents);

  rc = read(sfd, &si, sizeof(si));
  if (rc < 0)
    err(2, "read(sfd)");
  if (rc < (ssize_t)sizeof(si))
    errx(2, "read(sfd) came up short");

  if ((int)si.ssi_pid == want_pid)
    return si.ssi_status;
  errx(3, "pid %jd isn't my child", (intmax_t)si.ssi_pid);
}

int main(int argc, char *argv[]) {
  scmp_filter_ctx sctx = 0;
  int notifyfd = -1, sfd = -1, rc = 1;
  sigset_t smask;
  pid_t child;

  if (argc < 2)
    errx(1, "Usage: %s PROGRAM [ARGUMENTS...]", argv[0]);

  if (seccomp_api_get() < 5)
    errx(2, "SCMP_ACT_NOTIFY is unsupported on this kernel");

  if (!(sctx = seccomp_init(SCMP_ACT_ALLOW)))
    err(3, "seccomp_init");
  if (seccomp_attr_set(sctx, SCMP_FLTATR_CTL_LOG, 1))
    err(3, "seccomp_attr_set(SCMP_FLTATR_CTL_LOG)");

#define add_rule(syscall_name) \
    if (seccomp_rule_add(sctx, SCMP_ACT_NOTIFY, SCMP_SYS(syscall_name), 0)) \
      err(3, "seccomp_rule_add(%s)", #syscall_name);

  add_rule(chown)
  add_rule(fchownat)
  add_rule(lchown)
#undef add_rule

  if (seccomp_load(sctx))
    err(3, "seccomp_load");
  notifyfd = seccomp_notify_fd(sctx);

  sigemptyset(&smask);
  sigaddset(&smask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &smask, 0) < 0)
    err(2, "sigprocmask");
  if ((sfd = signalfd(-1, &smask, SFD_CLOEXEC)) < 0)
    err(2, "signalfd");

  argv++;
  child = fork();
  if (child == 0) {
    if (execvp(argv[0], argv))
      err(2, "execvp");
    /* not reached */
    return 255;
  } else if (child < 0)
    err(2, "fork");
  /* else parent ... */

  struct pollfd fds[2] = {{notifyfd, POLLIN, 0}, {sfd, POLLIN, 0}};
  while (1) {
    rc = ppoll(fds, sizeof(fds), 0, &smask);
    if (rc < 0)
      err(2, "ppoll");
    if (fds[0].revents)
      new_notification(fds[0].revents, notifyfd);
    if (fds[1].revents) {
      rc = new_signal(fds[1].revents, sfd, child);
      break;
    }
  }

  file_walk(dump_files);
  seccomp_release(sctx);
  return rc;
}
