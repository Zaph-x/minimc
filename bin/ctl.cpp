#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "loaders/loader.hpp"
#include "plugin.hpp"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <string>
#include <array>
#include <regex.h>
#include "ctl-parser.hpp"
#include "ctl-scanner.hpp"
#define MMCM MiniMC::Model
namespace po = boost::program_options;

struct registerStruct{
  std::string destinationRegister, opcode, content;
};

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

void functionRegisters(const MiniMC::Model::Function_ptr & func, std::vector<registerStruct>& registers){

    for (const auto& location: func->getCFA().getLocations()){
      if (location->nbIncomingEdges() == 1){
        auto instrStream = location->getIncomingEdges()[0]->getInstructions();
        for (auto& instr: instrStream){
          // TACContents index as a variant of InstructionContent is 0
          if (std::variant(instr.getContent()).index() == 0){
            auto content = get<MiniMC::Model::TACContent>(instr.getContent());
            if (content.res->isRegister()){
              auto resReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              auto resName = resReg->getSymbol().getFullName();
              std::ostringstream oss;
              oss << instr.getOpcode();
              auto opCodeString = oss.str();
              auto arguments = content.op1->string_repr() + content.op2->string_repr();
              registerStruct regStruct {resName, opCodeString, arguments};
              registers.push_back(regStruct);
            }
          }
          if(std::variant(instr.getContent()).index()==11){
                  auto content = get<MiniMC::Model::LoadContent>(instr.getContent());
                  auto laodString = "Load";
                  if(content.res->isRegister()){
                    auto loadReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
                    auto resnName = loadReg->getSymbol().getFullName();
                    std::ostringstream ossload;
                    ossload << instr.getOpcode();
                    auto load = content.addr->string_repr();
                    registerStruct loadStruct {resnName, laodString, load};
                    registers.push_back(loadStruct);
                  }
          }
          if(std::variant(instr.getContent()).index()==12){
                  auto content = get<MiniMC::Model::StoreContent>(instr.getContent());
                  auto storeString = "Store";
                  if(content.storee->isRegister()){
                  auto storeReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.addr);
                  auto resnName = storeReg->getSymbol().getFullName();
                  std::ostringstream ossstore;
                  ossstore << instr.getOpcode();
                  auto store = content.addr->string_repr() + content.storee->string_repr() + content.variableName;
                  registerStruct storeStruct {resnName, storeString, store};
                  registers.push_back(storeStruct);
                  }
          }
          // CallContents index as a variant of InstructionContent is 14
          if (std::variant(instr.getContent()).index() == 14){
            std::string resName;
            auto content = get<MiniMC::Model::CallContent>(instr.getContent());
            auto opCodeString = "Call";
            auto params = content.params;
            std::string allParams;
            for (auto par: params){
              allParams += par->string_repr();
            }
            auto funcptr = content.function;
            std::string funcName;
            if (funcptr->isConstant()){
              auto funcConstant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(funcptr);
              auto conSize = funcConstant->getSize();
              if(funcConstant->isPointer() && conSize == 8){
                auto tConstant = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(funcConstant);
                funcName = func->getPrgm().getFunctions()[getFunctionId(tConstant->getValue())]->getSymbol().getName();
                auto breakpointx = "x";
              } else if(funcConstant->isPointer() && conSize == 4){
                //Do 32-bit sized ptr stuff.
              }
            }
            //Hvordan håndteres result registret i et void call.
            if (content.res && content.res->isRegister()) {
              auto resReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              resName = resReg->getSymbol().getFullName();
              auto breakpont = 1+1;
            }

            registerStruct regStruct {resName, opCodeString, allParams};
          }
        }
      }
    }
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
          } else {
            std::cout << "Unsupported constant type " << value->getType()->get_type_name() << std::endl;
            varMap.erase(name);
          }
        }
    }
}


void write_module(std::string module_name, std::string& smv) {
    smv += "MODULE " + module_name + "\n";
}

void setup_variables(const MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::unordered_map<std::string, std::string> &initMap, std::string& smv) {
    // Write variables
    initMap["locations"] = "ASSIGN init(locations) := main-bb0;\n";
    for (const auto& instr : program.getInitialiser()) {
        storeVarValue(instr, varMap);
    }
    for (auto var: varMap) {
      initMap[var.first] = "ASSIGN init(" + var.first + ") := " + var.second[0] + ";\n";
    }
}

