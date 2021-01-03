/* SPDX-License-Identifier: NCSA */
#include <errno.h>
#include <poll.h>   /* POLLIN, ppoll */
#include <signal.h> /* SIG_*, sigaddset, SIG*, sigemptyset, sigprocmask, signal */
#include <stdio.h>  /* snprintf */
#include <string.h> /* memcpy, memset */
#include <sys/prctl.h>    /* PR_SET_CHILD_SUBREAPER, prctl */
#include <sys/signalfd.h> /* SFD_CLOEXEC, signalfd */
#include <sys/socket.h>   /* AF_*, CMSG_*, SCM_*, SOCK_*, SOL_*, socketpair */
#include <unistd.h>       /* execvp, fork */

#include "comproot.h"
#include "file.h"
#include "handlers/decl_handlers.h"
#include "util.h"

#define ADVERTISEMENT "COMPROOT_STAGE2"

struct comproot comproot = {
	.verbose = 2,
	.uid = -1,
	.gid = -1,
};

static void new_notification(short revents, int notifyfd) {
	struct seccomp_notif *req;
	struct seccomp_notif_resp *resp;

	if (!(revents & POLLIN))
		ERRX(2, "notifyfd has status %d", revents);

	if (seccomp_notify_alloc(&req, &resp))
		ERR(3, "seccomp_notify_alloc");
	if (seccomp_notify_receive(notifyfd, req))
		ERR(3, "seccomp_notify_receive");

	resp->id = req->id;
	resp->flags = 0;

	switch(req->data.nr) {
#define X(syscall_name) \
	case SCMP_SYS(syscall_name): \
		handle_##syscall_name(notifyfd, req, resp); \
		break;
#include "handlers/handlers.h"
#undef X
	default:
		ERRX(3, "received syscall %d with no handler", req->data.nr);
		break;
	}

	if (seccomp_notify_respond(notifyfd, resp))
		ERR(3, "seccomp_notify_respond");

	seccomp_notify_free(req, resp);
}

static int new_signal(short revents, int sfd, int want_pid, int *status) {
	struct signalfd_siginfo si;
	ssize_t rc;

	if (!(revents & POLLIN))
		ERRX(2, "sfd has status %d", revents);

	rc = read(sfd, &si, sizeof(si));
	if (rc < 0)
		ERR(2, "read(sfd)");
	if (rc < (ssize_t)sizeof(si))
		ERRX(2, "read(sfd) came up short");

	if (si.ssi_signo != SIGCHLD)
		return 0;
	PDBGX(si.ssi_pid, "exit = %d", si.ssi_status);
	if ((int)si.ssi_pid == want_pid) {
		*status = si.ssi_status;
		return 1;
	}
	return 0;
}

static void tx_notifyfd(int sockfd, pid_t *child, int *notifyfd, int push) {
	char buf[sizeof(pid_t)];
	char cbuf[CMSG_SPACE(sizeof(int))];
	struct iovec iov;
	struct msghdr msg;
	struct cmsghdr *cmsg;

	memset(&msg, 0, sizeof(msg));
	memset(cbuf, 0, sizeof(cbuf));

	iov.iov_base = buf;
	iov.iov_len = sizeof(buf);

	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cbuf;
	msg.msg_controllen = sizeof(cbuf);
	msg.msg_flags = 0;

	if (push) {
		memcpy(iov.iov_base, child, iov.iov_len);

		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;

		memcpy(CMSG_DATA(cmsg), notifyfd, sizeof(int));

		if (sendmsg(sockfd, &msg, 0) == -1)
			ERR(2, "sendmsg(sockfd)");
	} else {
		if (recvmsg(sockfd, &msg, 0) == -1)
			ERR(2, "recvmsg(sockfd)");
		close(sockfd);

		*child = *(pid_t *)buf;
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
				memcpy(notifyfd, CMSG_DATA(cmsg), sizeof(int));
				return;
			}
		}
		ERRX(2, "no notifyfd received");
	}
}

