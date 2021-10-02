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

void unshare_uts_namespace() {
  if (0 != unshare(CLONE_NEWUTS)) {
    fprintf(stderr, "%s\n", "UTS unshare has failed");
    exit(1);
  }

  const char *hostname = "simple_docker";
  if (-1 == sethostname(hostname, strlen(hostname))) {
    fprintf(stderr, "%s\n", "sethostname has failed");
    exit(1);
  }
}

void unshare_pid_namespace() {
  if (0 != unshare(CLONE_NEWPID)) {
    fprintf(stderr, "%s\n", "PID unshare has failed");
    exit(1);
  }
}

void unshare_mount_namespace() {
  if (0 != unshare(CLONE_NEWNS)) {
    fprintf(stderr, "%s\n", "NS unshare has failed");
    exit(1);
  }
}

void unshare_fs_namespace() {
  if (0 != unshare(CLONE_FS)) {
    fprintf(stderr, "%s\n", "FS unshare has failed");
    exit(1);
  }
}

void unshare_user_namespace() {
  if (0 != unshare(CLONE_NEWUSER)) {
    fprintf(stderr, "%s\n", "USER unshare has failed");
    exit(1);
  }
}

void unshare_namespaces() {
  unshare_user_namespace();
  unshare_uts_namespace();
  unshare_pid_namespace();
  unshare_mount_namespace();
  unshare_fs_namespace();
}

void chroot_to_fs(char *fs_path) {
  if (-1 == chroot(fs_path)) {
    fprintf(stderr, "%s\n", "chroot has failed");
    exit(1);
  }

  if (-1 == chdir("/")) {
    fprintf(stderr, "%s\n", "chdir has failed");
    exit(1);
  }
}

void mount_proc() {
  if (-1 == mount("none", "/proc", "proc", 0, NULL)) {
    fprintf(stderr, "%s\n", "mounting proc has failed");
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

  if (fclose(fp) != 0) {
    printf("Coudln't close uid_map file.\n");
    exit(1);
  }
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

void start_simple_docker(char *fs_path, char *command, char **args) {
  
  //   map_id_to_root();
  // map_id();
  // mount_proc();
  // mount_root();

  pid_t first_child_pid = fork();
  if (-1 == first_child_pid) {
    fprintf(stderr, "%s\n", "couldn't fork PID 1");
    exit(1);
  }

  int status;
  if (first_child_pid == 0) {
    unshare_namespaces();
    chroot_to_fs(fs_path);
    // fprintf(stderr, "%d", getuid());
    // if(setuid(0) == -1) {
    //   fprintf(stderr, "Could not set uid %m");
    //   exit(1);
    // }
    // map_id2();
    // map_id_to_root();
    pid_t second_child_pid = fork();
    if (-1 == second_child_pid) {
      fprintf(stderr, "%s\n", "couldn't fork PID 2");
      exit(1);
    }
    if (second_child_pid == 0) {
      if (-1 == execvp(command, args)) {
        fprintf(stderr, "%s\n", "couldn't execvp");
        exit(1);
      }
    }
    while (wait(&status) != -1 || errno != ECHILD);
    fprintf(stderr, "init died\n");
  }
  wait(&status);
}

int main(int argc, char **argv) {
  start_simple_docker(argv[1], argv[2], &argv[2]);
  return 0;
}
