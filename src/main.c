#include <stdio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
  pid_t child = -1;
  int status = -1, len = 0;
  char *target = NULL;
  char **child_argv = NULL;
  char buff[1024];
  struct user_regs_struct regs;

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

  while(1) {
    // entering the syscall
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status)) break;

    ptrace(PTRACE_GETREGS, child, 0, &regs);
    len = snprintf(buff, 1024, "Entering syscall %llu\n", regs.orig_rax);
    write(2, buff, len);

    // exiting the syscall
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status)) break;

    ptrace(PTRACE_GETREGS, child, 0, &regs);
    len = snprintf(buff, 1024, "Exited previous syscall with code %lld\n", (long long)regs.rax);
    write(2, buff, len);
  }

  if(WIFEXITED(status)) {
    printf("Child process exited with status %d\n", WEXITSTATUS(status));
  } else if(WIFSIGNALED(status)) {
    printf("Child process was terminated by signal %d\n", WTERMSIG(status));
  }

  return 0;
}
