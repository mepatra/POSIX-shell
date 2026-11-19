#pragma once
#include "shell.hpp"

bool try_assign_var(const std::string& token);
bool valid_var_name(const std::string& name);
std::string expand_tilde(const std::string& tok);
std::string expand_env(const std::string& tok);
std::vector<std::string> expand_glob(const std::string& tok);
void expand_alias(std::vector<std::string>& a);