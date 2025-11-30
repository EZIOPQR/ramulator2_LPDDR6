#include <functional>

#include "base/utils.h"
#include "frontend/frontend.h"
#include "translation/translation.h"
#include "frontend/impl/processor/simpleO3/trace_rw.h"


namespace Ramulator {

class Simple_rw_processor final : public IFrontEnd, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IFrontEnd, Simple_rw_processor, "Simple_rw_processor", "Simple_rw_processor")

  private:
    ITranslation*  m_translation;

    IMemorySystem* m_memory_system;
    size_t m_num_expected_insts = 0;
    bool reached_expected_num_insts = false;
    int s_insts_retired = 0;
    int m_cur_inst;
    int m_cur_bubble;
    Trace_rw *m_trace;

    std::string serialization_filename;
  

  public:
    void init() override {
      m_clock_ratio = param<uint>("clock_ratio").required();
      
      // Core params
      std::vector<std::string> trace_list = param<std::vector<std::string>>("traces").desc("A list of traces.").required();
      m_trace = new Trace_rw(trace_list[0]);
      m_cur_inst = 0;
      m_cur_bubble = 0;

      // Simulation parameters
      m_num_expected_insts = param<int>("num_expected_insts").desc("Number of instructions that the frontend should execute.").required();

      // Create address translation module
      m_translation = create_child_ifce<ITranslation>();


      m_logger = Logging::create_logger("SimpleO3");

      // Register the stats
      register_stat(m_num_expected_insts).name("num_expected_insts");
    }

    void tick() override {
      m_clk++;

      if(m_clk % 10000000 == 0) {
        m_logger->info("Processor Heartbeat {} cycles.", m_clk);
      }

      m_clk++;

      if (s_insts_retired >= m_num_expected_insts) {
        reached_expected_num_insts = true;
        return;
      }

      // First, issue the non-memory instructions
      int num_inserted_insts = 0;
      if (m_trace -> m_trace[m_cur_inst].bubble_count > m_cur_bubble) {
        m_cur_bubble++;
        return;
      }

      Request req(0, 0);

      if(m_trace->m_trace[m_cur_inst].rw == 0){
        req.addr = m_trace->m_trace[m_cur_inst].addr;
        req.type_id = Request::Type::Read;
      }else if(m_trace->m_trace[m_cur_inst].rw == 1){
        req.addr = m_trace->m_trace[m_cur_inst].addr;
        req.type_id = Request::Type::Write;
      }else{
        throw ConfigurationError("Trace has invalid rw type {}!", m_trace->m_trace[m_cur_inst].rw);
      }

      if(m_memory_system->send(req)){
        s_insts_retired++;
        m_cur_inst = (m_cur_inst + 1) % m_trace->m_trace_length;
        m_cur_bubble = 0;
      }else{
        return;
      }
    }

    void receive(Request& req) {

    };

    bool is_finished() override {
      return reached_expected_num_insts;
    }

    void connect_memory_system(IMemorySystem* memory_system) override {
      m_memory_system = memory_system;
    };

    int get_num_cores() override {
      return 1;
    };
};

}        // namespace Ramulator