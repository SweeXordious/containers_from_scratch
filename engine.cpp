#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sched.h>
#include <stdlib.h>
#include <string>
#include <sys/mount.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

const char *concat(int num_args, ...) {
  int strsize = 0;
  va_list ap;
  va_start(ap, num_args);
  for (int i = 0; i < num_args; i++)
    strsize += strlen(va_arg(ap, char *));

  char *res = (char *)malloc(strsize + 1);
  strsize = 0;
  va_start(ap, num_args);
  for (int i = 0; i < num_args; i++) {
    char *s = va_arg(ap, char *);
    strcpy(res + strsize, s);
    strsize += strlen(s);
  }
  va_end(ap);
  res[strsize] = '\0';

  return res;
}

void assertTrue(bool condition, const char message[], int exitStatus = 1,
                FILE *stream = stderr) {
  if (condition) {
    fprintf(stream, "Assert failed:\n\tMessage: %s\n\tReason: %m\n", message);
    exit(exitStatus);
  }
}

void chrootToFs(char *fs_path) {
  assertTrue(-1 == chroot(fs_path), "Couldn't chroot to file system.");
  assertTrue(-1 == chdir("/"), "Couldn't change directory after chroot.");
}

void mountProc() {
  assertTrue(-1 == mount("none", "/proc", "proc", 0, NULL),
             "Mounting Proc FS has failed.");
}

void mountRoot() {
  assertTrue(-1 == mount("none", "/", NULL, 0, NULL),
             "Mounting Root has failed.");
}

void setHostName(const char *hostname = "simple_engine") {
  assertTrue(-1 == sethostname(hostname, strlen(hostname)),
             "Coudldn't set the host name.");
}

void printFile(const char *path, FILE *stream = stderr) {
  FILE *fp = fopen(path, "r");
  assertTrue(NULL == fp,
             concat(3, "Couldn't open ", path, " file for printing"));

  char ch;
  while ((ch = fgetc(fp)) != EOF) {
    fprintf(stream, "%c", ch);
  }
  assertTrue(fclose(fp), concat(2, "Coudln't close ", path));
}

void printUidMapFile(int pid, FILE *stream = stderr) {
  char path[25];
  sprintf(path, "/proc/%d/uid_map", pid);
  printFile(path, stream);
}

void printGidMapFile(int pid, FILE *stream = stderr) {
  char path[25];
  sprintf(path, "/proc/%d/gid_map", pid);
  printFile(path, stream);
}

void writeToFile(const char *path, const char *content) {
  FILE *fp = fopen(path, "w");
  assertTrue(fp == NULL,
             concat(3, "Couldn't open ", path, " file for writing."));
  assertTrue(0 >= fprintf(fp, "%s\n", content),
             concat(2, "Couldnt write to ", path));
  assertTrue(-1 == fclose(fp), concat(2, "Coudln't close ", path));
}

void mapUidToRoot(int uid) {
  char path[100];
  sprintf(path, "/proc/%d/uid_map", getpid());

  char mapping[30];
  sprintf(mapping, "0 %d 1", uid);
  writeToFile(path, mapping);
}

void mapGidToRoot(int gid) {
  char path[100];
  sprintf(path, "/proc/%d/gid_map", getpid());

  char mapping[20];
  sprintf(mapping, "0 %d 1", gid);
  writeToFile(path, mapping);
}

void unshareUserNameSpace() {
  assertTrue(-1 == unshare(CLONE_NEWUSER), "Couldn't unshare USER namespace.");
}

void unshareNameSpaces(int flags) {
  assertTrue(-1 == unshare(flags),
             "Couldn't unshare PID | NS | UTS namespaces.");
}

static int startEngine(char *fsPath, char *command, char **args) {
  int uid = getuid();
  int gid = getgid();

  unshareUserNameSpace();
  mapUidToRoot(uid);
  // mapGidToRoot(gid);

  unshareNameSpaces(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS);
  setHostName();

  int status;
  pid_t firstChildPid = fork();
  assertTrue(-1 == firstChildPid, "couldn't fork PID 1.");
  if (firstChildPid == 0) {
    chrootToFs(fsPath);
    mountProc();

    pid_t secondChildPid = fork();
    assertTrue(-1 == secondChildPid, "couldn't fork PID 2.");
    if (secondChildPid == 0) {
      assertTrue(-1 == execvp(command, args), "couldn't execvp");
      while (wait(&status) != -1 || errno != ECHILD)
        ;
    }
  }
  wait(&status);
  return 0;
}

int main(int argc, char **argv) {
  startEngine(argv[1], argv[2], &argv[2]);
  return 0;
}