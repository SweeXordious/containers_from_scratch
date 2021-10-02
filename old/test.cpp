#include <sched.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <system_error>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

void unshare_user_namespace() {
  if (0 != unshare(CLONE_NEWUSER)) {
    fprintf(stderr, "%s\n", "USER unshare has failed");
    exit(1);
  }
}

void map_id() {
  int pid = getpid();
  char file[100];
  if (0 > sprintf(file, "/proc/%d/uid_map", pid)) {
    printf("Couldn't sprintf uid_map path.");
    exit(1);
  }

  int fd;
  fd = open(file, 1);
  if (fd < 0) {
    printf("Coudln't open file for writing.\n");
    exit(1);
  }

  int uid = getuid();
  char * buf;
  if (0 > sprintf(buf, "0 %d 1", uid)) {
    printf("Couldn't sprintf uid_map content.");
    exit(1);
  }

  if (write(fd, buf, strlen(buf))) {
    printf("Coudln't write mapping into file.\n");
    exit(1);
  }

  free(buf);
  close(fd);
}

void start(char * command, char ** args) {
  unshare_user_namespace();
  int fork_pid = fork();

  if (-1 == fork_pid) {
    fprintf(stderr, "%s\n", "couldn't fork");
    exit(1);
  }

  int status;
  if (0 == fork_pid) {
    map_id();

    if (-1 == execvp(command, args)) {
      fprintf(stderr, "%s\n", "couldn't execvp");
      exit(1);
    }
    while (wait(&status) != -1 || errno != ECHILD);
    fprintf(stderr, "init died\n");
  }
  wait(&status);
}

int main(int argc, char ** argv) {
  start(argv[1], & argv[1]);
  int status;
  wait( & status);
  return 0;
}