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

int main(int argc, char *argv[]) {
  pid_t pid;
  int status;
  int unshare_flags = CLONE_NEWNS | CLONE_NEWPID;

  if (-1 == unshare(unshare_flags))
    errExit("unshare");

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

  if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) !=
      0)
    errExit("mount");

  execlp("ps", "ps", "a", (char *)NULL);
}
