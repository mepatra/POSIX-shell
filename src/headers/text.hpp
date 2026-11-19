#pragma once
#include "shell.hpp"

std::string mask_special(const std::string& s);
void unmask(std::vector<std::string>& v);
void unmask_str(std::string& s);
void unmask_redirect(Redirect& r);