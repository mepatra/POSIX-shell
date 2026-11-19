#pragma once
#include "shell.hpp"

enum class BuiltinResult { NotBuiltin, Handled, Exit };
BuiltinResult run_builtin(std::vector<std::string>& a);