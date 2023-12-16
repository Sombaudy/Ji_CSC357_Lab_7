#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define PORT 2828

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      close(nfd);
      return;
   }

   while ((num = getline(&line, &size, network)) >= 0)
   {
      printf("received from client: %s", line);

      char copy[100];
      strcpy(copy, line);
      printf("copy: %s", copy);

      if(send(nfd, copy, sizeof(copy), 0) != sizeof(copy)) {
         perror("send error");
         close(nfd);
         return;
      }
   }

   free(line);
   fclose(network);
}

void handle_child() {
   pid_t pid;
   int status;
   while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
      printf("Child process %d terminated with status %d\n", pid, WEXITSTATUS(status));
   }
}

void run_service(int fd) {
   struct sigaction sa;
   sa.sa_handler = handle_child;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
   if (sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      exit(EXIT_FAILURE);
   }

   while (1) {
      int nfd = accept_connection(fd);
      if (nfd != -1) {
         pid_t pid = fork();
         if (pid == 0) {  // Child process
            printf("Connection established\n");
            handle_request(nfd);
            printf("Connection closed\n");
            close(nfd);
            exit(EXIT_SUCCESS);
         } else if (pid > 0) {  // Parent process
            close(nfd);  // Close client socket in the parent process
         } else {  // Fork error
            perror("fork");
            exit(EXIT_FAILURE);
         }
      }
   }
}

int main(void)
{
   int fd = create_service(PORT);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", PORT);
   run_service(fd);
   close(fd);

   return 0;
}
