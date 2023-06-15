#include "loaders/loader.hpp"
#include "plugin.hpp"
#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex.h>
#include <stdexcept>
#include <string>
#include "smv.hpp"
// Include regex
#include <regex>

#include "cpa/location.hpp"
#ifdef MINIMC_SYMBOLIC
#include "cpa/pathformula.hpp"
#endif
#include "cpa/concrete.hpp"
#include "loaders/loader.hpp"
#include "options.hpp"

namespace po = boost::program_options;

/*
 *  UTILITY FUNCTIONS
 * */

std::string exec_cmd(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

/*
 * END UTILITY FUNCTIONS
 * */

std::vector<ctl_spec> ctl_specs = {
  {.ctl_spec_name={"unsafe_fork", "cwe_400"}, .replace_type=CTLReplaceType::None, .ctl_specs={
      "AG (locations = fork-bb0 -> AX(AF locations = fork-bb0))",
      "AG (locations = fork-bb0 -> AX(EF locations = fork-bb0))",
  }},
  {.ctl_spec_name{"double_free", "cwe_415", "cwe_825", "cwe_1341"}, .replace_type=CTLReplaceType::Register, .ctl_specs={
      "AG ((locations = free-bb0 & %1 = Assigned) -> EX(AF (locations = free-bb0 & %1 = NonDet)))",
  }},
  {.ctl_spec_name={"xor_files"}, .replace_type=CTLReplaceType::Register, .ctl_specs={
      "EG ((locations = fopen-bb0 & %1 != Xor & EX ( locations = fgetc-bb0)) -> AG ( locations = fputc-bb0 & %1 = Unassigned ))",
      "EG ((locations = fopen-bb0 & %1 != Xor ) -> EX(EF(locations = fputc-bb0 & %1 = Xor)))",
  }},
  {.ctl_spec_name={"command_injection"}, .replace_type=CTLReplaceType::Register, .ctl_specs={
      "EG ((locations = strcat-bb0) -> (EX (locations = system-bb0)))",
  }},
  {.ctl_spec_name={"out_of_control", "cwe_400"}, .replace_type=CTLReplaceType::None, .ctl_specs={
       "A[(locations != close-bb0) U (locations = socket-bb0)] -> AX (EF locations = close-bb0)",
  }},
  {.ctl_spec_name={"file_write"}, .replace_type=CTLReplaceType::Register, .ctl_specs={
       "AF(locations = fopen-bb0) -> EX A[(locations != fclose-bb0) U (locations = fwrite-bb0 & %1 = Modified)]",
  }},
  {.ctl_spec_name={"setuid"}, .replace_type=CTLReplaceType::None, .ctl_specs={
       "EG(locations = getuid-bb0 -> EX(EF (locations = setuid-bb0)))"
  }},
};



namespace {

  struct LocalOptions {
    std::string spec{""};
    int keep_smv{0};
    int expect_false{0};
    int debug{0};
  };

  LocalOptions opts;

  void addOptions(po::options_description& od) {

    auto setSpec = [&](const std::string& s) {
      opts.spec = s;
    };
    auto setKeepSMV = [&](const int& i) {
      switch (i) {
        case 0:
          opts.keep_smv = 0;
          break;
        case 1:
          opts.keep_smv = 1;
          break;
      }
    };
    auto setExpectFalse = [&](const int& i) {
      switch (i) {
        case 0:
          opts.expect_false = false;
          break;
        case 1:
          opts.expect_false = true;
          break;
      }
    };

    auto setDebug = [&](const int& i) {
      switch (i) {
        case 0:
          opts.debug = false;
          break;
        case 1:
          opts.debug = true;
          break;
      }
    };

    po::options_description desc("CTL options");
    desc.add_options()("ctl.spec", po::value<std::string>()->default_value("")->notifier(setSpec), "Set an inital CTL spec");
    desc.add_options()("ctl.keep-smv", po::value<int>()->default_value(0)->notifier(setKeepSMV), "Keep the SMV file after analysis (0 = no, 1 = yes)");
    desc.add_options()("ctl.expect-false", po::value<int>()->default_value(0)->notifier(setExpectFalse), "Expect the CTL spec to be false (0 = no, 1 = yes)");
    desc.add_options()("ctl.debug", po::value<int>()->default_value(0)->notifier(setDebug), "Debug instead of running CTL analysis (0 = no, 1 = yes)");
    od.add(desc);
  }
};

MiniMC::Host::ExitCodes ctl_main(MiniMC::Model::Controller& ctrl, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Support::Messager messager{};
  auto prg = ctrl.getProgram();
  SmvSpec spec = generate_smv_spec(prg);

  if (opts.debug) {
    messager.message("Debugging mode enabled");
    messager.message("Amount of locations in model: " + std::to_string(spec.get_locations_size()));
    messager.message("Amount of registers in model: " + std::to_string(spec.get_registers_size()));
    messager.message("Approximate average amount of register states: " + std::to_string(spec.get_approx_register_states()));
  }

  spec.reduce();

  if (opts.debug) {
    messager.message("Amount of locations in model after reduction: " + std::to_string(spec.get_reduced_locations_size()));
  }

  if (opts.debug) {
    spec.print();
  }
  spec.write("smv.smv", opts.spec, ctl_specs);
  std::string cmd = "nusmv smv.smv";
  std::string output = exec_cmd(cmd.c_str());

  std::regex re(".*is false.*");
  std::smatch match;
  bool found = std::regex_search(output, match, re);

  if (!opts.keep_smv) {
    std::remove("smv.smv");
  }

  messager.message("CTL analysis result:\n" + output);

  if (opts.expect_false && !found) {
    messager.message("CTL analysis failed: expected false, got true");
    return MiniMC::Host::ExitCodes::UnexpectedResult;
  } else if (!opts.expect_false && found) {
    messager.message("CTL analysis failed: expected true, got false");
    return MiniMC::Host::ExitCodes::UnexpectedResult;
  } else {
    messager.message("CTL analysis succeeded");
  }


  return MiniMC::Host::ExitCodes::AllGood;
}


static CommandRegistrar ctl_reg("ctl", ctl_main, "CTL analysis", addOptions);