void write_locations(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv, std::vector<std::string> &known_locations) {
  std::string locations = "";
    for (const auto& func : program.getFunctions()) {
        std::string funcName = func->getSymbol().getName();

         // Make module states
        known_locations.push_back(funcName+"-bb0");
        locations += funcName+"-bb0, ";
        for (auto loc : func->getCFA().getEdges()) {
          std::string locName = std::to_string(loc->getTo()->getID());

          std::string currentLocation = funcName+"-bb"+locName;
          if (std::find(known_locations.begin(), known_locations.end(), currentLocation) == known_locations.end()) {
            known_locations.push_back(currentLocation);
            locations += currentLocation + ", ";
          }
        }
    }
    for (const auto& func : program.getFunctions()) {
        std::string funcName = func->getSymbol().getName();
        for (auto loc : func->getCFA().getLocations()) {
          if (loc->nbIncomingEdges() > 0){
            for (auto edge : loc->getIncomingEdges()) {
              // Iterate through instructions on each edge
              for (auto instr: edge->getInstructions()){
                storeVarValue(instr, varMap);
              }
            }
          }         }
    }

    locations = locations.substr(0, locations.size()-2);
    smv += "VAR locations: {" + locations + "};\n";
}

void write_variables(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv) {
   for (const auto& var : varMap) {
       std::string varName = var.first;
       std::string varValues = "";
       for (auto value : var.second) {
         varValues += value + ", ";
       }
       varValues = varValues.substr(0, varValues.size()-2);
       smv += "VAR " + varName + " : {" + varValues + "};\n";
   }
}

void write_init(std::unordered_map<std::string, std::string> initMap, std::string &smv) {
  for (const auto& init : initMap) {
      smv += init.second;
    }
}

void store_transitions(MiniMC::Model::Program program, MiniMC::Model::Function_ptr function, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv, std::vector<std::string> &known_locations, std::unordered_map<std::string, std::vector<std::string>> &transition_map) {
  
  for (auto &edge : function->getCFA().getEdges()) {
    std::string from = std::to_string(edge->getFrom()->getID());
    std::string to = std::to_string(edge->getTo()->getID());
    std::string currentLocation = function->getSymbol().getName()+"-bb"+from;
    std::string nextLocation = function->getSymbol().getName()+"-bb"+to;
    std::string next_stored = nextLocation;
    for (auto &instr : edge->getInstructions()) {
      if (instr.getOpcode() == MiniMC::Model::InstructionCode::Call) {
        auto call_instr = get<14>(instr.getContent());
        std::string func_name = call_instr.argument;
        nextLocation = func_name+"-bb0";
        if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
          nextLocation = "error";
        }
        //transition_map[currentLocation].push_back(next_stored);
        auto edge_count = program.getFunction(func_name)->getCFA().getEdges().size();
        transition_map[func_name + "-bb" + std::to_string(edge_count)].push_back(next_stored);
      }
    }
    if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
      nextLocation = "error";
    }
    transition_map[currentLocation].push_back(nextLocation);
  }
}


void write_function_case(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, MiniMC::Model::Function_ptr function, std::string &smv, std::vector<std::string> &known_locations) {
  std::unordered_map<std::string, std::vector<std::string>> transitions;

  store_transitions(program, function, varMap, smv, known_locations, transitions);

  for (auto kv : transitions) {
    std::string transition = "{";
    for (auto next : kv.second) {
      transition += next + ", ";
    }
    transition = transition.substr(0, transition.size()-2);
    transition += "}";
    smv += "    locations = " + kv.first + " : " + transition + ";\n";
  }

//  std::string function_case = "";
//  std::string nextLocation = "";
//  for (auto &edge : function->getCFA().getEdges()) {
//    std::string from = std::to_string(edge->getFrom()->getID());
//    std::string to = std::to_string(edge->getTo()->getID());
//    std::string currentLocation = function->getSymbol().getName()+"-bb"+from;
//    nextLocation = function->getSymbol().getName()+"-bb"+to;
//    std::string next_stored = nextLocation;
//    for (auto &instr : edge->getInstructions()) {
//      if (instr.getOpcode() == MiniMC::Model::InstructionCode::Call) {
//        auto call_instr = get<14>(instr.getContent());
//        std::string func_name = call_instr.argument;
//        nextLocation = func_name+"-bb0";
//        if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
//          nextLocation = "error";
//        }
//        auto edge_count = program.getFunction(func_name)->getCFA().getEdges().size();
//        function_case += "    locations = " + func_name + "-bb" + std::to_string(edge_count) + " : " + next_stored + ";\n";
//      }
//    }
//    if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
//      nextLocation = "error";
//    }
//    function_case += "    locations = " + currentLocation + " : " + nextLocation + ";\n";
//  }
//  function_case += "    locations = " + nextLocation + " : " + nextLocation + ";\n";
//  smv += function_case;
}

