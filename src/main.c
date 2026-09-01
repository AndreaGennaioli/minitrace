#include <stdio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include "syscalls.h"

void print_syscall(struct user_regs_struct *regs);

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
    print_syscall(&regs);

    // exiting the syscall
    ptrace(PTRACE_SYSCALL, child, 0, 0);
    waitpid(child, &status, 0);
    if(WIFEXITED(status) || WIFSIGNALED(status)) {
      write(2, "\n", 1);
      break;
    }

    ptrace(PTRACE_GETREGS, child, 0, &regs);
    len = snprintf(buff, sizeof(buff), " = %lld\n", (long long)regs.rax);
    write(2, buff, len);
  }

  if(WIFEXITED(status)) {
    printf("Child process exited with status %d\n", WEXITSTATUS(status));
  } else if(WIFSIGNALED(status)) {
    printf("Child process was terminated by signal %d\n", WTERMSIG(status));
  }

  return 0;
}

void print_syscall(struct user_regs_struct *regs) {
  const char *syscall_name = NULL;
  char buff[1024];
  int off = 0;

  if(regs->orig_rax < syscall_names_count)
    syscall_name = syscall_names[regs->orig_rax];

  if(syscall_name == NULL) {
    off += snprintf(buff, sizeof(buff), "<syscall-%llu>", regs->orig_rax);
  } else {
    off += snprintf(buff, sizeof(buff), "%s", syscall_name);
  }

  // currently printing all 6 parameters. Should only print effectly used parameters.
  off += snprintf(buff + off, sizeof(buff) - off, "(0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx, 0x%llx)", regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9);

  write(2, buff, off);
}
