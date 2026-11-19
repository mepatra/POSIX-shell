#include "builtins.hpp"
#include "expand.hpp"
#include "jobs.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

static void cmd_alias(const std::vector<std::string> &args){
  if(args.size() < 2){
    for(const auto &k: aliases){
      std::cout << "alias " << k.first << "=" << k.second << "\n";
    }
  }
  for(size_t i = 1; i < args.size(); i++){
    std::string s = args[i];
    s.erase(std::remove(s.begin(), s.end(), MASK), s.end());
    size_t eq = s.find('=');
    if(eq == std::string::npos){
      auto it = aliases.find(s);
      if(it != aliases.end())
        std::cout << "alias: " << it->first << "=" << it->second << "\n";
      else{
        std::cerr << "alias: " << s << "not found \n";
        last_exit_status = 1;
        return;
      }
      continue;
    }
    if(eq == 0){std::cerr << "alias: invalid name \n"; last_exit_status = 1; return;}
    aliases[s.substr(0,eq)] = s.substr(eq + 1);
  }
  last_exit_status = 0;
}

static void cmd_cd(std::vector<std::string> &args){
  std::error_code ec;
  std::string old = fs::current_path(ec).string();

  std::string target;
  if(args.size() < 2){
    const char *h = getenv("HOME");
    if(!h){std::cerr << "cd: home not set \n"; last_exit_status = 1; return; }
    target = h;
  }
  else if(args[1] == "-"){
    const char *o = getenv("OLDPWD");
    if(!o){std::cerr << "cd: old pwd not set \n"; last_exit_status = 1; return;}
    target = o;
    std::cout << target << "\n";
  }
  else{
    target = args[1];
  }
  fs::current_path(target, ec);
  if(ec){
    std::cerr << "cd: " << target << ": " << ec.message() << "\n";
    last_exit_status = 1;
    return;
  }
  setenv("OLDPWD", old.c_str(), 1);
  setenv("PWD",fs::current_path(ec).string().c_str(), 1);
  last_exit_status = 0;
}

static void cmd_export(const std::vector<std::string> &args){
  last_exit_status = 0;
  int argsSize = args.size();
  for(int i = 1; i < argsSize; i++){
    std::string s = args[i];
    s.erase(std::remove(s.begin(), s.end(), MASK), s.end());
    size_t eq = s.find('=');
    std::string name = (eq == std::string::npos) ? s : s.substr(0,eq);
    if(!valid_var_name(name)){
      std::cerr << "export invalid command name " << name << "\n";
      last_exit_status = 1;
      continue;
    }
    if(eq != std::string::npos){
      shell_vars[name] = s.substr(eq + 1);
    }
    
    auto it = shell_vars.find(name);
    if(it != shell_vars.end())
      setenv(name.c_str(), it->second.c_str(), 1);
  }
}

void cmd_unalias(std::vector<std::string> &a){
  if(a.size() < 2){std::cerr << "usage: unalias name \n"; last_exit_status = 1; return;}
  for(size_t i = 1; i < a.size(); i++) aliases.erase(a[i]);
  last_exit_status = 0;
}

BuiltinResult run_builtin(std::vector<std::string>& a) {
  const std::string& c = a[0];
  if (c == "exit" || c == "quit") return BuiltinResult::Exit;
  if (c == "cd")      { cmd_cd(a);      return BuiltinResult::Handled; }
  if (c == "jobs")    { cmd_jobs(); last_exit_status = 0; return BuiltinResult::Handled; }
  if (c == "fg")      { cmd_fg(a);      return BuiltinResult::Handled; }
  if (c == "bg")      { cmd_bg(a);      return BuiltinResult::Handled; }
  if (c == "alias")   { cmd_alias(a);   return BuiltinResult::Handled; }
  if (c == "unalias") { cmd_unalias(a); return BuiltinResult::Handled; }
  if (c == "export")  { cmd_export(a);  return BuiltinResult::Handled; }
  return BuiltinResult::NotBuiltin;
}
