#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "loaders/loader.hpp"
#include "plugin.hpp"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include "ctl-parser.hpp"
#include "ctl-scanner.hpp"

namespace po = boost::program_options;

auto parse_query(const std::string& s) -> std::vector<ctl::syntax_tree_t> {
    /*
     * This function is taken from the ctl-expr project example code
     * https://github.com/sillydan1/ctl-expr/blob/main/src/main.cpp
     */
    std::istringstream iss{s};
    ctl::ast_factory factory{};
    ctl::multi_query_builder builder{};
    ctl::scanner scn{iss, std::cerr, &factory};
    ctl::parser_args pargs{&scn, &factory, &builder};
    ctl::parser p{pargs};
    if(p.parse() != 0)
      return {};
    return builder.build().queries;
}

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

// Adds variable values to the varMap for StoreInstructions
void storeVarValue(const MiniMC::Model::Instruction& instr, std::unordered_map<std::string, std::vector<std::string>> &varMap){
    if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
        auto content = get<12>(instr.getContent());
        auto name = content.variableName;
        if (varMap.find(name) == varMap.end()) {
          varMap[name] = std::vector<std::string>();
        }
        if (content.storee->isConstant()) {
          auto value = std::dynamic_pointer_cast<MiniMC::Model::Constant>(content.storee);

          // NuSMV does not support i64, but does allow bitvectors of arbitrary size(words) so only i32 and i16 is supported at the moment.
          if (value->isInteger()) {
            std::size_t valueSize = value->getSize();

            if (valueSize == 4) { // We have an integer
              auto storeeValue = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV32>>(value)->getValue();
              varMap[name].push_back(std::to_string(storeeValue));
            } else if (valueSize == 2) { // We have a short;
              auto storeeValue = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV16>>(value)->getValue();
              varMap[name].push_back(std::to_string(storeeValue));
            }

          } else if (value->isBool()) {
            if (varMap[name].empty()) {
              varMap[name].emplace_back("boolean");
            }
          }
        }
    }
}

void writeToFile(const MiniMC::Model::Program program, std::string fileName) {
    // Construct SMV string
    std::string smv = "";
    smv += "MODULE main\n";
    std::string varString;
    std::unordered_map<std::string, std::vector<std::string>> varMap;
    std::unordered_map<std::string, std::string> initMap;
    std::unordered_map<std::string, std::vector<std::string>> nextMap;
    //First basic block of main is always init state
    initMap["locations"] = "ASSIGN init(locations) := main-bb0;\n";

    //Handle globals
    for (const auto& instr : program.getInitialiser()) {
        storeVarValue(instr, varMap);
    }

    for (auto var: varMap) {
        initMap[var.first] = "ASSIGN init(" + var.first + ") := " + var.second[0] + ";\n";
    }


    std::string locations = "";
    for (const auto& func : program.getFunctions()) {
        std::string funcName = func->getSymbol().getName();

         // Make module states
        for (auto loc : func->getCFA().getLocations()) {
          std::string locName = std::to_string(loc->getID());

          std::string currentLocation = funcName+"-bb"+locName;
          locations += currentLocation + ", ";

          if (loc->nbIncomingEdges() > 0){
            for (auto edge : loc->getIncomingEdges()) {
              // Iterate through instructions on each edge
              for (auto instr: edge->getInstructions()){
                storeVarValue(instr, varMap);
              }
            }
          } else {

          }
          // Next Facit:


        }
        // Remove trailing comma and write as VAR string.

    }
    locations = locations.substr(0, locations.size()-2);
    smv += "VAR locations: {" + locations + "};\n";

    // Add variables to module string from varMap
    for (const auto& var : varMap) {
        varString = "";
        std::string varName = var.first;
        std::string varValues = "";
        for (auto value : var.second) {
          varValues += value + ", ";
        }
        varValues = varValues.substr(0, varValues.size()-2);
        varString += "VAR " + varName + " : {" + varValues + "};\n";
        smv += varString;
    }
    //Add init values
      for (const auto& init : initMap) {
              smv += init.second;
      }



    // Create file and write to it, if created wipe contents before writing.
    std::ofstream output_file(fileName);
    output_file << smv << std::endl;
    output_file.close();
}

namespace {

  struct LocalOptions {
    std::string spec{""};
    int keep_smv{0};
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

    po::options_description desc("CTL options");
    desc.add_options()("ctl.spec", po::value<std::string>()->default_value("")->notifier(setSpec), "Set an inital CTL spec");
    desc.add_options()("ctl.keep-smv", po::value<int>()->default_value(0)->notifier(setKeepSMV), "Keep the SMV file after analysis (0 = no, 1 = yes)");
    od.add(desc);
  }

};

MiniMC::Host::ExitCodes ctl_main(MiniMC::Model::Controller& ctrl, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Support::Messager messager{};
  auto& prg = ctrl.getProgram();
  messager.message("CTL analysis initialised");

  /*
   * HERE THE FILE WILL BE WRITTEN. FOR NOW WE WILL HARDCODE A FILE NAME TO USE
   * */

  std::string filename = "output_file.smv";
  writeToFile(*prg, filename);
  // check if file exists
  
  std::ifstream infile(filename);

  if (infile.good()) {
    messager.message("Found SMV file");
    infile.close();
  }



  std::ofstream outfile;
  outfile.open(filename, std::ios_base::app);
  try {
    if (opts.spec != "") {
      // if (parse_query(opts.spec).size() == 0) {
      //   messager.message("!!! Invalid CTL spec !!!");
      //   return MiniMC::Host::ExitCodes::RuntimeError;
      // }
      outfile << std::endl << "CTLSPEC " << std::endl << opts.spec << std::endl;
    } else {
      // Enter interactive mode for CTL
      messager.message("No CTL spec provided. Entering interactive mode");
      messager.message("To exit interactive mode and continue with analysis, enter an empty string");
      messager.message("Enter CTL spec: ");
      std::string spec;
      std::string buffer;
      while (std::getline(std::cin, buffer)) {

        if (buffer == "") {
          break;
        }
        // if (parse_query(buffer).size() == 0) {
        //   messager.message("!!! Invalid CTL spec !!!");
        //   messager.message("CTL spec was: " + buffer);
        //   messager.message("Enter CTL spec: ");
        //   continue;
        // }
        spec += "CTLSPEC\n";
        spec += buffer;
        spec += "\n";
        messager.message("Enter CTL spec: ");
      }
      outfile << std::endl << spec << std::endl;
    }
  } catch (std::exception& e) {
    messager.message(e.what());
    outfile.close();
    
    if (opts.keep_smv == 0) std::remove(filename.c_str());

    return MiniMC::Host::ExitCodes::RuntimeError;
  }
  outfile.close();
  std::string output = exec_cmd("nusmv output_file.smv");

  if (opts.keep_smv == 0) std::remove(filename.c_str());

  messager.message(output);
  
  if (output.find("Parse Error") != std::string::npos) {
    messager.message("!!! Parse Error !!!");
    return MiniMC::Host::ExitCodes::RuntimeError;
  } 
  return MiniMC::Host::ExitCodes::AllGood;
}

static CommandRegistrar ctl_reg("ctl", ctl_main, "CTL analysis", addOptions);
