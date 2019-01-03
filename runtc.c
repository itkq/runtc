#define _GNU_SOURCE
#include <err.h>
#include <errno.h>
#include <fcntl.h>
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

#define _PATH_PROC_SETGROUPS "/proc/self/setgroups"
#define _PATH_PROC_UIDMAP "/proc/self/uid_map"
#define _PATH_PROC_GIDMAP "/proc/self/gid_map"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

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

static inline int write_all(int fd, const void *buf, size_t count) {
  while (count) {
    ssize_t tmp;

    errno = 0;
    tmp = write(fd, buf, count);
    if (tmp > 0) {
      count -= tmp;
      if (count)
        buf = (const void *)((const char *)buf + tmp);
    } else if (errno != EINTR && errno != EAGAIN)
      return -1;
    if (errno == EAGAIN) /* Try later, *sigh* */
      usleep(250000);
  }
  return 0;
}

static void map_id(const char *file, u_int32_t from, u_int32_t to) {
  char *buf;
  int fd;

  fd = open(file, O_WRONLY);
  if (fd < 0)
    errExit("map_id() open");

  asprintf(&buf, "%u %u 1", from, to);
  if (write_all(fd, buf, strlen(buf)))
    errExit("write_all");
  free(buf);
  close(fd);
}

enum {
  SETGROUPS_NONE = -1,
  SETGROUPS_DENY = 0,
  SETGROUPS_ALLOW = 1,
};

static const char *setgroups_strings[] = {[SETGROUPS_DENY] = "deny",
                                          [SETGROUPS_ALLOW] = "allow"};

static void setgroups_control(int action) {
  const char *file = _PATH_PROC_SETGROUPS;
  const char *cmd;
  int fd;

  if (action < 0 || (size_t)action >= ARRAY_SIZE(setgroups_strings))
    return;
  cmd = setgroups_strings[action];

  fd = open(file, O_WRONLY);
  if (fd < 0) {
    if (errno == ENOENT)
      return;
    errExit("open");
  }

  if (write_all(fd, cmd, strlen(cmd)))
    errExit("write_all");
  close(fd);
}

int main(int argc, char *argv[]) {
  pid_t pid;
  int status;
  uid_t real_euid = geteuid();
  gid_t real_egid = getegid();
  int unshare_flags = CLONE_NEWCGROUP | CLONE_NEWUSER | CLONE_NEWUTS |
                      CLONE_NEWNS | CLONE_NEWPID;
  const char *hostname = "runtc";
  const char *domainname = "runtc";
  const char *rootpath = "../root";

  if (argc < 2) {
    usage(argv[0]);
  }

  if (-1 == unshare(unshare_flags))
    errExit("unshare");

  if (chroot(rootpath) != 0)
    errExit("chroot");

  if (chdir("/"))
    errExit("chdir");

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

  /* since Linux 3.19 unprivileged writing of /proc/self/gid_map
   * has s been disabled unless /proc/self/setgroups is written
   * first to permanently disable the ability to call setgroups
   * in that user namespace. */
  setgroups_control(SETGROUPS_DENY);
  map_id(_PATH_PROC_UIDMAP, 0, real_euid);
  map_id(_PATH_PROC_GIDMAP, 0, real_egid);

  if (sethostname(hostname, strlen(hostname)) != 0)
    errExit("sethostname");

  if (setdomainname(domainname, strlen(domainname)) != 0)
    errExit("setdomainname");

  execvp(argv[1], &argv[1]);
}
