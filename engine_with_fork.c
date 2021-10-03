#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sched.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

void chroot_to_fs(char *fs_path) {
  if (-1 == chroot(fs_path)) {
    fprintf(stderr, "%s\n%m", "chroot has failed");
    exit(1);
  }

  if (-1 == chdir("/")) {
    fprintf(stderr, "%s\n%m", "chdir has failed");
    exit(1);
  }
}

void mount_proc() {
  if (-1 == mount("none", "/proc", "proc", 0, NULL)) {
    fprintf(stderr, "%s\n%m", "mounting proc has failed");
    exit(1);
  }
}

void mount_root() {
  if (-1 == mount("none", "/", NULL, 0, NULL)) {
    fprintf(stderr, "%s\n", "mounting root has failed");
    exit(1);
  }
}

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */

static char child_stack[STACK_SIZE];

struct clone_args {
  char *fs_path;
  char *command;
  char **args;
};

void setHostName() {
  const char* hostname = "simple_engine";
  if(-1 == sethostname(hostname, strlen(hostname))) {
    fprintf(stderr, "%s\n%m", "sethostname has failed");
    exit(1);
  }
}

static int engine(void *arg) {
  struct clone_args *args = (struct clone_args *)arg;
  int status;
  pid_t first_child_pid = fork();
  if (-1 == first_child_pid) {
    fprintf(stderr, "%s: %m\n", "couldn't fork PID 1");
    exit(1);
  }
  if (first_child_pid == 0) {
    chroot_to_fs(args->fs_path);
    mount_proc();
    if (-1 == execvp(args->command, args->args)) {
      fprintf(stderr, "%s\n%m", "couldn't execvp");
      exit(1);
    }
    while (wait(&status) != -1 || errno != ECHILD);
  }
  // int out = wait(&status);
  // fprintf(stderr,"%d\n", out);
  fprintf(stderr, "engine finished");
  return 0;
}

int main(int argc, char **argv) {
  struct clone_args args = {argv[1], argv[2], &argv[2]};

  int status;
  pid_t child_pid = clone(
    engine,
    child_stack + STACK_SIZE,
    CLONE_NEWUSER | CLONE_NEWNS |CLONE_NEWPID | CLONE_NEWUTS,
    (void *)&args
  );
  if(child_pid == -1 ){
    fprintf(stderr, "Coudln't clone\n%m");
    exit(1);
  }
  while (wait(&status) != -1 || errno != ECHILD);
  int out = wait(&status);
  fprintf(stderr,"%d\n%m", out);
  // waitpid(child_pid, );
  return 0;
}
