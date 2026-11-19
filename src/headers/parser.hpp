#pragma once
#include "shell.hpp"

Redirect parse_redirect(std::vector<std::string>& args);
std::vector<ChainSegment> parse_chaining(const std::vector<std::string>& args);
std::vector<std::vector<std::string>> parse_pipeline(std::vector<std::string>& args);
