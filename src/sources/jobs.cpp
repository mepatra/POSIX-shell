#include "jobs.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void take_terminal(){
  tcsetpgrp(STDIN_FILENO, getpgrp());
  tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_tmodes);
}

static std::vector<Job>::iterator find_job(const std::vector<std::string> &args){
  if(jobs.empty()) return jobs.end();
  if(args.size() < 2) return jobs.end() - 1;
  std::string s = args[1];
  if(!s.empty() && s[0] == '%') s.erase(0, 1);
  int id = atoi(s.c_str());
  for(auto it = jobs.begin(); it != jobs.end(); ++it){
    if(it->id == id) return it;
  }
  return jobs.end();
}

bool reap_jobs(){
  bool reported = false;
  for(auto it = jobs.begin(); it != jobs.end();){
    if(!it->stopped){
      for(auto p = it->pids.begin(); p != it->pids.end();){
        int status;
        pid_t res = waitpid(*p, &status, WNOHANG | WUNTRACED);
        if(res == 0){
          ++p;
        }
        else if(res > 0 && WIFSTOPPED(status)){
          if(!it->stopped){
            it -> stopped = true;
            std::cout << "\n[" << it->id << "] suspended (tty input) " << it->command << "\n";
            reported = true;
          }
          ++p;
        }
        else{
          p = it->pids.erase(p);
        }
      }
      if(it->pids.empty()){
        std::cout << " \n[" << it->id << "] - done     " << it->command << "\n" << std::endl;
        reported = true;
        it = jobs.erase(it);
        continue;
      }
    }
    ++it;
  }
  return reported;
}

int alloc_job_id() {
  for (int id = 1;; ++id) {
    bool used = false;
    for (const auto& j : jobs)
      if (j.id == id) { used = true; break; }
    if (!used) return id;
  }
}

void cmd_bg(const std::vector<std::string> &args){
  auto it = find_job(args);
  if(it == jobs.end()){std::cerr << "bg: no such jobs \n"; last_exit_status =1 ; return;}
  it->stopped = false;
  kill(-it->pgid, SIGCONT);
  std::cout << "[" << it->id << "] continued " << it->command << "\n";
  last_exit_status = 0; 
}

void cmd_fg(const std::vector<std::string> &args){
  auto it = find_job(args);
  if(it == jobs.end()){std::cerr << "fg: no such job \n"; last_exit_status = 1; return;}

  Job j = *it;
  jobs.erase(it);

  std::cout << "[" << j.id << "] continued " << j.command << "\n";
  tcsetpgrp(STDIN_FILENO, j.pgid);
  for (pid_t p : j.pids) kill(p, SIGCONT);
  kill(-j.pgid, SIGCONT);

  int status = 0;
  for(pid_t p: j.pids){
    waitpid(p, &status, WUNTRACED);
    if(WIFSTOPPED(status)) break;
  }
  take_terminal();

  if(WIFSTOPPED(status)){
    j.stopped = true;
    jobs.push_back(j);
    last_exit_status = 128 + WSTOPSIG(status);
    std::cout << "\nsuspended " << j.command << "\n";
  }
  else if(WIFEXITED(status)) last_exit_status = WEXITSTATUS(status);
  else if(WIFSIGNALED(status)) last_exit_status = 128 + WTERMSIG(status);
}

void cmd_jobs(){
  std::vector<Job> sorted = jobs;
  std::sort(sorted.begin(), sorted.end(),
            [](const Job &a, const Job &b){ return a.id < b.id; });
  for(const auto &j : sorted){
    std::cout << "[" << j.id << "] "
              << (j.stopped ? "suspended " : "running ")
              << j.command << "\n";
  }
}
