#include "shell.hpp"

int last_exit_status = 0;
int next_job_id = 1;
int stop_sig = 0;
std::unordered_map<std::string, std::string> shell_vars;
std::unordered_map<std::string, std::string> aliases;
std::vector<Job> jobs;
struct termios shell_tmodes;