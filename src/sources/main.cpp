#include "executor.hpp"
#include "repl.hpp"
#include "shell.hpp"   
#include <csignal>
#include <readline/readline.h>
#include <unistd.h>

int main() {
  signal(SIGINT, SIG_IGN);  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN); signal(SIGTTOU, SIG_IGN);
  pid_t shell_pid = getpid();
  setpgid(shell_pid, shell_pid);
  tcsetpgrp(STDIN_FILENO, shell_pid);
  tcgetattr(STDIN_FILENO, &shell_tmodes);

  rl_catch_signals = 0;
  rl_event_hook = check_jobs;
  rl_attempted_completion_function = shell_completion;
  rl_variable_bind("show-all-if-ambiguous", "on");

  std::string command;
  while (read_command(command))
    if (!run_line(command)) break;
  return 0;
}