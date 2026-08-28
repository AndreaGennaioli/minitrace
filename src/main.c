#include <stdio.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
  pid_t child = -1;
  int status = -1;
  char *target = NULL;
  char **child_argv = NULL;

  if(argc < 2){
    fprintf(stderr, "Usage: %s BINARY [ARGS...]\n", *argv);
    return 1;
  }

  target = argv[1];
  child_argv = &argv[1];

  child = fork();

  if(child == -1) {
    perror("fork()");
    return 1;
  } else if(child == 0) {
    // long ptrace(op, pid_t pid, addr, data)
    if(ptrace(PTRACE_TRACEME, 0, 0, 0) == -1) {
      perror("ptrace()");
      return 1;
    }
    execve(target, child_argv, envp);

    perror("execve()");
    return 1;
  }

  // kernel sends SIGTRAP to child at the end of execve, so there is no
  // timing issue
  waitpid(child, &status, 0);

  if(WIFSTOPPED(status)) {
    // should output signal 5 (SIGTRAP)
    printf("Child process stopped with signal %d\n", WSTOPSIG(status));
  }

  return 0;
}
