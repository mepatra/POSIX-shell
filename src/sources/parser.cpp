#include "parser.hpp"
#include <iostream>

Redirect parse_redirect(std::vector<std::string> &args){
  Redirect r;
  std::vector<std::string> clean;
  int n = args.size();

  for(int i = 0; i < n; i++){
    const std::string &t = args[i];
    bool is_redir = (t == ">" || t == ">>" || t == "<" || t == "2>" || t == "2>>");
    if(is_redir){
      if(i + 1 >= n){
        std::cerr << "expected filename after " << t << "\n";
        last_exit_status = 1;
        return r;
      }
      const std::string &f = args[++i];
      if(t == ">")       { r.out_file = f; r.append = false; }
      else if(t == ">>") { r.out_file = f; r.append = true; }
      else if(t == "<")  { r.in_file = f; }
      else if(t == "2>") { r.err_file = f; r.err_append = false; }
      else               { r.err_file = f; r.err_append = true; }
    }
    else if(t == "2>&1") r.err_to_out = true;
    else clean.push_back(t);
  }
  args = clean;
  return r;
}

std::vector<ChainSegment> parse_chaining(const std::vector<std::string> &args){
  std::vector<ChainSegment> seg;
  std::vector<std::string> curr;
  for(const auto &tok: args){
    if(tok == "||" || tok == "&&" || tok == ";"){
      if(curr.empty()){
        std::cerr << "syntax error near " << tok << "\n";
        last_exit_status = 1;
        return{};
      }
      seg.push_back({curr, tok});
      curr.clear();
    }else{
      curr.push_back(tok);
    }
  }
  if(!curr.empty()) seg.push_back({curr, ""});
  else if(!seg.empty() && seg.back().op != ";"){
    std::cerr << "syntax error missing command after " << seg.back().op << "\n";
    last_exit_status = 1;
    return{};
  }
  return seg;
}

std::vector<std::vector<std::string>> parse_pipeline(std::vector<std::string> &args){
  std::vector<std::vector<std::string>> commands;
  std::vector<std::string> current;
  int args_size = args.size();
  for(int i = 0; i < args_size; i++){
    if(args[i] == "|"){
      if(current.empty()){
        std::cerr << "expected arg after | \n";
        last_exit_status = 1;
        return{};
      }
      commands.push_back(current);
      current.clear();
      continue;
    }
    else{
      current.push_back(args[i]);
    }
  }
  if(!current.empty()) commands.push_back(current);
  else if(!commands.empty()){
    std::cerr << "syntax error: expected command after | \n";
    last_exit_status = 1;
    return {};
  }
  return commands;
}
