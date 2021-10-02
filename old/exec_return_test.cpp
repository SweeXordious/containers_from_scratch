#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char** argv) {
    
    printf("\n\n[Executing command]:\n");

    fork();
    fork();
    
    int status;
    while(wait(&status) != -1 || errno != ECHILD);


    // printf("\n\n[Executing command122]:\n");

    execvp(argv[1], &argv[1]);

    printf("Will we come here ?");
    return 0;
}