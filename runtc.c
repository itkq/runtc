#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* A simple error-handling function: print an error message based
   on the value in 'errno' and terminate the calling process */
#define errExit(msg)                                                           \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

static void usage(char *pname) {
  fprintf(stderr, "Usage: %s cmd [arg...]\n", pname);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  pid_t pid;
  int status;
  int unshare_flags = CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWPID;
  const char *hostname = "runtc";
  const char *domainname = "runtc";
  const char *rootpath = "./root";

  if (chroot(rootpath) != 0)
    errExit("chroot");

  if (chdir("/"))
    errExit("chdir");

  if (-1 == unshare(unshare_flags))
    errExit("unshare");

  if (argc < 2) {
    usage(argv[0]);
  }

  pid = fork();
  switch (pid) {
  case -1:
    errExit("fork");
  case 0: // child
    break;
  default: // parent
    if (waitpid(pid, &status, 0) == -1)
      errExit("waitpid");

    exit(EXIT_SUCCESS);
  }

  /* change the propagation type */
  if (mount("none", "/proc", NULL, MS_PRIVATE | MS_REC, NULL) != 0)
    errExit("unmount");

  if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) !=
      0)
    errExit("mount");

  if (sethostname(hostname, strlen(hostname)) != 0)
    errExit("sethostname");

  if (setdomainname(domainname, strlen(domainname)) != 0)
    errExit("setdomainname");

  execvp(argv[1], &argv[1]);
}
