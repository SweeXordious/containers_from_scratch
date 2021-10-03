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

struct engineArgs  {
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

void map_id_to_root(char* path, int uid) {
  //   print_uid_map (getpid());
  //   setuid(0);
  //   print_uid_map (getpid());

  char mapping[20];
  sprintf(mapping, "0 %d 1", uid);
  printf("mapping: %s\n", mapping);

  printf("path: %s\n", path);
  char ch;
  print_uid_map(getpid());
  FILE *fp = fopen(path, "w");
  if (!fp) {
    fprintf(stderr, "Couldn't open  uid_map for writing.\n%m\n");
    exit(1);
  }

  int written = fprintf(fp, "%s\n", mapping);

  if (written <= 0) {
    printf("Couldnt write to uid_map file.\n");
    exit(1);
  }

  if (fclose(fp) == -1) {
    fprintf(stderr, "Coudln't close uid_map file.\n%m");
    // exit(1);
  }
  print_uid_map(getpid());
}


int child_pid = 0;
int done = 0;

static int startEngine(void *arg) {
  struct engineArgs *args = (struct engineArgs *)arg;
  int status;
  fprintf(stderr, "uid: %d\n", getuid());
  fprintf(stderr, "pid: %d\n", getpid());
  int uid = getuid();
  if (-1 == unshare(CLONE_NEWUSER)) {
    fprintf(stderr, "Couldn't unshare USER namespace\n%m\n");
    exit(1);
  }
  char path[100];
    // sprintf(path, "%sproc/%d/uid_map",args->fs_path, getpid());
    sprintf(path, "/proc/%d/uid_map", getpid());
    fprintf(stderr, "Trying to open file: %s\n", path);
    FILE *fp = fopen(path, "r");
    if (!fp) {
      fprintf(stderr, "Couldn't open uid_map file.\n%m\n");
    }
    fprintf(stderr, "pid: %d\n", getpid());

    printf("uid_map: \n");

    char ch;
    while ((ch = fgetc(fp)) != EOF) {
      printf("%c", ch);
    }
    fclose(fp);
    fprintf(stderr, "Finished open file:\n");

    sleep(1);
    map_id_to_root(path, uid);


  if (-1 == unshare(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS)) {
    fprintf(stderr, "Couldn't unshare PID namespace\n%m\n");
    exit(1);
  }

  pid_t first_child_pid = fork();
  if (-1 == first_child_pid) {
    fprintf(stderr, "%s: %m\n", "couldn't fork PID 1");
    exit(1);
  }
  if (first_child_pid == 0) {

    chroot_to_fs(args->fs_path);
    mount_proc();
    child_pid = 1;
    int delay = 2;
    while(done == 0 && delay >0) {
      fprintf(stderr, "sleeping...");
      sleep(1);
      delay--;
    }
    if (-1 == execvp(args->command, args->args)) {
      fprintf(stderr, "%s\n%m", "couldn't execvp");
      exit(1);
    }
    while (wait(&status) != -1 || errno != ECHILD);
  }
  wait(&status);
  fprintf(stderr, "engine finished");
  return 0;
}


int main(int argc, char **argv) {
  struct engineArgs args = {argv[1], argv[2], &argv[2]};

  pid_t child_pid = clone(
    startEngine,
    child_stack + STACK_SIZE,
    0,
    (void *)&args
  );
  if(child_pid == -1 ){
    fprintf(stderr, "Coudln't clone\n%m");
    exit(1);
  }

  fprintf(stderr, "\nProceeding with the mapping: uid: %d, pid: %d\n", getuid(), getpid());

  // while(child_pid == 0) {
  //   sleep(1);
  // }
  // if (child_pid != 0 ){
  //   fprintf(stderr, "Proceeding with the mapping2\n");
  //   char path[100];
  //   sprintf(path, "%sproc/%d/uid_map",args.fs_path, getpid());

  //   fprintf(stderr, "Trying to open file:%s\n", path);
  //   FILE *fp = fopen(path, "r");
  //   if (!fp) {
  //     fprintf(stderr, "Couldn't open uid_map file.\n%m\n");
  //     exit(1);
  //   }

  //   printf("uid_map: \n");

  //   char ch;
  //   while ((ch = fgetc(fp)) != EOF) {
  //     printf("%c", ch);
  //   }
  //   fclose(fp);
  //   fprintf(stderr, "Finished open file:\n");
  //   map_id_to_root(path, getuid());
  //   done = 1;
  // }

  sleep(30);
  return 0;
}