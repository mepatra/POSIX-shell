#pragma once
#include <string>

bool read_command(std::string& out);
char** shell_completion(const char* text, int start, int end);
int check_jobs();