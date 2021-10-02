#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    pid_t pid_parent = getpid();
    fork(); // This line will be called from parent's process

    if(getpid() == pid_parent) {
        printf("parent process [PID = %d]\n", getpid());
    } else {
        printf("child process [PID = %d]\n", getpid());
        printf("\n\npress CTRL + C to exit");
    }

    while(1) {
        sleep(1);
    }
    return 0;
}