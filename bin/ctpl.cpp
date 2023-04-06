#include "cpa/interface.hpp"
#include "host/host.hpp"
#include "model/controller.hpp"
#include "model/location.hpp"
#include "plugin.hpp"
#include "support/feedback.hpp"

#include <boost/program_options.hpp>
#include <fstream>

namespace po = boost::program_options;

namespace {

  void addOptions(po::options_description& op){

  };
}

void writeToFile(MiniMC::Model::Program& program) {
  // Construct SMV string
  std::string smv;
  std::string moduleString = "MODULE main\n";
  std::string varString = "";
  std::unordered_map<std::string, std::vector<std::string>> varMap;
  // Add variables
  for (auto instr : program.getInitialiser()) {

    if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
      auto content = get<12>(instr.getContent());
      auto name = content.variableName;
      if (varMap.find(name) == varMap.end()) {
        varMap[name] = std::vector<std::string>();
      }
      if (content.storee->isConstant()) {
        //auto value = std::static_pointer_cast<MiniMC::Model::TConstant>(content.storee);
        auto value = content.storee->string_repr();
        varMap[name].push_back(value);
      }
    }
  }

  for (auto func : program.getFunctions()) {
    std::string funcName = func->getSymbol().getName();

    std::string locations = "";
    // Make module states
    for (auto loc : func->getCFA().getLocations()) {
      std::string locName = loc->getInfo().getName();
      locations += locName;
    }
    // Add module locations
    moduleString += "VAR " + funcName + " : {" + locations + "};\n";

    // Write moduleString to output_file.smv
  }

  // Add variables to module string from int map
  for (auto var : varMap) {
    varString = "";
    std::string varName = var.first;
    std::string varValues = "";
    for (auto value : var.second) {
      varValues += value + ", ";
    }
    varString += "VAR " + varName + " : {" + varValues + "};\n";
    moduleString += varString;
  }
  // Create file and write to it, if created wipe contents before writing.
  std::ofstream output_file("output_file.smv");
  output_file << smv << std::endl;
  output_file.close();
}

MiniMC::Host::ExitCodes ctpl_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Model::Program& prgm = *controller.getProgram();

  // Debugging breakpoint for inspecting the program repr.
  auto main = prgm.getFunction("main");
  std::vector<std::shared_ptr<MiniMC::Model::Function>> functions = prgm.getFunctions();

  writeToFile(prgm);

  // Call NuSMV binary to check the generated SMV file

  return MiniMC::Host::ExitCodes::AllGood;
}

static CommandRegistrar ctpl_reg("ctpl", ctpl_main, "Checking CT(P)L specification", addOptions);
