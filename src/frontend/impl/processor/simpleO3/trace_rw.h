#ifndef     RAMULATOR_FRONTEND_PROCESSOR_TRACE_H
#define     RAMULATOR_FRONTEND_PROCESSOR_TRACE_H

#include <vector>

#include "base/type.h"

namespace Ramulator {

class Trace_rw {
  private:
    struct Trace {
      int bubble_count = 0;
      Addr_t addr = -1;
      int rw = -1;   // 0 for load, 1 for store
    };
    

    
    size_t m_curr_trace_idx = 0;

    int m_load_type;
    int m_store_type;


  public:
    Trace_rw(std::string file_path_str);
    std::vector<Trace> m_trace;
    size_t m_trace_length = 0;
};

}        // namespace Ramulator


#endif   // RAMULATOR_FRONTEND_PROCESSOR_TRACE_H