#include "lexer.hpp"

std::vector<std::string> tokenizer (const std::string &command){
  std::vector<std::string> args;
  std::string curr;
  bool in_token = false;
  char q = '\0';

  auto flush = [&]{if(in_token){args.push_back(curr); curr.clear(); in_token = false; }};
  auto lit = [&](char c, const char *special){
    if(strchr(special, c)) curr += MASK;
    curr += c;
  };

  for(size_t i = 0; i < command.size(); i++){
    char c = command[i];

    if(q){
      if(q == '"' && c == '\\' && i + 1 < command.size() && strchr("\"\\$", command[i + 1])){
          lit(command[++i], "$?|<>;&*~");
          continue;
          }
          if(q == c) q = '\0';
          else{lit(c, q == '\'' ? "?|<>;&*$~" : "?|<>;&*~");}
      continue;
    }

    if(c == '\'' || c == '"'){q = c; in_token = true; continue;}
   
    if(c == '\\' && i + 1 < command.size()){
      lit(command[++i], "?|<>;*&$~");
      in_token = true;
      continue;
    }

    if(std::isspace((unsigned char)c)){flush(); continue;}

    if(c == '|' || c == '&' || c == '<' || c == '>' || c == ';'){
      if(c == '>' && in_token && curr == "2"){
        curr.clear();
        in_token = false;
        std::string op = "2>";
          if(i +  1 < command.size() && command[i + 1] == '>'){
            op = "2>>";
            i++;
          }
          else if(i + 2 < command.size() && command[i + 1] == '&' && command[i + 2] == '1'){
            op = "2>&1";
            i += 2;
          }
          args.push_back(op);
          continue;
      }
      flush();
      std::string op(1, c);
      if(i + 1 < command.size() && command[i + 1] == c && (c == '|' || c == '&' || c == '>')){
        op += c; i++;
      }
      args.push_back(op);
      continue;
    }
    curr += c;
    in_token = true;
  }
  if(q){
    std::cerr << "syntax error: undetermined quote \n";
    last_exit_status = 1;
    return{};
  }
  flush();
  return args;
}