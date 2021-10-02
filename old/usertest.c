#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

#define USERS_COUNT 64

void
show_who_am_i()
{
        printf("pid=%-8d uid=%-8d euid=%-8d\n", getpid(), getuid(), geteuid());
        return;
}

static int
child(void* arg)
{
        int err;
        int* uid = (int*)arg;

        // setuid() sets the effective user ID of the calling process.
        //
        // If the calling process is privileged (more precisely: if the process
        // has the CAP_SETUID capability in its user namespace), the real UID
        // and saved  set-user-ID  are  also set.
        //
        err = setuid(10001 + *uid);
        if (err == -1) {
                perror("setuid");
                exit(1);
        }

        show_who_am_i();
}

int
main(int argc, char** argv)
{
        int err;
        pid_t pids[USERS_COUNT];

        show_who_am_i();

        for (int i = 0; i < USERS_COUNT; i++) {
                void* stack = malloc(100);
                if (stack == NULL) {
                        perror("malloc");
                        return 1;
                }

                pids[i] = clone(child, stack + 100, SIGCHLD, &i);
                if (pids[i] == -1) {
                        perror("clone: ");
                        return 1;
                }
        }

        for (int i = 0; i < USERS_COUNT; i++) {
                err = waitpid(pids[i], NULL, 0);
                if (err == -1) {
                        perror("waitpid: ");
                        return 1;
                }
        }

        return 0;
}
