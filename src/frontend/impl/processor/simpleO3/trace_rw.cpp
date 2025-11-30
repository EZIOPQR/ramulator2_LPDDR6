#include <filesystem>
#include <iostream>
#include <fstream>

#include <spdlog/spdlog.h>

#include "base/exception.h"
#include "base/utils.h"
#include "frontend/impl/processor/simpleO3/trace_rw.h"


namespace Ramulator {

namespace fs = std::filesystem;

Trace_rw::Trace_rw(std::string file_path_str) {
  fs::path trace_path(file_path_str);
  if (!fs::exists(trace_path)) {
    throw ConfigurationError("Trace {} does not exist!", file_path_str);
  }

  std::ifstream trace_file(trace_path);
  if (!trace_file.is_open()) {
    throw ConfigurationError("Trace {} cannot be opened!", file_path_str);
  }

  std::string line;
  while (std::getline(trace_file, line)) {
    std::vector<std::string> tokens;
    tokenize(tokens, line, " ");

    int num_tokens = tokens.size();
    if (num_tokens != 3) {
      throw ConfigurationError("Trace {} format invalid!", file_path_str);
    }
    int bubble_count = std::stoi(tokens[0]);
    int rw = std::stoi(tokens[1]);
    Addr_t addr = std::stoll(tokens[2]);

    if(rw == 0 || rw == 1){
      m_trace.push_back({bubble_count, addr, rw});
    }else{
      throw ConfigurationError("Trace {} has invalid rw type {}!", file_path_str, rw);
    }
  }

  trace_file.close();
  m_trace_length = m_trace.size();
}

}        // namespace Ramulator