static void advertise_socket(int sockfd) {
	char *s;
	int n;

	n = snprintf(0, 0, "%jd", (intmax_t)sockfd);
	if (n < 0)
		ERR(2, "snprintf(0)");
	s = malloc(n + 1);
	if (snprintf(s, n + 1, "%jd", (intmax_t)sockfd) < 0)
		ERR(2, "snprintf(s)");

	if (setenv(ADVERTISEMENT, s, 1) == -1)
		ERR(2, "setenv(%s)", ADVERTISEMENT);
	free(s);
}

static int stage2(char *sockfd_env, char *argv[]) {
	scmp_filter_ctx sctx = 0;
	int sockfd, notifyfd = -1;
	pid_t child;

	errno = 0;
	sockfd = (int) strtol(sockfd_env, 0, 10);
	if (errno)
		ERR(1, ADVERTISEMENT);
	if (unsetenv(ADVERTISEMENT) == -1)
		ERR(2, "unsetenv(%s)", ADVERTISEMENT);
	set_cloexec(sockfd);

	if (!(sctx = seccomp_init(SCMP_ACT_ALLOW)))
		ERR(3, "seccomp_init");
#define X(syscall_name) \
	if (seccomp_rule_add(sctx, SCMP_ACT_NOTIFY, SCMP_SYS(syscall_name), 0)) \
		ERR(3, "seccomp_rule_add(%s)", #syscall_name);
#include "handlers/handlers.h"
#undef X

	if (seccomp_load(sctx))
		ERR(3, "seccomp_load");
	notifyfd = seccomp_notify_fd(sctx);
	set_cloexec(notifyfd);
	seccomp_release(sctx);

	child = fork();
	if (child == 0) {
		if (execvp(argv[1], argv + 1) == -1)
			ERR(2, "execvp: %s", argv[1]);
		/* not reached */
		return 255;
	} else if (child == -1)
		ERR(2, "fork");
	/* else parent ... */

	tx_notifyfd(sockfd, &child, &notifyfd, 1);
	return 0;
}

static int stage1(char *argv[]) {
	int sockfds[2], sfd = -1, notifyfd = -1, rc = -1;
	pid_t child;
	sigset_t smask;

	if (seccomp_api_get() < 5)
		ERRX(2, "SCMP_ACT_NOTIFY is unsupported on this kernel");

	if (prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0, 0) == -1)
		ERR(2, "prctl(PR_SET_CHILD_SUBREAPER)");

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds) == -1)
		ERR(2, "socketpair");
	advertise_socket(sockfds[0]);
	set_cloexec(sockfds[1]);

	if (sigemptyset(&smask) == -1)
		ERR(2, "sigemptyset");
	if (sigaddset(&smask, SIGCHLD) == -1)
		ERR(2, "sigaddset(SIGCHLD)");
	if (sigprocmask(SIG_BLOCK, &smask, 0) == -1)
		ERR(2, "sigprocmask");
	if ((sfd = signalfd(-1, &smask, SFD_CLOEXEC)) == -1)
		ERR(2, "signalfd");

	child = fork();
	if (child == 0) {
		if (signal(SIGCHLD, SIG_DFL) == SIG_ERR)
			ERR(2, "signal(SIGCHLD)");
		if (execvp(argv[0], argv) == -1)
			ERR(2, "execvp: %s", argv[0]);
		/* not reached */
		return 255;
	} else if (child == -1)
		ERR(2, "fork");
	/* else parent ... */

	comproot.uid = getuid();
	comproot.gid = getgid();

	close(sockfds[0]);
	tx_notifyfd(sockfds[1], &child, &notifyfd, 0);

	struct pollfd fds[] = {{notifyfd, POLLIN, 0}, {sfd, POLLIN, 0}};
	while (1) {
		rc = ppoll(fds, 2, 0, &smask);
		if (rc == -1)
			ERR(2, "ppoll");
		if (fds[0].revents)
			new_notification(fds[0].revents, notifyfd);
		if (fds[1].revents) {
			if (new_signal(fds[1].revents, sfd, child, &rc))
				break;
		}
	}

	file_walk(dump_files);
	return rc;
}

int main(int argc, char *argv[]) {
	char *sockfd_env = 0;

	if (argc < 2)
		ERRX(1, "Usage: %s PROGRAM [ARGUMENTS...]", argv[0]);

	if ((sockfd_env = getenv(ADVERTISEMENT)))
		return stage2(sockfd_env, argv);
	else
		return stage1(argv);
}
