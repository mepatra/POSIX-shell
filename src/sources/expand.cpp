#include "expand.hpp"
#include "lexer.hpp"
#include "text.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <glob.h>

static bool has_wildcard(const std::string &tok){
  int tokSize = tok.size();
  for(int i = 0; i < tokSize; i++){
    if(tok[i] == MASK){i++; continue;}
    if(tok[i] == '*' || tok[i] == '?') return true;
  }
  return false;
}

bool try_assign_var(const std::string &token){
  size_t eq = token.find('=');
  if(eq == std::string::npos || eq == 0) return false;
  std::string name = token.substr(0, eq);
  if(isdigit((unsigned char)name[0])) return false;
  for (char c: name)
    if(!isalnum((unsigned char)c) && c != '_') return false;
  std::string value = token.substr(eq + 1);
  value.erase(std::remove(value.begin(), value.end(), MASK), value.end());
  shell_vars[name] = value;
  if(getenv(name.c_str())) setenv(name.c_str(), value.c_str(), 1);
  return true;
}

bool valid_var_name(const std::string &name){
  if(name.empty() || isdigit((unsigned char)name[0])) return false;
  for(const char c: name){
    if(!isalnum((unsigned char)c) && c != '_') return false; 
  }
  return true;
}

std::string expand_env(const std::string &tok){
  std::string out;
  int tokSize = tok.size();
  for(int i = 0; i < tokSize; i++){
    if(tok[i] == MASK && i + 1 < tokSize){
      out += tok[i];
      out += tok[++i];
      continue;
    }
    if(tok[i] == '$' && i + 1 < tokSize){
      if(tok[i + 1] == '?'){
        out += std::to_string(last_exit_status);
        i++;
        continue;
      }
      size_t j = i + 1;
      std::string name;
      while(j < tok.size() && (isalnum((unsigned char)tok[j]) || tok[j] == '_'))
        name += tok[j++];
      if(!name.empty()){
        auto it = shell_vars.find(name);
        if(it != shell_vars.end()) out += mask_special(it->second);
        else if(const char *v = getenv(name.c_str())) out += mask_special(v);
        i = j - 1;
        continue;
      }
    }
    out += tok[i];
  }
  return out;
}

std::vector<std::string> expand_glob(const std::string &tok){
  if(!has_wildcard(tok)) return {tok};

  glob_t g;
  int res = glob(tok.c_str(), GLOB_NOCHECK, nullptr, &g);
  std::vector<std::string> matches;
  if(res == 0){
    for(size_t i = 0; i < g.gl_pathc; i++){
      std::string m;
      for(const char*p = g.gl_pathv[i]; *p; p++){
        if(strchr("?|<>;&*$", *p)) m += MASK;
        m += *p;
      }
      matches.push_back(m);
    }
  }else{
      matches.push_back(tok);
  }
  globfree(&g);
  return matches;
}

std::string expand_tilde(const std::string &tok){
  if(tok.empty() || tok[0] != '~') return tok;
  if(tok.size() > 1 && tok[1] != '/') return tok;
  const char *home = getenv("HOME");
  if(!home) return tok;
  return mask_special(home) + tok.substr(1);
}

void expand_alias(std::vector<std::string> &a){
  if(a.empty()) return;
  auto it = aliases.find(a[0]);
  if(it == aliases.end()) return;

  std::vector<std::string> out = tokenizer(it->second);
  out.insert(out.end(), a.begin() + 1, a.end());
  a = out;
}
