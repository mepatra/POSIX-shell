#include "repl.hpp"
#include "jobs.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <readline/history.h>
#include <readline/readline.h>

namespace fs = std::filesystem;

static std::string fetch_path(){
  std::error_code ec;
  fs::path curr_path = fs::current_path(ec);
  if(ec) return "?";

  const char *home = getenv("HOME");
  if(home){
    fs::path rel = fs::relative(curr_path, fs::path(home), ec);
    if(!ec){
      std::string relSTr = rel.string();
      if(relSTr.substr(0, 2) != ".."){
        return relSTr == "." ? " ~ " : " ~/" + relSTr;
      }
    }
  }
  fs::path chopped_path;
  short int count = 0;
  for(const auto &tok: curr_path){
    chopped_path /= tok;
    count++;
    if(count == 2) break;
  }
  return chopped_path.string();
}

static std::string fetch_branch(){
  std::error_code ec;
  fs::path curr_path = fs::current_path(ec);
  if(ec) return "";
  while(true){
    fs::path gitPath = curr_path / ".git" / "HEAD";
    if(fs::exists(gitPath, ec)){
      std::ifstream head_file (gitPath);
      if(head_file.is_open()){
        std::string line;
        if(std::getline(head_file,line)){
          while(!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')){
            line.pop_back();
          }
          if(line.rfind("ref: refs/heads/", 0) == 0){
            return line.substr(16);
          }
          return line.substr(0, 7);
        }
      }
    }
    if(curr_path == curr_path.root_path() || curr_path.empty())
      break;
    curr_path = curr_path.parent_path();
  }
  return "";
}

static std::string make_prompt(){
  std::string p = "\n" + fetch_path();
  std::string b = fetch_branch();
  if(!b.empty()) p += " on " + b;
  return p += " > ";
}

static char *command_generator(const char *text, int state){
  static std::vector<std::string> matches;
  static int idx = 0;

  if(state == 0){
    idx = 0;
    matches.clear();
    std::string prefix(text);

    for(const char *b : {"cd","jobs","fg","bg","exit","quit","alias","unalias","export"})
      if(strncmp(b, text, prefix.size()) == 0) matches.push_back(b);

    const char *path = getenv("PATH");
    if(path){
      std::stringstream ss(path);
      std::string dir;
      while(std::getline(ss, dir, ':')){
        std::error_code ec;
        fs::directory_iterator it(dir, ec);
        for(; it != fs::directory_iterator(); it.increment(ec)){
          if(ec) break;
          std::string name = it->path().filename().string();
          if(name.compare(0, prefix.size(), prefix) == 0) matches.push_back(name);
        }
      }
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
  }
  int n = matches.size();
  if(idx < n) return strdup(matches[idx++].c_str());
  return nullptr;
}

bool read_command(std::string &out){
  char *line = readline(make_prompt().c_str());
  if(!line){std::cout << "\n"; return false;}
  out = line;
  if(!out.empty()) add_history(line);
  free(line);
  return true;
}

char **shell_completion(const char *text, int start, int end){
  if(start == 0) return rl_completion_matches(text, command_generator);
  return nullptr;
}

int check_jobs(){
  if(reap_jobs()){
    std::cout << std::flush;
    rl_on_new_line();
    rl_redisplay();
  }
  return 0;
}
