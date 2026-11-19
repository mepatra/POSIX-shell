#pragma once
#include "shell.hpp"

bool reap_jobs();
int alloc_job_id();
void cmd_jobs();
void cmd_fg(const std::vector<std::string>& args);
void cmd_bg(const std::vector<std::string>& args);
void take_terminal();