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

void print_uid_map(int pid) {
  char path[100];
  sprintf(path, "/proc/%d/uid_map", pid);

  FILE *fp = fopen(path, "r");
  if (!fp) {
    printf("Couldn't open uid_map file.\n");
    exit(1);
  }

  printf("uid_map: \n");

  char ch;
  while ((ch = fgetc(fp)) != EOF) {
    printf("%c", ch);
  }
  fclose(fp);
}

void map_id_to_root() {
  int pid = getpid();
  int uid = getuid();
  //   print_uid_map (getpid());
  //   setuid(0);
  //   print_uid_map (getpid());

  char mapping[20];
  sprintf(mapping, "0 %d 1", uid);
  printf("mapping: %s\n", mapping);

  char path[100];
  sprintf(path, "/proc/%d/uid_map", pid);

  printf("path: %s\n", path);
  char ch;
  print_uid_map(getpid());
  FILE *fp = fopen(path, "w+");
  if (!fp) {
    printf("Couldn't open uid_map for writing.\n");
    exit(1);
  }

  int written = fprintf(fp, "%s\n", mapping);

  if (written <= 0) {
    printf("Couldnt write to uid_map file.\n");
    exit(1);
  }

  if (fclose(fp) == -1) {
    fprintf(stderr, "Coudln't close uid_map file.\n%m");
    exit(1);
  }
  print_uid_map(getpid());
}

int map_id2() {
  int child_pid = getpid();
  char path[100] = {0};
  int uid_map = 0;
  if (snprintf(path, sizeof(path), "/proc/%d/%s", child_pid, "uid_map") >
      sizeof(path)) {
    fprintf(stderr, "snprintf too big? %m\n");
    return -1;
  }
  fprintf(stderr, "writing %s...", "uid_map");
  if ((uid_map = open(path, O_WRONLY)) == -1) {
    fprintf(stderr, "open failed: %m\n");
    return -1;
  }
  if (dprintf(uid_map, "0 %d %d\n", 10000, 2000) == -1) {
    fprintf(stderr, "dprintf failed: %m\n");
    close(uid_map);
    return -1;
  }
  close(uid_map);
  // if (write(fd, &(int){0}, sizeof(int)) != sizeof(int)) {
  //   fprintf(stderr, "couldn't write: %m\n");
  //   return -1;
  // }
}

void map_id() {
  int pid = getpid();
  char file[100];
  sprintf(file, "/proc/%d/uid_map", pid);

  char *buf;
  int fd;
  printf(file);
  fd = open(file, 1); // write only
  if (fd < 0) {
    printf("Coudln't open file for writing.\n");
    exit(1);
  }

  int uid = getuid();
  char mapping[100];
  sprintf(buf, "0 %d 1", uid);
  printf(buf);

  if (write(fd, buf, strlen(buf))) {
    printf("Coudln't write mapping into file.\n");
    exit(1);
  }

  free(buf);
  close(fd);
}

// void start_simple_docker(char *fs_path, char *command, char **args) {

//   pid_t first_child_pid = fork();
//   if (-1 == first_child_pid) {
//     fprintf(stderr, "%s\n", "couldn't fork PID 1");
//     exit(1);
//   }

//   int status;
//   if (first_child_pid == 0) {
//     // chroot_to_fs(fs_path);
//     // fprintf(stderr, "%d", getuid());
//     // if(setuid(0) == -1) {
//     //   fprintf(stderr, "Could not set uid %m");
//     //   exit(1);
//     // }
//     // map_id2();
//     // map_id_to_root();
//     pid_t second_child_pid = fork();
//     if (-1 == second_child_pid) {
//       fprintf(stderr, "%s\n", "couldn't fork PID 2");
//       exit(1);
//     }
//     if (second_child_pid == 0) {
//       if (-1 == execvp(command, args)) {
//         fprintf(stderr, "%s\n", "couldn't execvp");
//         exit(1);
//       }
//     }

//   }

// }

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */

static char child_stack[STACK_SIZE];

struct clone_args {
  char *fs_path;
  char *command;
  char **args;
};

static int childFunc(void *arg) {
  struct clone_args *args = (struct clone_args *)arg;
  int status;
  pid_t first_child_pid = fork();
  if (-1 == first_child_pid) {
    fprintf(stderr, "%s: %m\n", "couldn't fork PID 1");
    exit(1);
  }
  if (first_child_pid == 0) {
    // mount_root();
    chroot_to_fs(args->fs_path);
    mount_proc();
    // map_id2();
    if (-1 == execvp(args->command, args->args)) {
      fprintf(stderr, "%s\n%m", "couldn't execvp");
      exit(1);
    }
    while (wait(&status) != -1 || errno != ECHILD);
  }
  wait(&status);
  fprintf(stderr, "child_func end");
  return 0;
}

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

int main(int argc, char **argv) {
  struct clone_args args = {argv[1], argv[2], &argv[2]};
  int status;
  pid_t child_pid = clone(
    childFunc,
    child_stack + STACK_SIZE,
    CLONE_NEWUSER | CLONE_NEWNS |CLONE_NEWPID | CLONE_NEWUTS,
    (void *)&args
  );
  if(child_pid == -1 ){
    fprintf(stderr, "Coudln't clone\n%m");
    exit(1);
  }
  // waitpid(child_pid, &status, 0);
  wait(&status);
  // start_simple_docker(argv[1], argv[2], &argv[2]);

  while (1);
  return 0;
}
