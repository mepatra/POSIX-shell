#include "text.hpp"
#include <algorithm>
#include <cstring>

std::string mask_special(const std::string &s){
  std::string out;
  for(char c: s){
    if(strchr("?|<>;&*$", c)) out += MASK;
    out += c;
  }
  return out;
}

void unmask(std::vector<std::string> &v){
  for(auto &s: v){s.erase(std::remove(s.begin(), s.end(), MASK), s.end());}
}

void unmask_redirect(Redirect &r){
  unmask_str(r.out_file); unmask_str(r.in_file); unmask_str(r.err_file); 
}

void unmask_str(std::string &s){
  s.erase(std::remove(s.begin(), s.end(), MASK), s.end());
}