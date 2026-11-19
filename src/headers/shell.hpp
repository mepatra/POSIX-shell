#pragma once
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <unordered_map>
#include <vector>

const char MASK = '\x01';

struct ChainSegment{
  std::vector<std::string> token;
  std::string op;
};

struct Job{
  std::vector<pid_t> pids;
  pid_t pgid;
  std::string command;
  bool stopped;
  int id;
};

struct Redirect{
  std::string out_file;
  bool append = false;
  std::string in_file;
  std::string err_file;
  bool err_append = false;
  bool err_to_out = false;
};

extern int last_exit_status;
extern int next_job_id;
extern std::unordered_map<std::string, std::string> shell_vars;
extern std::unordered_map<std::string, std::string> aliases;
extern std::vector<Job> jobs;
extern struct termios shell_tmodes;