void write_assignment_case(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv, std::vector<std::string> &known_locations, std::string var_name) {
    // TODO Iterate vars, assign if var name matches. Fix all next cases
    std::string assignment_case = "";
    for (const auto& func : program.getFunctions()) {
      std::string funcName = func->getSymbol().getName();
      for (auto loc : func->getCFA().getLocations()) {
        if (loc->nbIncomingEdges() > 0){
          for (auto edge : loc->getIncomingEdges()) {
            // Iterate through instructions on each edge
            for (auto instr: edge->getInstructions()){
              if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
                auto assign_instr = get<12>(instr.getContent());
                std::string varName = assign_instr.variableName;
                std::string value;
                if (assign_instr.storee->isConstant()) {
                  auto const_instr = std::dynamic_pointer_cast<MiniMC::Model::Constant>(assign_instr.storee);
                  size_t size = const_instr->getSize();
                  if (size == 1) {
                    value = std::to_string(std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV8>>(assign_instr.storee)->getValue());
                  } else if (size == 2) {
                    value = std::to_string(std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV16>>(assign_instr.storee)->getValue());
                  } else if (size == 4) {
                    value = std::to_string(std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV32>>(assign_instr.storee)->getValue());
                  } else if (size == 8) {
                    value = std::to_string(std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::BV64>>(assign_instr.storee)->getValue());
                  }


                }
                assignment_case += "    (locations = " + funcName + "-bb" + std::to_string(loc->getID()) + ") : " + value + ";\n";
              }
            }
          }
        }
    }
  }
  smv += assignment_case + "    TRUE : " + var_name + ";\n";
}

void write_next(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv, std::vector<std::string> &known_locations) {
  std::unordered_map<std::string, std::string> call_stack_map;
  smv += "ASSIGN next(locations) :=\n";
  smv += "  case\n";
  MiniMC::Model::Function_ptr function = program.getFunctions().back(); // get main
  
  
  for (const auto &func : program.getFunctions()) {
    write_function_case(program, varMap, func, smv, known_locations);
  }
  smv += "    TRUE : locations;\n";
  smv += "  esac;\n";
 
  for (const auto& var : varMap) {
    std::string varName = var.first;
    smv += "ASSIGN next(" + varName + ") :=\n";
    smv += "  case\n";
    write_assignment_case(program, varMap, smv, known_locations, var.first);
    smv += "  esac;\n";
  }
}

void writeToFile(const MiniMC::Model::Program program, std::string fileName) {
    // Construct SMV string
    std::string smv = "";
    std::string varString;
    std::unordered_map<std::string, std::vector<std::string>> nextMap;
    std::unordered_map<std::string, std::string> initMap;
    std::unordered_map<std::string, std::vector<std::string>> varMap;
    std::vector<std::string> known_locations;
    write_module("main", smv);

    setup_variables(program, varMap, initMap, smv);
    write_locations(program, varMap, smv, known_locations);
    write_variables(program, varMap, smv);
    write_init(initMap, smv);
    write_next(program, varMap, smv, known_locations);
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
  std::vector<registerStruct> registers;

  for (const MiniMC::Model::Function_ptr& func: prg->getFunctions()){
    functionRegisters(func, registers);
  }
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
      // if the opts.spec is equal to "unsafe_fork" replace it with "AG (locations = fork-bb0 -> AX(AF locations =
      // fork-bb0))"
      if (opts.spec == "unsafe_fork") {
        opts.spec = "AG (locations = fork-bb0 -> AX(AF locations = fork-bb0))";
        outfile << std::endl << "CTLSPEC NAME unsafe_fork := " << opts.spec << ";" << std::endl;
      } else {
        outfile << std::endl << "CTLSPEC " << std::endl << opts.spec << std::endl;
      }
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
        if (buffer == "unsafe_fork") {
          spec += ("\nCTLSPEC NAME unsafe_fork := AG (locations = fork-bb0 -> AX(AF locations = fork-bb0));\n");
        } else {
          spec += "CTLSPEC\n";
          spec += buffer;
          spec += "\n";
        }
        
        messager.message("Enter CTL spec: ");
      }
      outfile << std::endl << spec << std::endl;
    }
  } catch (std::except    messager.message(e.what());
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
