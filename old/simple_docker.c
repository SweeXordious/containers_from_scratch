#include <sched.h>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <system_error>

void unshare_uts_namespace() {
  if(0 != unshare(CLONE_NEWUTS)) {
    fprintf(stderr, "%s\n", "UTS unshare has failed");
    exit(1);
  }

  const char* hostname = "simple_docker";
  if(-1 == sethostname(hostname, strlen(hostname))) {
    fprintf(stderr, "%s\n", "sethostname has failed");
    exit(1);
  }
}

void unshare_pid_namespace() {
  if(0 != unshare(CLONE_NEWPID)) {
    fprintf(stderr, "%s\n", "PID unshare has failed");
    exit(1);
  }
}

void unshare_mount_namespace() {
  if(0 != unshare(CLONE_NEWNS)) {
    fprintf(stderr, "%s\n", "NS unshare has failed");
    exit(1);
  }
}

void unshare_fs_namespace() {
  if(0 != unshare(CLONE_FS)) {
    fprintf(stderr, "%s\n", "FS unshare has failed");
    exit(1);
  }
}

void unshare_user_namespace() {
  if(0 != unshare(CLONE_NEWUSER)) {
    fprintf(stderr, "%s\n", "USER unshare has failed");
    exit(1);
  }
}

void unshare_namespaces() {
  unshare_user_namespace();
  unshare_uts_namespace();
  unshare_pid_namespace();
  unshare_mount_namespace();
  // unshare_fs_namespace();
}

void chroot_to_fs(char* fs_path) {
  if(-1 == chroot(fs_path)) {
    fprintf(stderr, "%s\n", "chroot has failed");
    exit(1);
  }

  if(-1 == chdir("/")) {
    fprintf(stderr, "%s\n", "chdir has failed");
    exit(1);
  }
}

void mount_proc() {
  if(-1 == mount("none", "/proc", "proc", 0, NULL)) {
    fprintf(stderr, "%s\n%m", "mounting proc has failed");
    exit(1);
  }
}

void mount_root() {
  if(-1 == mount("none", "/", NULL, 0, NULL)) {
    fprintf(stderr, "%s\n", "mounting root has failed");
    exit(1);
  }
}

void start_simple_docker(char* fs_path, char* command, char** args) {

  pid_t first_child_pid = fork();
  if(-1 == first_child_pid) {
    fprintf(stderr, "%s\n", "couldn't fork PID 1");
    exit(1);
  }
    unshare_namespaces();


  // mount_root();

  int status;
  if(first_child_pid == 0) {
    chroot_to_fs(fs_path);
    mount_proc();

    pid_t second_child_pid = fork();
    if(-1 == second_child_pid) {
      fprintf(stderr, "%s\n", "couldn't fork PID 2");
      exit(1);
    }


    if(second_child_pid == 0 ){
      if(-1 == execvp(command, args)) {
        fprintf(stderr, "%s\n", "couldn't execvp");
        exit(1);
      }
    }
    while (wait(&status) != -1 || errno != ECHILD);
    fprintf(stderr, "init died\n");
  }
  wait(&status);
}

int main(int argc, char** argv) {
  start_simple_docker(argv[1], argv[2], &argv[2]);
  return 0;
}
