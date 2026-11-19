#include "executor.hpp"
#include "builtins.hpp"
#include "expand.hpp"
#include "jobs.hpp"    
#include "lexer.hpp"
#include "parser.hpp"
#include "text.hpp"
#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

static void run_redirects(Redirect r){
  if(!r.out_file.empty()){
    int flags = O_WRONLY | O_CREAT | (r.append ? O_APPEND : O_TRUNC);
    int fd = open(r.out_file.c_str(), flags, 0644);
    if(fd < 0){std::cerr << "failed to open \n"; _exit(1);}
    dup2(fd, 1);
    close(fd);
  }
  if(!r.in_file.empty()){
    int fd = open(r.in_file.c_str(), O_RDONLY);
    if(fd < 0){std::cerr << "failed to open \n"; _exit(1);}
    dup2(fd, 0);
    close(fd);
  }
  if(!r.err_file.empty()){
  int flags = O_WRONLY | O_CREAT | (r.err_append ? O_APPEND : O_TRUNC);
  int fd = open(r.err_file.c_str(), flags, 0644);
  if(fd < 0){std::cerr << "failed to open \n"; _exit(1);}
  dup2(fd, 2);
  close(fd);
  }
  else if(r.err_to_out){
  dup2(1, 2);
  }
}

static void run_pipeline(std::vector<std::vector<std::string>> &commands, bool bg){
  int n = commands.size();
  int prev_read = -1;
  int fd[2];

  std::vector<pid_t> pids;
  pid_t pgid = 0;

  std::string label;
  for(int i = 0; i < n; i++){
    for(auto tok: commands[i]){
      tok.erase(std::remove(tok.begin(), tok.end(), MASK), tok.end());
      label += tok + " ";
    }
    if(i < n - 1) label += "| ";
  }
 
  for(int i = 0; i < n; i++){
    pipe(fd);

    pid_t pid = fork();

    if(pid < 0){std::cerr << "fork failed \n"; last_exit_status = 1; return;}

    if(pid == 0){
      setpgid(0,pgid);
      signal(SIGTTOU,SIG_DFL);
      signal(SIGTTIN, SIG_DFL);
      signal(SIGINT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      if(i < n - 1){
        dup2(fd[1], 1);
      }

      if(prev_read != - 1){
        dup2(prev_read, 0);
        close(prev_read);
      }

      close(fd[0]);
      close(fd[1]);

      Redirect r = parse_redirect(commands[i]);
      unmask(commands[i]);
      unmask_redirect(r);
      run_redirects(r);
      if(commands[i].empty()) _exit(0);

      std::vector<char*> argv;
      for(const auto &a: commands[i]){
        argv.push_back(const_cast<char*>(a.c_str()));
      }
      argv.push_back(nullptr);

      execvp(argv[0], argv.data());
      int e = errno;
      std::cerr << argv[0] << ": " << strerror(e) << "\n";
      _exit(e == ENOENT ? 127: 126);
    } else{
      if(pgid == 0) pgid = pid;
      setpgid(pid, pgid);
      pids.push_back(pid);
      if(prev_read != -1)
        close(prev_read);
      close(fd[1]);
      prev_read = fd[0];
    }
  }

  if(prev_read != -1){close(prev_read);}
  if(bg){
    jobs.push_back({pids, pgid, label, false, alloc_job_id()});
    std::cout << "[" << jobs.back().id << "] " << pgid << "\n";
    last_exit_status = 0;
    return;
   }
  tcsetpgrp(STDIN_FILENO, pgid);

  bool stopped = false;
  int stop_status = 0;
  for(size_t i = 0; i < pids.size(); i++){
    int status; 
    waitpid(pids[i], &status, WUNTRACED);
    if(WIFSTOPPED(status)){stopped = true; stop_status = status; break;}
    if(i == pids.size() - 1){
      if(WIFEXITED(status)) last_exit_status = WEXITSTATUS(status);
      else if(WIFSIGNALED(status)) last_exit_status = 128 + WTERMSIG(status);
    }
  }
  take_terminal();
  if(stopped){
    last_exit_status = 128 + WSTOPSIG(stop_status);
    jobs.push_back({pids, pgid, label, true, alloc_job_id()});
    std::cout << "\n[" << jobs.back().id << "] suspended " << label << "\n";
  }
}

static void run_external(std::vector<std::string> &args){
  bool bg = false;
  if(!args.empty() && args.back() == "&"){
    bg = true;
    args.pop_back();
  }
  Redirect r = parse_redirect(args);
  unmask(args); 
  unmask_redirect(r);
  if(args.empty()) return;
  int argSize = args.size();

  std::string commands;
  for(int i = 0; i < argSize; i++){
    commands += args[i] + " ";
  }

  std::vector<char*> argv;

  for(const auto &a: args){
    argv.push_back(const_cast<char*>(a.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = fork();

  if(pid < 0){std::cerr << "fork failed \n"; last_exit_status = 1; return;}

  if(pid == 0){
    setpgid(0,0);
    signal(SIGTTOU,SIG_DFL);
    signal(SIGTTIN,SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    run_redirects(r);
    execvp(argv[0], argv.data());
    int e = errno;
    std::cerr << argv[0] << ": " << strerror(e) << "\n";
    _exit(e == ENOENT ? 127: 126);
  }
  setpgid(pid, pid);
  if(bg){
    jobs.push_back({{pid}, pid , commands, false, alloc_job_id()});
    std::cout << "[" << jobs.back().id << "] " << pid << "\n";
    last_exit_status = 0;
    return;
  }
  tcsetpgrp(STDIN_FILENO, pid);

  int status;
  waitpid(pid, &status, WUNTRACED);

  take_terminal();

  if(WIFSTOPPED(status)){
    last_exit_status = 128 + WSTOPSIG(status);
    jobs.push_back({{pid}, pid, commands, true, alloc_job_id()});
    std::cout << "\n[" << jobs.back().id << "] suspended " << commands << "\n";
  }
  else if(WIFEXITED(status)) last_exit_status = WEXITSTATUS(status);
  else if(WIFSIGNALED(status)) last_exit_status = 128 + WTERMSIG(status);
}

bool run_line(const std::string& command) {
  std::vector<std::string> args = tokenizer(command);
  if (args.empty()) return true;

  std::string prev_op = ";";
  for (auto& seg : parse_chaining(args)) {
    std::string op_before = prev_op;
    prev_op = seg.op;
    if (op_before == "&&" && last_exit_status != 0) continue;
    if (op_before == "||" && last_exit_status == 0) continue;

    std::vector<std::string> a = seg.token;
    expand_alias(a);
    if (a.empty()) continue;
    for (auto& t : a) t = expand_tilde(t);
    for (auto& t : a) t = expand_env(t);
    if (a.size() == 1 && try_assign_var(a[0])) { last_exit_status = 0; continue; }

    std::vector<std::string> expanded;
    for (const auto& t : a) {
      auto m = expand_glob(t);
      expanded.insert(expanded.end(), m.begin(), m.end());
    }
    a = expanded;

    switch (run_builtin(a)) {
      case BuiltinResult::Exit:       return false;
      case BuiltinResult::Handled:    continue;
      case BuiltinResult::NotBuiltin: break;
    }

    std::vector<std::vector<std::string>> commands = parse_pipeline(a);
    if (commands.empty()) continue;

    if (commands.size() < 2) {
      run_external(a);
    } else {
      bool bg = false;
      if (commands.back().back() == "&") {
        commands.back().pop_back();
        bg = true;
        if (commands.back().empty()) {
          std::cerr << "syntax error near &\n";
          continue;
        }
      }
      run_pipeline(commands, bg);
    }
  }
  return true;
}