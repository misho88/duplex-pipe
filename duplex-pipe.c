#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define NAME "duplex-pipeline"
#define PREFIX NAME ": "

void
usage()
{
	fprintf(stderr, "usage: " NAME " [-i STDIN_FD(0)] [-o STDOUT_FD(1)] [-I REVIN_FD(0)] [-O REVOUT_FD(1)] slave... '' master...\n");
	exit(100);
}

int
get_fd(char * context, char * text)
{
	if (text[0] == '\0') fprintf(stderr, PREFIX "for %s: not a number: ''\n", context), usage();

	char * endptr;
	long int value = strtol(text, &endptr, 10);
	if (endptr[0] != '\0') fprintf(stderr, PREFIX "for %s: not a number: %s\n", context, text), usage();
	if (value > 0x7FFFFFFF || value < 0) fprintf(stderr, "duplex-pipeline: for %s: out of range: %ld\n", context, value), usage();
	return (int)value;
}

void
wait_pids(int pids[2])
{
	for (int i = 0; i < 2;) {
		int status;
		if (waitpid(pids[i], &status, 0) < 0) perror(PREFIX "waitpid()"), exit(errno);
		if (WIFEXITED(status)) {
			int code = WEXITSTATUS(status);
			if (code) exit(code);
			i++;
		}
	}
}

void
delete(int fd)
{
	if (close(fd) < 0) perror(PREFIX "close()"), exit(errno);
}

void
move(int oldfd, int newfd)
{
	if (oldfd == newfd) return;
	if (dup2(oldfd, newfd) < 0) perror(PREFIX "dup2()"), exit(errno);
	if (close(oldfd) < 0) perror(PREFIX "close()"), exit(errno);
}

void
execute(char ** argv)
{
	execvp(argv[0], argv);
	fprintf(stderr, PREFIX "%s: ", argv[0]);
	perror("");
	exit(errno);
}

struct pipe { int rfd, wfd; }
make_pipe()
{
	int fds[2];
	if (pipe(fds) < 0) perror(PREFIX "pipe()"), exit(errno);
	return (struct pipe){ .rfd = fds[0], .wfd = fds[1] };
}

pid_t
do_fork()
{
	pid_t pid = fork();
	if (pid < 0) perror(PREFIX "fork()"), exit(errno);
	return pid;
}

int
main(int argc, char ** argv)
{
	struct args { int stdin, stdout, revin, revout; } args = { 0, 1, 0, 1 };
	int i = 1;
	for (; i < argc && argv[i][0] == '-'; i++) {
		if (i == argc - 1 || argv[i][1] == '\0' || argv[i][2] != '\0') usage();
		switch (argv[i][1]) {
		case 'i': args.stdin  = get_fd(argv[i], argv[i + 1]); i++; break;
		case 'I': args.revin  = get_fd(argv[i], argv[i + 1]); i++; break;
		case 'o': args.stdout = get_fd(argv[i], argv[i + 1]); i++; break;
		case 'O': args.revout = get_fd(argv[i], argv[i + 1]); i++; break;
		default: usage();
		}
	}

	int slave = i, master = argc;
	for (; i < argc; i++) {
		if (argv[i][0] == '\0') {
			master = i + 1;
			argv[i] = NULL;
			break;
		}
	}

	if (     i ==  argc    ) fprintf(stderr, PREFIX "block separator '' missing \n"), exit(100);
	if (master == slave + 1) fprintf(stderr, PREFIX "empty slave block\n"          ), exit(100);
	if (     i ==  argc - 1) fprintf(stderr, PREFIX "empty master block\n"         ), exit(100);

	struct pipe std = make_pipe(), rev = make_pipe();

	pid_t pids[2];
	pids[0] = do_fork();
	if (pids[0]) {
		delete(std.wfd);
		move(std.rfd, args.stdin);
		delete(rev.rfd);
		move(rev.wfd, args.revout);

		pids[1] = do_fork();
		if (pids[1])
			wait_pids(pids);
		else
			execute(argv + master);
	}
	else {
		delete(std.rfd);
		move(std.wfd, args.stdout);
		delete(rev.wfd);
		move(rev.rfd, args.revin);
		execute(argv + slave);
	}
	return 0;
}
