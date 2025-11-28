#include "dram/dram.h"
#include "dram/lambdas.h"
#include <bitset>

namespace Ramulator {

class LPDDR6 : public IDRAM, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IDRAM, LPDDR6, "LPDDR6", "LPDDR6 Device Model")

  public:
    // column的定义修改，现在改为page size
    inline static const std::map<std::string, Organization> org_presets = {
      //   name           density   DQ   Ch Ra Bg Ba   Ro     Co
      {"LPDDR6_2Gb_x24",  {2<<10,   12, {1, 1, 4, 4, 1<<13, 1<<11}}},
      {"LPDDR6_4Gb_x24",  {4<<10,   12, {1, 1, 4, 4, 1<<14, 1<<11}}},
      {"LPDDR6_8Gb_x24",  {8<<10,   12, {1, 1, 4, 4, 1<<15, 1<<11}}},
      {"LPDDR6_16Gb_x24", {16<<10,  12, {1, 1, 4, 4, 1<<16, 1<<11}}},
      {"LPDDR6_32Gb_x24", {32<<10,  12, {1, 1, 4, 4, 1<<17, 1<<11}}},
    };

    // nCCD_L: BL/n_max, nCCD_S: BL/n_min
    inline static const std::map<std::string, std::vector<int>> timing_presets = {
      //   name         rate   nBL  nCL  nWCKPST   nRCD  nRPab  nRPpb   nRAS  nRC   nWR  nRTP nCWL nCCD_S nCCD_L nRRD nWTRS nWTRL nFAW  nPPD  nRFCab nRFCpb nREFI nPBR2PBR nPBR2ACT nCS,  tCK_ps
      {"LPDDR6_6400",  {6400,  2,   20,     7,      15,    17,   15,     34,   30,   28,   4,  11,    2,     4,    4,    5,    10,   16,  2,   -1,      -1,   -1,   -1,        -1,    2,   1250}},
    };


  /************************************************
   *                Organization
   ***********************************************/   
    const int m_internal_prefetch_size = 24;

    inline static constexpr ImplDef m_levels = {
      "channel", "rank", "bankgroup", "bank", "row", "column",    
    };


  /************************************************
   *             Requests & Commands
   ***********************************************/
    inline static constexpr ImplDef m_commands = {
      "NOP",
      "ACT-1",  "ACT-2",
      "PRE",    "PREA",
      "RD24",   "WR24",   "RD24A",   "WR24A",
      "REFab",  "REFpb",
      "RFMab",  "RFMpb",
    };

    inline static const ImplLUT m_command_scopes = LUT (
      m_commands, m_levels, {
        {"NOP", "channel"},
        {"ACT-1", "row"},    {"ACT-2",  "row"},
        {"PRE",   "bank"},   {"PREA",   "rank"},
        {"RD24",  "column"}, {"WR24",   "column"}, {"RD24A", "column"}, {"WR24A", "column"},
        {"REFab", "rank"},   {"REFpb",  "rank"},
        {"RFMab", "rank"},   {"RFMpb",  "rank"},
      }
    );

    // 代表命令的持续时间
    inline static constexpr ImplDef m_nCK = {
      "1CK", "2CK"
    }; 
    inline static const ImplLUT m_command_nCK = LUT(
      m_commands, m_nCK, {
        {"NOP", "2CK"},
        {"ACT-1", "2CK"},    {"ACT-2",  "2CK"},
        {"PRE",   "2CK"},   {"PREA",   "2CK"},
        {"RD24",  "2CK"}, {"WR24",   "2CK"}, {"RD24A", "2CK"}, {"WR24A", "2CK"},
        {"REFab", "2CK"},   {"REFpb",  "2CK"},
        {"RFMab", "2CK"},   {"RFMpb",  "2CK"},
      }
    );

    inline static const ImplLUT m_command_meta = LUT<DRAMCommandMeta> (
      m_commands, {
                // open?   close?   access?  refresh?
        {"NOP",    {false,  false,   false,   false}},
        {"ACT-1",  {false,  false,   false,   false}},
        {"ACT-2",  {true,   false,   false,   false}},
        {"PRE",    {false,  true,    false,   false}},
        {"PREA",   {false,  true,    false,   false}},
        {"RD24",   {false,  false,   true,    false}},
        {"WR24",   {false,  false,   true,    false}},
        {"RD24A",  {false,  true,    true,    false}},
        {"WR24A",  {false,  true,    true,    false}},
        {"REFab",  {false,  false,   false,   true }},
        {"REFpb",  {false,  false,   false,   true }},
        {"RFMab",  {false,  false,   false,   true }},
        {"RFMpb",  {false,  false,   false,   true }},
      }
    );

    inline static constexpr ImplDef m_requests = {
      "read", "write", "all-bank-refresh", "open-row", "close-row"
    };

    inline static const ImplLUT m_request_translations = LUT (
      m_requests, m_commands, {
        {"read", "RD24"}, {"write", "WR24"}, 
        {"all-bank-refresh", "REFab"}, {"open-row", "ACT-1"}, {"close-row", "PRE"}
      }
    );

   
  /************************************************
   *                   Timing
   ***********************************************/
    // nWCKPST=RD(tWCKPST/tCK)
    inline static constexpr ImplDef m_timings = {
      "rate", 
      "nBL16", "nCL", "nWCKPST", "nRCD", "nRPab", "nRPpb", "nRAS", "nRC", "nWR", "nRTP", "nCWL",
      "nCCD_S", "nCCD_L",
      "nRRD",
      "nWTRS", "nWTRL",
      "nFAW",
      "nPPD",
      "nRFCab", "nRFCpb","nREFI",
      "nPBR2PBR", "nPBR2ACT",
      "nCS",
      "tCK_ps"
    };


  /************************************************
   *                 Node States
   ***********************************************/
    inline static constexpr ImplDef m_states = {
    //    ACT-1       ACT-2
       "Pre-Opened", "Opened", "Closed", "PowerUp", "N/A", "Refreshing"
    };

    inline static const ImplLUT m_init_states = LUT (
      m_levels, m_states, {
        {"channel",   "N/A"}, 
        {"rank",      "PowerUp"},
        {"bankgroup", "N/A"},
        {"bank",      "Closed"},
        {"row",       "Closed"},
        {"column",    "N/A"},
      }
    );
  
  private:
    // log
    struct{
      FILE* file = nullptr;
    } vcdLogger;

  public:
    void initVCDLogger(){
      FILE *tmpfs;
      // 写入cmd对照表
      tmpfs = fopen("trace/cmd_trans.txt", "w");
      for(int i = 0; i < m_commands.size(); ++i){
        fprintf(tmpfs, "%s %s\n", std::bitset<7>(i).to_string().c_str(), m_commands(i).data());
      }
      fprintf(tmpfs, "%s NOP\n", std::bitset<7>(m_commands.size()).to_string().c_str());
      fclose(tmpfs);

      // 写入bank状态对照表
      tmpfs = fopen("trace/bank_status_trans.txt", "w");
      for(int i = 0; i < m_states.size(); ++i){
        fprintf(tmpfs, "%s %s\n", std::bitset<4>(i).to_string().c_str(), m_states(i).data());
      }
      fclose(tmpfs);

      vcdLogger.file = fopen("trace/trace.vcd", "w");

      fprintf(vcdLogger.file, "$timescale 1ps $end\n");
      {
        fprintf(vcdLogger.file, "$scope module ramulator $end\n");
        {
          fprintf(vcdLogger.file, "$scope module cmd $end\n");
          fprintf(vcdLogger.file, "$var wire 64 cycle cycle $end\n"); // 当前周期
          fprintf(vcdLogger.file, "$var wire 7 cmd cmd $end\n"); // 当前命令
          fprintf(vcdLogger.file, "$var wire 64 addr addr $end\n"); // 当前命令对应的地址
          fprintf(vcdLogger.file, "$var wire 2 WCKSync WCKSync $end\n"); // WCK同步状态
          fprintf(vcdLogger.file, "$upscope $end\n");
        }

        // bank状态相关
        {
          fprintf(vcdLogger.file, "$scope module bank_status $end\n");
          for (int i = 0; i < 4; i++) {
            fprintf(vcdLogger.file, "$scope module bg%02d $end\n", i);
            for(int j = 0; j < 4; j++) {
              fprintf(vcdLogger.file, "$scope module ba%02d $end\n", j);
              fprintf(vcdLogger.file, "$var wire 4 bank_status_%02d_%02d bank_status_%02d_%02d $end\n", i, j, i, j); // bank状态
              fprintf(vcdLogger.file, "$upscope $end\n");
            }
            fprintf(vcdLogger.file, "$upscope $end\n");
          }
          fprintf(vcdLogger.file, "$upscope $end\n");
        }
        fprintf(vcdLogger.file, "$upscope $end\n");
      }

      fprintf(vcdLogger.file, "$enddefinitions $end\n");

      fprintf(vcdLogger.file, "#0\n");
    }

    void VCDLogCycle(){
      fprintf(vcdLogger.file, "#%lu\n", m_clk * 1250);
    }


  public:
    struct Node : public DRAMNodeBase<LPDDR6> {
      Node(LPDDR6* dram, Node* parent, int level, int id) : DRAMNodeBase<LPDDR6>(dram, parent, level, id) {};
    };
    std::vector<Node*> m_channels;
    
    FuncMatrix<ActionFunc_t<Node>>  m_actions;
    FuncMatrix<PreqFunc_t<Node>>    m_preqs;
    FuncMatrix<RowhitFunc_t<Node>>  m_rowhits;
    FuncMatrix<RowopenFunc_t<Node>> m_rowopens;

    State_t last_m_states[4][4];
  
  // 相关状态定义
  public:
    Clk_t m_final_synced_cycle = -1; // Extra CAS Sync command needed for RD/WR after this cycle

    Clk_t m_cur_cmd_countdown = 0;    // Countdown for the current command's duration
    int m_cur_cmd = 0; // 记录当前正在执行的指令，在最后一个周期实际生效
    AddrVec_t m_cur_addr_vec; // 当前正在执行的指令对应的地址向量 


  public:
    void tick() override {
      m_clk++;

      // log 当前周期
      VCDLogCycle();
      fprintf(vcdLogger.file, "b%s cycle\n", std::bitset<64>(m_clk).to_string().c_str());
      // log WCKSync状态
      if(m_clk <= m_final_synced_cycle){
        fprintf(vcdLogger.file, "b11 WCKSync\n");
      } else {
        fprintf(vcdLogger.file, "b00 WCKSync\n");
      }

      for(int bg = 0; bg < 4; ++bg){
        for(int b = 0; b < 4; ++b){
          auto curState = m_channels[0]->m_child_nodes[0]->m_child_nodes[bg]->m_child_nodes[b]->m_state;
          auto lastState = last_m_states[bg][b];
          if(curState != lastState){
            m_logger->info("At clk {}, BankGroup {} Bank {} : {} -> {}", m_clk, bg, b, m_states(lastState), m_states(curState));
            last_m_states[bg][b] = curState;
            fprintf(vcdLogger.file, "b%s bank_status_%02d_%02d\n", std::bitset<4>(curState).to_string().c_str(), bg, b);
          }
        }
      } 

      // 处理当前命令
      handle_cur_command();
    };
    
    ~LPDDR6() override {
      if(vcdLogger.file){
        fclose(vcdLogger.file);
      }
    };

    void init() override {
      RAMULATOR_DECLARE_SPECS();
      set_organization();
      set_timing_vals();

      set_actions();
      set_preqs();
      set_rowhits();
      set_rowopens();
      
      create_nodes();

      m_logger = Logging::create_logger("LPDDR6");
      initVCDLogger();

      m_cur_cmd = m_commands["NOP"];
      m_cur_cmd_countdown = 1;
      m_cur_addr_vec = AddrVec_t(m_levels.size(), 0);
    };

    // issue_command作为接口接收命令，launch_command执行实际的命令逻辑
    void issue_command(int command, const AddrVec_t& addr_vec) override {
      m_cur_cmd = command;
      m_cur_addr_vec = addr_vec;
      // 在dram_controller中先step dram，再step controller，所以当前clk周期已经到了
      m_cur_cmd_countdown = (m_command_nCK[command] == m_nCK["1CK"]) ? 0 : 
                            (m_command_nCK[command] == m_nCK["2CK"]) ? 1 : 
                            1;
      
      // VCD log 当前命令
      fprintf(vcdLogger.file, "b%s cmd\n", std::bitset<7>(m_cur_cmd).to_string().c_str());
      unsigned long long addr = 0;
      addr = m_cur_addr_vec[m_levels["channel"]];
      addr = (addr * m_organization.count[m_levels["rank"]]) + m_cur_addr_vec[m_levels["rank"]];
      addr = (addr * m_organization.count[m_levels["bankgroup"]]) + m_cur_addr_vec[m_levels["bankgroup"]];
      addr = (addr * m_organization.count[m_levels["bank"]]) + m_cur_addr_vec[m_levels["bank"]];
      addr = (addr * m_organization.count[m_levels["row"]]) + m_cur_addr_vec[m_levels["row"]];
      addr = (addr * m_organization.count[m_levels["column"]]) + m_cur_addr_vec[m_levels["column"]];
      fprintf(vcdLogger.file, "b%s addr\n", std::bitset<64>(addr).to_string().c_str());
    };

    void handle_cur_command(){
      if(m_cur_cmd_countdown > 0){
        if(m_cur_cmd_countdown == 1){
          launch_command(m_cur_cmd, m_cur_addr_vec);
        }
        m_cur_cmd_countdown--;
        // VCD log 当前命令
        fprintf(vcdLogger.file, "b%s cmd\n", std::bitset<7>(m_cur_cmd).to_string().c_str());
        unsigned long long addr = 0;
        addr = m_cur_addr_vec[m_levels["channel"]];
        addr = (addr * m_organization.count[m_levels["rank"]]) + m_cur_addr_vec[m_levels["rank"]];
        addr = (addr * m_organization.count[m_levels["bankgroup"]]) + m_cur_addr_vec[m_levels["bankgroup"]];
        addr = (addr * m_organization.count[m_levels["bank"]]) + m_cur_addr_vec[m_levels["bank"]];
        addr = (addr * m_organization.count[m_levels["row"]]) + m_cur_addr_vec[m_levels["row"]];
        addr = (addr * m_organization.count[m_levels["column"]]) + m_cur_addr_vec[m_levels["column"]];
        fprintf(vcdLogger.file, "b%s addr\n", std::bitset<64>(addr).to_string().c_str());
      }
    }

    void launch_command(int command, const AddrVec_t& addr_vec) {
      int channel_id = addr_vec[m_levels["channel"]];
      m_channels[channel_id]->update_timing(command, addr_vec, m_clk);
      m_channels[channel_id]->update_states(command, addr_vec, m_clk);
    };

    int get_preq_command(int command, const AddrVec_t& addr_vec) override {
      int channel_id = addr_vec[m_levels["channel"]];
      return m_channels[channel_id]->get_preq_command(command, addr_vec, m_clk);
    };

    bool check_ready(int command, const AddrVec_t& addr_vec) override {
      // LPDDR6只允许在even cycle发命令
      if(m_clk % 2 != 0){
        return false;
      }

      // 由于even cycle的限制，理论上不会进入这个分支
      if(m_cur_cmd_countdown > 0){
        return false;
      }

      int channel_id = addr_vec[m_levels["channel"]];
      return m_channels[channel_id]->check_ready(command, addr_vec, m_clk);
    };

    bool check_rowbuffer_hit(int command, const AddrVec_t& addr_vec) override {
      int channel_id = addr_vec[m_levels["channel"]];
      return m_channels[channel_id]->check_rowbuffer_hit(command, addr_vec, m_clk);
    };
    
    bool check_node_open(int command, const AddrVec_t& addr_vec) override {
      int channel_id = addr_vec[m_levels["channel"]];
      return m_channels[channel_id]->check_node_open(command, addr_vec, m_clk);
    };

  private:
    void set_organization() {
      // Channel width
      m_channel_width = param_group("org").param<int>("channel_width").default_val(12);

      // Organization
      m_organization.count.resize(m_levels.size(), -1);

      // Load organization preset if provided
      if (auto preset_name = param_group("org").param<std::string>("preset").optional()) {
        if (org_presets.count(*preset_name) > 0) {
          m_organization = org_presets.at(*preset_name);
        } else {
          throw ConfigurationError("Unrecognized organization preset \"{}\" in {}!", *preset_name, get_name());
        }
      }

      // Override the preset with any provided settings
      if (auto dq = param_group("org").param<int>("dq").optional()) {
        m_organization.dq = *dq;
      }

      for (int i = 0; i < m_levels.size(); i++){
        auto level_name = m_levels(i);
        if (auto sz = param_group("org").param<int>(level_name).optional()) {
          m_organization.count[i] = *sz;
        }
      }

      if (auto density = param_group("org").param<int>("density").optional()) {
        m_organization.density = *density;
      }

      // Sanity check: is the calculated chip density the same as the provided one?
      size_t _density = size_t(m_organization.count[m_levels["bankgroup"]]) *
                        size_t(m_organization.count[m_levels["bank"]]) *
                        size_t(m_organization.count[m_levels["row"]]) *
                        size_t(m_organization.count[m_levels["column"]]) * 8;
      _density >>= 20;
      if (m_organization.density != _density) {
        throw ConfigurationError(
            "Calculated {} chip density {} Mb does not equal the provided density {} Mb!", 
            get_name(),
            _density, 
            m_organization.density
        );
      }

    };

    void set_timing_vals() {
      m_timing_vals.resize(m_timings.size(), -1);

      // Load timing preset if provided
      bool preset_provided = false;
      if (auto preset_name = param_group("timing").param<std::string>("preset").optional()) {
        if (timing_presets.count(*preset_name) > 0) {
          m_timing_vals = timing_presets.at(*preset_name);
          preset_provided = true;
        } else {
          throw ConfigurationError("Unrecognized timing preset \"{}\" in {}!", *preset_name, get_name());
        }
      }

      // Check for rate (in MT/s), and if provided, calculate and set tCK (in picosecond)
      if (auto dq = param_group("timing").param<int>("rate").optional()) {
        if (preset_provided) {
          throw ConfigurationError("Cannot change the transfer rate of {} when using a speed preset !", get_name());
        }
        m_timing_vals("rate") = *dq;
      }
      int tCK_ps = 1E6 / (m_timing_vals("rate") / 2);
      m_timing_vals("tCK_ps") = tCK_ps;

      // Load the organization specific timings
      int dq_id = [](int dq) -> int {
        switch (dq) {
          case 16: return 0;
          default: return -1;
        }
      }(m_organization.dq);

      int rate_id = [](int rate) -> int {
        switch (rate) {
          case 6400:  return 0;
          default:    return -1;
        }
      }(m_timing_vals("rate"));


      // Refresh timings
      // tRFC table (unit is nanosecond!)
      constexpr int tRFCab_TABLE[4] = {
      //  2Gb   4Gb   8Gb  16Gb
          130,  180,  210,  280, 
      };

      constexpr int tRFCpb_TABLE[4] = {
      //  2Gb   4Gb   8Gb  16Gb
          60,   90,   120,  140, 
      };

      constexpr int tPBR2PBR_TABLE[4] = {
      //  2Gb   4Gb   8Gb  16Gb
          60,   90,   90,  90, 
      };

      constexpr int tPBR2ACT_TABLE[4] = {
      //  2Gb   4Gb   8Gb  16Gb
          8,    8,    8,   8, 
      };

      // tREFI(base) table (unit is nanosecond!)
      constexpr int tREFI_BASE = 3906;
      int density_id = [](int density_Mb) -> int { 
        switch (density_Mb) {
          case 2048:  return 0;
          case 4096:  return 1;
          case 8192:  return 2;
          case 16384: return 3;
          default:    return -1;
        }
      }(m_organization.density);

      m_timing_vals("nRFCab")    = JEDEC_rounding(tRFCab_TABLE[density_id], tCK_ps);
      m_timing_vals("nRFCpb")    = JEDEC_rounding(tRFCpb_TABLE[density_id], tCK_ps);
      m_timing_vals("nPBR2PBR")  = JEDEC_rounding(tPBR2PBR_TABLE[density_id], tCK_ps);
      m_timing_vals("nPBR2ACT")  = JEDEC_rounding(tPBR2ACT_TABLE[density_id], tCK_ps);
      m_timing_vals("nREFI") = JEDEC_rounding(tREFI_BASE, tCK_ps);

      // Overwrite timing parameters with any user-provided value
      // Rate and tCK should not be overwritten
      for (int i = 1; i < m_timings.size() - 1; i++) {
        auto timing_name = std::string(m_timings(i));

        if (auto provided_timing = param_group("timing").param<int>(timing_name).optional()) {
          // Check if the user specifies in the number of cycles (e.g., nRCD)
          m_timing_vals(i) = *provided_timing;
        } else if (auto provided_timing = param_group("timing").param<float>(timing_name.replace(0, 1, "t")).optional()) {
          // Check if the user specifies in nanoseconds (e.g., tRCD)
          m_timing_vals(i) = JEDEC_rounding(*provided_timing, tCK_ps);
        }
      }

      // Check if there is any uninitialized timings
      for (int i = 0; i < m_timing_vals.size(); i++) {
        if (m_timing_vals(i) == -1) {
          throw ConfigurationError("In \"{}\", timing {} is not specified!", get_name(), m_timings(i));
        }
      }      

      // Set read latency
      m_read_latency = m_timing_vals("nCL") + m_timing_vals("nBL16");

      // Populate the timing constraints
      #define V(timing) (m_timing_vals(timing))
      populate_timingcons(this, {
          /*** Channel ***/ 
          // CAS <-> CAS
          /// Data bus occupancy
          {.level = "channel", .preceding = {"RD24", "RD24A"}, .following = {"RD24", "RD24A"}, .latency = V("nBL16")},
          {.level = "channel", .preceding = {"WR24", "WR24A"}, .following = {"WR24", "WR24A"}, .latency = V("nBL16")},

          /*** Rank (or different BankGroup) ***/ 
          // CAS <-> CAS
          {.level = "rank", .preceding = {"RD24", "RD24A"}, .following = {"RD24", "RD24A"}, .latency = V("nCCD_S")},
          {.level = "rank", .preceding = {"WR24", "WR24A"}, .following = {"WR24", "WR24A"}, .latency = V("nCCD_S")},
          /// RD <-> WR, Minimum Read to Write, Assuming tWPRE = 1 tCK                          
          {.level = "rank", .preceding = {"RD24", "RD24A"}, .following = {"WR24", "WR24A"}, .latency = V("nCL") + V("nCCD_S") + 2 - V("nCWL")},
          /// WR <-> RD, Minimum Read after Write
          {.level = "rank", .preceding = {"WR24", "WR24A"}, .following = {"RD24", "RD24A"}, .latency = V("nCWL") + V("nBL16") + V("nWTRS")},
          /// CAS <-> CAS between sibling ranks, nCS (rank switching) is needed for new DQS
          {.level = "rank", .preceding = {"RD24", "RD24A"}, .following = {"RD24", "RD24A", "WR24", "WR24A"}, .latency = V("nBL16") + V("nCS"), .is_sibling = true},
          {.level = "rank", .preceding = {"WR24", "WR24A"}, .following = {"RD24", "RD24A"}, .latency = V("nCL")  + V("nBL16") + V("nCS") - V("nCWL"), .is_sibling = true},
          /// CAS <-> PREab
          {.level = "rank", .preceding = {"RD24"}, .following = {"PREA"}, .latency = V("nRTP") + V("nCCD_S")}, // 延时加上BL/n_min
          {.level = "rank", .preceding = {"WR24"}, .following = {"PREA"}, .latency = V("nCWL") + V("nCCD_S") + 1 + V("nWR")},          
          /// RAS <-> RAS
          {.level = "rank", .preceding = {"ACT-1"}, .following = {"ACT-1", "REFpb"}, .latency = V("nRRD")},          
          {.level = "rank", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nFAW"), .window = 4},          
          {.level = "rank", .preceding = {"ACT-1"}, .following = {"PREA"}, .latency = V("nRAS")},          
          {.level = "rank", .preceding = {"PREA"}, .following = {"ACT-1"}, .latency = V("nRPab")},          
          /// RAS <-> REF
          {.level = "rank", .preceding = {"ACT-1"}, .following = {"REFab"}, .latency = V("nRC")},          
          {.level = "rank", .preceding = {"PRE"}, .following = {"REFab"}, .latency = V("nRPpb")},          
          {.level = "rank", .preceding = {"PREA"}, .following = {"REFab"}, .latency = V("nRPab")},          
          {.level = "rank", .preceding = {"RD24A"}, .following = {"REFab"}, .latency = V("nRPpb") + V("nRTP") + V("nCCD_S")}, // 延时加上BL/n_min          
          {.level = "rank", .preceding = {"WR24A"}, .following = {"REFab"}, .latency = V("nCWL") + V("nCCD_S") + 1 + V("nWR") + V("nRPpb")},          
          {.level = "rank", .preceding = {"REFab"}, .following = {"REFab", "ACT-1", "REFpb"}, .latency = V("nRFCab")},          
          {.level = "rank", .preceding = {"ACT-1"},   .following = {"REFpb"}, .latency = V("nPBR2ACT")},  
          {.level = "rank", .preceding = {"REFpb"}, .following = {"REFpb"}, .latency = V("nPBR2PBR")},  

          /*** Same Bank Group ***/ 
          /// CAS <-> CAS
          {.level = "bankgroup", .preceding = {"RD24", "RD24A"}, .following = {"RD24", "RD24A"}, .latency = V("nCCD_L")},          
          {.level = "bankgroup", .preceding = {"WR24", "WR24A"}, .following = {"WR24", "WR24A"}, .latency = V("nCCD_L")},          
          {.level = "bankgroup", .preceding = {"WR24", "WR24A"}, .following = {"RD24", "RD24A"}, .latency = V("nCWL") + V("nBL16") + V("nWTRL")},
          /// RAS <-> RAS
          {.level = "bankgroup", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nRRD")},  

          /*** Bank ***/ 
          {.level = "bank", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nRC")},  
          {.level = "bank", .preceding = {"ACT-2"}, .following = {"RD24", "RD24A", "WR24", "WR24A"}, .latency = V("nRCD")},  
          {.level = "bank", .preceding = {"ACT-2"}, .following = {"PRE"}, .latency = V("nRAS")},  
          {.level = "bank", .preceding = {"PRE"}, .following = {"ACT-1"}, .latency = V("nRPpb")},  
          {.level = "bank", .preceding = {"RD24"},  .following = {"PRE"}, .latency = V("nRTP") + V("nCCD_S")}, // 延时加上BL/n_min
          {.level = "bank", .preceding = {"WR24"},  .following = {"PRE"}, .latency = V("nCWL") + V("nCCD_S") + 1 + V("nWR")},  
          {.level = "bank", .preceding = {"RD24A"}, .following = {"ACT-1"}, .latency = V("nRTP") + V("nRPpb") + V("nCCD_S")}, // 延时加上BL/n_min
          {.level = "bank", .preceding = {"WR24A"}, .following = {"ACT-1"}, .latency = V("nCWL") + V("nCCD_S") + 1 + V("nWR") + V("nRPpb")},  
        }
      );
      #undef V

    };

    void set_actions() {
      m_actions.resize(m_levels.size(), std::vector<ActionFunc_t<Node>>(m_commands.size()));

      // Rank Actions
      m_actions[m_levels["rank"]][m_commands["PREA"]] = Lambdas::Action::Rank::PREab<LPDDR6>;
      m_actions[m_levels["rank"]][m_commands["RD24"]] = [this] (Node* node, int cmd, int target_id, Clk_t clk) {
        m_final_synced_cycle = clk + m_timing_vals("nCL") + m_timing_vals("nBL16") + m_timing_vals("nWCKPST"); 
      };
      m_actions[m_levels["rank"]][m_commands["WR24"]] = [this] (Node* node, int cmd, int target_id, Clk_t clk) {
        m_final_synced_cycle = clk + m_timing_vals("nCWL") + m_timing_vals("nBL16") + m_timing_vals("nWCKPST"); 
      };
      // Bank actions
      m_actions[m_levels["bank"]][m_commands["ACT-1"]] = [] (Node* node, int cmd, int target_id, Clk_t clk) {
        node->m_state = m_states["Pre-Opened"];
        node->m_row_state[target_id] = m_states["Pre-Opened"];
      };
      m_actions[m_levels["bank"]][m_commands["ACT-2"]] = Lambdas::Action::Bank::ACT<LPDDR6>;
      m_actions[m_levels["bank"]][m_commands["PRE"]]   = Lambdas::Action::Bank::PRE<LPDDR6>;

      // TODO: read/write + PRE 
      // m_actions[m_levels["bank"]][m_commands["RD24A"]] = Lambdas::Action::Bank::PRE<LPDDR6>;
      // m_actions[m_levels["bank"]][m_commands["WR24A"]] = Lambdas::Action::Bank::PRE<LPDDR6>;
    };

    void set_preqs() {
      m_preqs.resize(m_levels.size(), std::vector<PreqFunc_t<Node>>(m_commands.size()));

      // Rank Preqs
      m_preqs[m_levels["rank"]][m_commands["REFab"]] = Lambdas::Preq::Rank::RequireAllBanksClosed<LPDDR6>;
      m_preqs[m_levels["rank"]][m_commands["RFMab"]] = Lambdas::Preq::Rank::RequireAllBanksClosed<LPDDR6>;

      m_preqs[m_levels["rank"]][m_commands["REFpb"]] = [this] (Node* node, int cmd, const AddrVec_t& addr_vec, Clk_t clk) {

        for (auto bg : node->m_child_nodes) {
          for (auto bank : bg->m_child_nodes) {
            int num_banks_per_bg = m_organization.count[m_levels["bank"]];
            int flat_bankid = bank->m_node_id + bg->m_node_id * num_banks_per_bg;
            if (flat_bankid == addr_vec[LPDDR6::m_levels["bank"]] || flat_bankid == addr_vec[LPDDR6::m_levels["bank"]] + 8) {
              switch (node->m_state) {
                case m_states["Pre-Opened"]: return m_commands["PRE"];
                case m_states["Opened"]: return m_commands["PRE"];
              }
            }
          }
        }

        return cmd;
      };
      
      m_preqs[m_levels["rank"]][m_commands["RFMpb"]] = m_preqs[m_levels["rank"]][m_commands["REFpb"]];

      // Bank Preqs
      m_preqs[m_levels["bank"]][m_commands["RD24"]] = [this] (Node* node, int cmd, const AddrVec_t& addr_vec, Clk_t clk) {
        switch (node->m_state) {
          case m_states["Closed"]: return m_commands["ACT-1"];
          case m_states["Pre-Opened"]: return m_commands["ACT-2"];
          case m_states["Opened"]: {
            if (node->m_row_state.find(addr_vec[m_levels["row"]]) != node->m_row_state.end()) {
              Node* rank = node->m_parent_node->m_parent_node;
              return cmd;
            } else {
              return m_commands["PRE"];
            }
          }    
          default: {
            spdlog::error("[Preq::Bank] Invalid bank state for an RD/WR command!");
            std::exit(-1);      
          } 
        }
      };
      m_preqs[m_levels["bank"]][m_commands["WR24"]] = [this] (Node* node, int cmd, const AddrVec_t& addr_vec, Clk_t clk) {
        switch (node->m_state) {
          case m_states["Closed"]: return m_commands["ACT-1"];
          case m_states["Pre-Opened"]: return m_commands["ACT-2"];
          case m_states["Opened"]: {
            if (node->m_row_state.find(addr_vec[m_levels["row"]]) != node->m_row_state.end()) {
              Node* rank = node->m_parent_node->m_parent_node;
              return cmd;
            } else {
              return m_commands["PRE"];
            }
          }    
          default: {
            spdlog::error("[Preq::Bank] Invalid bank state for an RD/WR command!");
            std::exit(-1);      
          } 
        }
      };
    };

    void set_rowhits() {
      m_rowhits.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      m_rowhits[m_levels["bank"]][m_commands["RD24"]] = Lambdas::RowHit::Bank::RDWR<LPDDR6>;
      m_rowhits[m_levels["bank"]][m_commands["WR24"]] = Lambdas::RowHit::Bank::RDWR<LPDDR6>;
    }


    void set_rowopens() {
      m_rowopens.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      m_rowopens[m_levels["bank"]][m_commands["RD24"]] = Lambdas::RowOpen::Bank::RDWR<LPDDR6>;
      m_rowopens[m_levels["bank"]][m_commands["WR24"]] = Lambdas::RowOpen::Bank::RDWR<LPDDR6>;
    }


    void create_nodes() {
      int num_channels = m_organization.count[m_levels["channel"]];
      for (int i = 0; i < num_channels; i++) {
        Node* channel = new Node(this, nullptr, 0, i);
        m_channels.push_back(channel);
      }
    };
};


}        // namespace Ramulator
