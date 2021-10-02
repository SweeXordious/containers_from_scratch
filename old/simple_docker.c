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
  unshare_fs_namespace();
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
    fprintf(stderr, "%s\n", "mounting proc has failed");
    exit(1);
  }
}

void mount_root() {
  if(-1 == mount("none", "/", NULL, 0, NULL)) {
    fprintf(stderr, "%s\n", "mounting root has failed");
    exit(1);
  }
}

void print_uid_map(int pid) {
    char path[100];
    sprintf(path, "/proc/%d/uid_map", pid);

    FILE *fp = fopen(path, "r");
    printf("uid_map: \n");

    char ch;
    while ((ch = fgetc(fp) )!= EOF)
    {
       printf("%c", ch);
    }
    fclose(fp);
}

void map_id_to_root() {
  int pid = getpid();
  int uid = getuid();
  print_uid_map (getpid());
  setuid(0);
  print_uid_map (getpid());

  char mapping[20];
  sprintf(mapping, "0 %d 1", uid);
  printf("%s\n", mapping);

  char path[100];
  sprintf(path, "/proc/%d/uid_map", pid);

  printf("path: %s\n", path);
  char ch;

  FILE *fp = fopen(path, "w+");
  if (!fp) {
    printf("Couldn't open file.\n");
  }

  // int written = fprintf(fp, "%s\n", mapping);
  //
  // if (written <= 0){
  //   printf("Couldnt write\n");
  // }

  if (fclose(fp) != 0) {
    printf("Coudln't close file.\n");
    exit(1);
  }
}

// void map_id(const char *file, uint32_t from, uint32_t to)
// {
// 	char *buf;
// 	FILE *fd;

// 	fd = fopen(file, 'w');
// 	if (fd < 0) {
// 		 printf("Couldn't open uid map file.\n");
//      free(buf);
// 	   close(fd);
//      exit(1);
//   }

// 	sprintf(&buf, "%u %u 1", from, to);
// 	if (fprintf(fd, "%s", mapping)) {
// 		 printf("Couldn't write to uid map file.\n");
//      free(buf);
// 	   close(fd);
//      exit(1);
//   }

// 	free(buf);
// 	close(fd);
// }

void start_simple_docker(char* fs_path, char* command, char** args) {
  unshare_namespaces();

  pid_t first_child_pid = fork();
  if(-1 == first_child_pid) {
    fprintf(stderr, "%s\n", "couldn't fork PID 1");
    exit(1);
  }

  chroot_to_fs(fs_path);
  map_id_to_root();
  mount_proc();
  // mount_root();

  int status;
  if(first_child_pid == 0) {
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
