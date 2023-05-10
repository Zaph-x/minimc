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
  std::string destinationRegister, opcode, content, regLocation;
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
std::string registerFormat(std::string regStr) {
  std::vector<std::string> splitString;
  std::string intermediateString = regStr;
  boost::replace_all(intermediateString, ".", "depth");
  boost::split(splitString, intermediateString, boost::is_any_of(" "));
  splitString[0].erase(0,1);

  if(splitString[0].empty() || std::isdigit(splitString[0][0])){
    splitString[0] = splitString[0].substr(1);
  }
  return splitString[0];
}
void functionRegisters(const MiniMC::Model::Function_ptr & func, std::vector<registerStruct>& registers) {
  std::string funcName = func->getSymbol().getName();

  for (const auto& location : func->getCFA().getLocations()) {
    if (location->nbIncomingEdges() > 0) {
      for (const auto& edge : location->getIncomingEdges()) {
        std::string locName = std::to_string(edge->getTo()->getID());

        auto instrStream = edge->getInstructions();
        for (auto& instr : instrStream) {
          registerStruct mmcregister;
          std::string intermediateLocation = funcName + "-bb" + locName;
          mmcregister.regLocation = intermediateLocation;
          // TACContents index as a variant of InstructionContent is 0
          if (std::variant(instr.getContent()).index() == 0) {
            auto content = get<MiniMC::Model::TACContent>(instr.getContent());
            if (content.res->isRegister()) {
              auto resReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto resName = splitResult[0] + "-" + resReg->getSymbol().prefix().getName();
              std::ostringstream oss;
              oss << instr.getOpcode();
              auto opCodeString = oss.str();
              std::string op1 = registerFormat(content.op1->string_repr());
              std::string op2 = registerFormat(content.op2->string_repr());

              auto contentString = op1 + "_" + op2;
              boost::replace_all(contentString, ":", "-");
              mmcregister.destinationRegister = resName;
              mmcregister.content = contentString;
              mmcregister.opcode = opCodeString;

              registers.push_back(mmcregister);
            }
          }
          if (std::variant(instr.getContent()).index() == 5) {
            auto content = get<MiniMC::Model::PtrAddContent>(instr.getContent());
            auto ptrAddString = "PtrAdd";
            if (content.res->isRegister()) {
              auto ptrAddReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto resName = splitResult[0] + "-" + ptrAddReg->getSymbol().prefix().getName();
              std::ostringstream ossPtrAdd;
              ossPtrAdd << instr.getOpcode();
              auto ptrAdd = content.res->string_repr() + content.ptr->string_repr() + content.skipsize->string_repr() + content.nbSkips->string_repr();
              mmcregister.destinationRegister = resName;
              mmcregister.content = ptrAddString;
              mmcregister.opcode = ptrAdd;
              registers.push_back(mmcregister);
            }
          }
          if (std::variant(instr.getContent()).index() == 9) {
            auto content = get<MiniMC::Model::NonDetContent>(instr.getContent());
            auto nonDetString = "NonDet";
            if (content.res->isRegister()) {
              auto nonDetReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto resName = splitResult[0] + "-" + nonDetReg->getSymbol().prefix().getName();
              std::ostringstream ossNonDet;
              ossNonDet << instr.getOpcode();
              auto nonDet = registerFormat(content.res->string_repr() + content.min->string_repr() + content.max->string_repr() + content.arguments);
              mmcregister.destinationRegister = resName;
              mmcregister.content = nonDetString;
              mmcregister.opcode = nonDet;
              registers.push_back(mmcregister);
            }
          }
          if (std::variant(instr.getContent()).index() == 11) {
            auto content = get<MiniMC::Model::LoadContent>(instr.getContent());
            auto loadString = "Load";
            if (content.res->isRegister()) {
              auto loadReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto resName = splitResult[0] + "-" + loadReg->getSymbol().prefix().getName();
              std::ostringstream ossload;
              ossload << instr.getOpcode();
              auto load = registerFormat(content.addr->string_repr());
              mmcregister.destinationRegister = resName;
              mmcregister.content = loadString;
              mmcregister.opcode = load;
              registers.push_back(mmcregister);
            }
          }
          if (std::variant(instr.getContent()).index() == 12) {
            auto content = get<MiniMC::Model::StoreContent>(instr.getContent());
            auto storeString = "Store";
            if (content.storee->isRegister()) {
              auto storeReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.addr);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto resName = splitResult[0] + "-" + storeReg->getSymbol().prefix().getName();
              std::ostringstream ossstore;
              ossstore << instr.getOpcode();
              auto store = registerFormat(content.addr->string_repr() + content.storee->string_repr() + content.variableName);
              mmcregister.destinationRegister = resName;
              mmcregister.content = storeString;
              mmcregister.opcode = store;
              registers.push_back(mmcregister);
            }
          }
          // CallContents index as a variant of InstructionContent is 14
          if (std::variant(instr.getContent()).index() == 14) {
            std::string resName;
            std::string callContent;
            std::string opCodeString = "Call";
            auto content = get<MiniMC::Model::CallContent>(instr.getContent());
            if (!content.res){
              resName = opCodeString+"-"+intermediateLocation;
            }else if (content.res->isRegister()) {
              auto returnVal = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitReturnVal;
              boost::split(splitReturnVal, location->getInfo().getName(), boost::is_any_of(":"));
              resName = splitReturnVal[0] + "-" + returnVal->getSymbol().prefix().getName();
              boost::replace_all(resName, ":", "-");
              callContent += content.res->getType()->get_type_name() + "_";
            }

            callContent += content.argument;
            if (content.params.size() > 1) {
              for (int i = 1; i < content.params.size(); i++) {

                if (content.params[i]->isConstant()) {
                  auto paramsConstant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(content.params[i]);
                  if (paramsConstant->isPointer() & paramsConstant->getSize() == 8) {
                    auto paramsPointer = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(paramsConstant);
                    auto paramsPointerValue = paramsPointer->getValue();
                    //Pointer struct segments use ascii values to describe the content. 72 = H = Heap. 70 = F = Function and so on.
                    if (paramsPointerValue.segment == 72) {
                      for (auto initInstr : func->getPrgm().getInitialiser()) {
                        if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
                          auto content = get<12>(instr.getContent());
                          callContent += "_"+content.variableName;
                          }
                        }
                      }
                    }
                } else {
                  callContent += "_" + registerFormat(content.params[i]->string_repr());

                }
                boost::replace_all(callContent, ":", "-");
              }
              auto funcptr = content.function;
              std::string funcName;
              if (funcptr->isConstant()) {
                auto funcConstant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(funcptr);
                auto conSize = funcConstant->getSize();
                if (funcConstant->isPointer() && conSize == 8) {
                  auto tConstant = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(funcConstant);
                  funcName = func->getPrgm().getFunctions()[getFunctionId(tConstant->getValue())]->getSymbol().getName();
                } else if (funcConstant->isPointer() && conSize == 4) {
                  //Do 32-bit sized ptr stuff.
                }
              }
              //Hvordan håndteres result registret i et void call.
              /*if (content.res && content.res->isRegister()) {
              auto resReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              resName = resReg->getSymbol().getFullName();
            }*/
              mmcregister.destinationRegister = resName;
              mmcregister.content = callContent;
              mmcregister.opcode = opCodeString;
              mmcregister.regLocation = intermediateLocation;
              registers.push_back(mmcregister);
            }
          }
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
        boost::replace_all(name, ".", "");

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
          } else if (value->isAggregate()){
            auto storeeValue = std::dynamic_pointer_cast<MiniMC::Model::AggregateConstant>(value)->string_repr();
            std::vector<std::string> split;
            boost::split(split, storeeValue, boost::is_any_of("$"));
            std::string asciiValues = split[1];
            std::string charValues;
            for (int i= 0; i < asciiValues.length(); i+=3){
              if (asciiValues[i] == ' '){
                  i++;
                }

                std::string byte = asciiValues.substr(i, 3);
                char char_byte = char(std::stoi(byte, nullptr, 16));
                if ( char_byte == '\0'){
                  break;
                }
                charValues += char_byte;
            }
            varMap[name].push_back(charValues);
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

void write_variables(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>> &varMap, std::string &smv, std::vector<registerStruct> &varRegs) {
  std::string varValues;
  for (const auto& var : varMap) {
      varValues = "";
       std::string varName = var.first;
       if(var.second.size()==1){
         varValues += "nondet, ";
       }
       for (auto value : var.second) {
         varValues += value + ", ";
       }
       varValues = varValues.substr(0, varValues.size()-2);
       smv += "VAR " + varName + " : {" + varValues + "};\n";
   }
  for (auto& regStruct : varRegs){
    std::string varRegName = regStruct.destinationRegister;
    //boost::replace_all(varRegName, ":", "-");
    smv += "VAR " + varRegName + " : {undef, " + regStruct.content + "};\n";
  }
}

void write_init(std::unordered_map<std::string, std::string> initMap, std::string &smv, std::vector<registerStruct> &varRegs) {
  for (const auto& init : initMap) {
      smv += init.second;
    }
  for (auto& regStruct : varRegs){
    smv += "ASSIGN init(" + regStruct.destinationRegister + ") := undef;\n";
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
void write_registerVar_next(std::string &smv, std::vector<registerStruct> &varRegs){
  for (const auto& var : varRegs) {
    std::string varName = var.destinationRegister;
    smv += "ASSIGN next(" + varName + ") :=\n";
    smv += "  case\n";
    smv += "    " + varName + " = undef & ( locations = " + var.regLocation + ") : " + var.content + "; \n";
    smv += "    TRUE : "+ varName + ";\n";
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
    std::vector<registerStruct> varRegs;
    std::vector<std::string> known_locations;
    write_module("main", smv);
    for(const auto func : program.getFunctions()){
      functionRegisters(func, varRegs);
    }
    setup_variables(program, varMap, initMap, smv);
    write_locations(program, varMap, smv, known_locations);
    write_variables(program, varMap, smv, varRegs);
    write_init(initMap, smv, varRegs);
    write_next(program, varMap, smv, known_locations);
    write_registerVar_next(smv, varRegs);
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

MiniMC::Host::ExitCodes ctl_main(MiniMC::Model::Controller& ctrl, const MiniMC::CPA::AnalysisBuilder& cpa)
{
  MiniMC::Support::Messager messager{};
  auto& prg = ctrl.getProgram();
  std::vector<registerStruct> registers;
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
