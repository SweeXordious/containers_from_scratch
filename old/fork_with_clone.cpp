#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cstring>
#include <sched.h>

#define STRINGIZE(x) #x

/**
 * A shortcut to call _assert with the correct line number.
 */
#define ASSERTMSG(condition, msg) _assert(condition, msg, __LINE__)

/**
 * Same as ASSERTMSG, but will use the condition as string to make the error message.
 */
#define ASSERT(condition) ASSERTMSG(condition, "assert " + STRINGIZE(condition) + "has failed")

/**
 * Show error message and exist if condition is false.
 * If is_syscall is set to true, will show errno current value.
 */
bool _assert(bool condition, const char msg_onerror[], int line, bool is_syscall=true) {
    if(!condition) {
        fprintf(stderr, "[%s] on line: %d\n", msg_onerror, line);

        if(is_syscall) 
            fprintf(stderr, "errno=%d [%s]\n", errno, std::strerror(errno));

        exit(1);
    }
    return true;
}


void unsharenamespaces() {
    const int UNSHARE_FLAGS = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNET | CLONE_FS | CLONE_FILES;
    const char* hostname = "doqueru";

    ASSERTMSG(-1 != unshare(UNSHARE_FLAGS), "unshare has failed");
    ASSERTMSG(-1 != sethostname(hostname, strlen(hostname)), "sethostname has failed");
}

void doqueru(char* exec_path, char** argv) {
    unsharenamespaces();

    pid_t child_pid = fork();
    ASSERTMSG(-1 != child_pid, "fork PID 1 has failed");

    int status;
    if(child_pid == 0) {
        ASSERTMSG(-1 != (child_pid = fork()), "fork PID 2 has failed");
        if(child_pid == 0)
            ASSERTMSG(-1 != execvp(exec_path, argv), "exec has failed");

        while (wait(&status) != -1 || errno != ECHILD);

        fprintf(stderr, "init died\n");
    }
    wait(&status);
}

struct clone_args {
    char* exec_path;
    char** argv;
};

int doqueru1(void *arg) {
    // unsharenamespaces();
    struct clone_args *args = (struct clone_args*) arg;

    char* exec_path = args->exec_path;
    char** argv = args->argv;
    
    pid_t child_pid = fork();
    ASSERTMSG(-1 != child_pid, "fork PID 1 has failed");

    int status;
    if(child_pid == 0) {
        ASSERTMSG(-1 != (child_pid = fork()), "fork PID 2 has failed");
        if(child_pid == 0) {
            printf(exec_path);
            printf(*argv);
            ASSERTMSG(-1 != execvp(exec_path, argv), "exec has failed");
        }
        while (wait(&status) != -1 || errno != ECHILD);

        fprintf(stderr, "init died\n");
    }
    wait(&status);

    return 1;
}

int main(int argc, char** argv) {
    // doqueru(argv[1], &argv[1]);

    void *pchild_stack = malloc(1024 * 1024);
    if(pchild_stack == NULL) {
        printf("ERROR: Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    struct clone_args args;
    args.exec_path = argv[1];
    args.argv = &argv[1];
    void* arg = (void*) &args;

    int pid = clone(doqueru1, pchild_stack + (1024*1024), CLONE_FS, arg);
    if(pid < 0) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
    }

    wait(NULL);
    free(pchild_stack);
    printf("INFO: Child process terminated.\n");

    exit(0);
}
