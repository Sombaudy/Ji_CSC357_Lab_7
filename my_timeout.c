#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // Include signal.h for sigaction

void timeout_handler(int signum) {
    printf("Killing child...\n");
    kill(0, SIGKILL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <timeout_seconds> ./<program> [args...]\n", argv[0]);
        return 1;
    }

    int timeout_seconds = atoi(argv[1]);

    struct sigaction sa;
    sa.sa_handler = timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    } else if(pid == 0) {
        // Child process
        alarm(timeout_seconds);
        execvp(argv[2], &argv[2]);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int child_status = WEXITSTATUS(status);
            if (child_status == 0) {
                printf("Child process completed successfully with status: %d\n", child_status);
            } else {
                printf("Child process completed with status: %d\n", child_status);
            }
            return child_status;
        } else if (WIFSIGNALED(status)) {
            printf("Child process terminated by signal: %d\n", WTERMSIG(status));
            return EXIT_FAILURE;
        } else {
            printf("Child process terminated abnormally\n");
            return EXIT_FAILURE;
        }
    }
}
