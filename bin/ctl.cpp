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

namespace po = boost::program_options;

struct registerStruct {
  std::string destinationRegister, opcode, content, regLocation, condition = "undef";
};

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

std::string format_immediate_value(std::string value_to_format) {
  std::vector<std::string> split_string;
  std::string intermediate = value_to_format;
  boost::split(split_string, intermediate, boost::is_any_of(" "));

  auto value = split_string[0];
  boost::replace_all(value, "<0x", "");
  if (value.empty()) {
    throw std::exception();
  }
  return value;
}

std::string format_register(std::string regStr) {
  std::vector<std::string> splitString;
  std::string intermediateString = regStr;
  boost::replace_all(intermediateString, ".", "_");
  boost::replace_all(intermediateString, ":tmp", "-reg");
  boost::split(splitString, intermediateString, boost::is_any_of(" "));
  splitString[0].erase(0, 1);

  if (splitString[0].empty() || std::isdigit(splitString[0][0])) {
    splitString[0] = splitString[0].substr(1);
  }
  return splitString[0];
}

std::string format_unknown(std::string& func_name, std::string unknown_value) {
  if (boost::contains(unknown_value, func_name)) {
    return format_register(unknown_value);
  }
  return format_immediate_value(unknown_value);
}

bool is_register(std::string& func_name, std::string& value) {
  return boost::contains(value, func_name);
}

std::vector<registerStruct> get_for_register(std::string& func_name, std::string& register_name, std::unordered_map<std::string, std::vector<registerStruct>>& registers) {
  std::vector<registerStruct> result;
  for (auto& reg : registers[func_name]) {
    if (reg.destinationRegister == register_name) {
      result.push_back(reg);
    }
  }
  return result;
}

void add_range(std::string& location, std::string& dest, std::string &funcName, std::string& lower, std::string& upper, std::unordered_map<std::string, std::vector<registerStruct>> &regs) {
  int lowerInt = 0;
  int upperInt = 0;
  if (!lower.empty() && std::find_if(lower.begin(), lower.end(), [](unsigned char c) { return !std::isdigit(c); }) == lower.end()) lowerInt = std::stoi(lower);
  if (!upper.empty() && std::find_if(upper.begin(), upper.end(), [](unsigned char c) { return !std::isdigit(c); }) == upper.end()) upperInt = std::stoi(upper);

  registerStruct mmcregister;
  mmcregister.destinationRegister = dest;
  mmcregister.opcode = "Range";
  mmcregister.content = std::to_string(lowerInt);
  mmcregister.regLocation = location;
  mmcregister.condition = dest + " = " + mmcregister.content;
  regs[funcName].push_back(mmcregister);

  if (lowerInt < upperInt) {
    for (int i = lowerInt+1; i <= upperInt; i++) {
      mmcregister.destinationRegister = dest;
      mmcregister.opcode = "Range";
      mmcregister.content = std::to_string(i);
      mmcregister.regLocation = location;
      mmcregister.condition = dest + " != " + std::to_string(i) + " & " + dest + " = " + std::to_string(i-1);

      regs[funcName].push_back(mmcregister);
    }
  } else {
    for (int i = lowerInt-1; i >= upperInt; i--) {
      mmcregister.destinationRegister = dest;
      mmcregister.opcode = "Range";
      mmcregister.content = std::to_string(i);
      mmcregister.regLocation = location;
      mmcregister.condition = dest + " != " + std::to_string(i) + " & " + dest + " = " + std::to_string(i+1);

      regs[funcName].push_back(mmcregister);
    }
  }
}

void functionRegisters(const MiniMC::Model::Function_ptr& func, std::unordered_map<std::string, std::vector<registerStruct>>& registers) {
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
              std::string op1 = format_unknown(funcName, content.op1->string_repr());
              std::string op2 = format_unknown(funcName, content.op2->string_repr());

              auto contentString = op1 + "_" + op2;

              if (opCodeString.starts_with("ICMP_")) {
                add_range(intermediateLocation, op1, funcName, op1, op2, registers);
                mmcregister.destinationRegister = resName;
                mmcregister.opcode = "Compare";
                if (opCodeString.ends_with("LT")) {
                  mmcregister.condition = op1 + " < " + op2;
                  mmcregister.content = "true";
                  registers[funcName].push_back(mmcregister);
                  mmcregister.condition = op1 + " >= " + op2;
                  mmcregister.content = "false";
                  registers[funcName].push_back(mmcregister);
                } else if (opCodeString.ends_with("GT")) {
                  mmcregister.condition = op1 + " > " + op2;
                  mmcregister.content = "true";
                  registers[funcName].push_back(mmcregister);
                  mmcregister.condition = op1 + " <= " + op2;
                  mmcregister.content = "false";
                  registers[funcName].push_back(mmcregister);
                } else if (opCodeString.ends_with("LE")) {
                  mmcregister.condition = op1 + " <= " + op2;
                  mmcregister.content = "true";
                  registers[funcName].push_back(mmcregister);
                  mmcregister.condition = op1 + " > " + op2;
                  mmcregister.content = "false";
                  registers[funcName].push_back(mmcregister);
                } else if (opCodeString.ends_with("GE")) {
                  mmcregister.condition = op1 + " >= " + op2;
                  mmcregister.content = "true";
                  registers[funcName].push_back(mmcregister);
                  mmcregister.condition = op1 + " < " + op2;
                  mmcregister.content = "false";
                  registers[funcName].push_back(mmcregister);
                }
              } else if (opCodeString == "Add") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " + " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Sub") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " - " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Mul") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " * " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Div") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " / " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Mod") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " % " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "And") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " & " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Or") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " | " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              } else if (opCodeString == "Xor") {
                mmcregister.destinationRegister = resName;
                mmcregister.content = op1 + " ^ " + op2;
                mmcregister.opcode = opCodeString;
                registers[funcName].push_back(mmcregister);
              }
              else {
                mmcregister.destinationRegister = resName;
                mmcregister.content = contentString;
                mmcregister.opcode = opCodeString;

                registers[funcName].push_back(mmcregister);
              }
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
              registers[funcName].push_back(mmcregister);
            }
          }
          if (std::variant(instr.getContent()).index() == 9) {
            auto content = get<MiniMC::Model::NonDetContent>(instr.getContent());
            auto nonDetString = "NonDet";
            if (content.res->isRegister()) {
              auto nonDetReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitResult;
              boost::split(splitResult, location->getInfo().getName(), boost::is_any_of(":"));
              auto symbol_prefix = nonDetReg->getSymbol().prefix().getName();
              boost::replace_all(symbol_prefix, "_", "");
              auto resName = splitResult[0] + "-" + symbol_prefix;
              std::ostringstream ossNonDet;
              ossNonDet << instr.getOpcode();
              auto nonDet = format_register(content.res->string_repr() + content.min->string_repr() + content.max->string_repr() + content.arguments);
              mmcregister.destinationRegister = resName;
              mmcregister.content = nonDetString;
              mmcregister.opcode = nonDet;
              mmcregister.condition = "TRUE";
              registers[funcName].push_back(mmcregister);
              mmcregister.content = "undef";
              registers[funcName].push_back(mmcregister);
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
              auto load = format_register(content.addr->string_repr());
              mmcregister.destinationRegister = resName;
              mmcregister.content = loadString;
              mmcregister.opcode = load;
              registers[funcName].push_back(mmcregister);
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
              auto store = format_register(content.addr->string_repr() + content.storee->string_repr() + content.variableName);
              mmcregister.destinationRegister = resName;
              mmcregister.content = storeString;
              mmcregister.opcode = store;
              registers[funcName].push_back(mmcregister);
            }
          }
          // CallContents index as a variant of InstructionContent is 14
          if (std::variant(instr.getContent()).index() == 14) {
            std::string resName;
            std::string callContent;
            std::string opCodeString = "Call";
            auto content = get<MiniMC::Model::CallContent>(instr.getContent());
            if (!content.res) {
              resName = opCodeString + "-" + intermediateLocation;
            } else if (content.res->isRegister()) {
              auto returnVal = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              std::vector<std::string> splitReturnVal;
              boost::split(splitReturnVal, location->getInfo().getName(), boost::is_any_of(":"));
              resName = splitReturnVal[0] + "-" + returnVal->getSymbol().prefix().getName();
              boost::replace_all(resName, ":", "-");
              auto call_content_end = content.res->getType()->get_type_name();
              boost::replace_all(call_content_end, "_", "");
              callContent += call_content_end;
            }

            callContent += content.argument;
            if (content.params.size() > 1) {
              for (unsigned long i = 1L; i < content.params.size(); i++) {

                if (content.params[i]->isConstant()) {
                  auto paramsConstant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(content.params[i]);
                  if (paramsConstant->isPointer() && paramsConstant->getSize() == 8L) {
                    auto paramsPointer = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(paramsConstant);
                    auto paramsPointerValue = paramsPointer->getValue();
                    // Pointer struct segments use ascii values to describe the content. 72 = H = Heap. 70 = F = Function and so on.
                    if (paramsPointerValue.segment == 72) {
                      for (auto initInstr : func->getPrgm().getInitialiser()) {
                        if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
                          auto content = get<12>(instr.getContent());
                          callContent += "_" + content.variableName;
                        }
                      }
                    }
                  }
                } else {
                  callContent += "_" + format_register(content.params[i]->string_repr());
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
                  // Do 32-bit sized ptr stuff.
                }
              }
              // Hvordan håndteres result registret i et void call.
              /*if (content.res && content.res->isRegister()) {
              auto resReg = std::dynamic_pointer_cast<MiniMC::Model::Register>(content.res);
              resName = resReg->getSymbol().getFullName();
            }*/
              mmcregister.destinationRegister = resName;
              mmcregister.content = callContent;
              mmcregister.opcode = opCodeString;
              mmcregister.regLocation = intermediateLocation;
              registers[funcName].push_back(mmcregister);
            }
          }
        }
      }
    }
  }
}
// Adds variable values to the varMap for StoreInstructions
void storeVarValue(const MiniMC::Model::Instruction& instr, std::unordered_map<std::string, std::vector<std::string>>& varMap) {
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
      } else if (value->isAggregate()) {
        auto storeeValue = std::dynamic_pointer_cast<MiniMC::Model::AggregateConstant>(value)->string_repr();
        std::vector<std::string> split;
        boost::split(split, storeeValue, boost::is_any_of("$"));
        std::string asciiValues = split[1];
        std::string charValues;
        for (int i = 0; i < asciiValues.length(); i += 3) {
          if (asciiValues[i] == ' ') {
            i++;
          }

          std::string byte = asciiValues.substr(i, 3);
          char char_byte = char(std::stoi(byte, nullptr, 16));
          if (char_byte == '\0') {
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

void setup_variables(const MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::unordered_map<std::string, std::string>& initMap, std::string& smv) {
  // Write variables
  initMap["locations"] = "ASSIGN init(locations) := main-bb0;\n";
  for (const auto& instr : program.getInitialiser()) {
    storeVarValue(instr, varMap);
  }
  for (auto var : varMap) {
    if (var.second.size() == 1) continue;
    initMap[var.first] = "ASSIGN init(" + var.first + ") := " + var.second[0] + ";\n";
  }
}

void write_locations(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::string& smv, std::vector<std::string>& known_locations) {
  std::string locations = "";
  for (const auto& func : program.getFunctions()) {
    std::string funcName = func->getSymbol().getName();

    // Make module states
    known_locations.push_back(funcName + "-bb0");
    locations += funcName + "-bb0, ";
    for (auto loc : func->getCFA().getEdges()) {
      std::string locName = std::to_string(loc->getTo()->getID());

      std::string currentLocation = funcName + "-bb" + locName;
      if (std::find(known_locations.begin(), known_locations.end(), currentLocation) == known_locations.end()) {
        known_locations.push_back(currentLocation);
        locations += currentLocation + ", ";
      }
    }
  }
  for (const auto& func : program.getFunctions()) {
    std::string funcName = func->getSymbol().getName();
    for (auto loc : func->getCFA().getLocations()) {
      if (loc->nbIncomingEdges() > 0) {
        for (auto edge : loc->getIncomingEdges()) {
          // Iterate through instructions on each edge
          for (auto instr : edge->getInstructions()) {
            storeVarValue(instr, varMap);
          }
        }
      }
    }
  }

  locations = locations.substr(0, locations.size() - 2);
  smv += "VAR locations: {" + locations + "};\n";
}

std::string get_value_mappings(MiniMC::Model::Program &prg, std::unordered_map<std::string, std::vector<registerStruct>> &varMap, std::string &contents, bool &has_changed) {
  std::string new_contents ="";
  for (const auto& func : prg.getFunctions()) {
    std::string funcName = func->getSymbol().getName();
    if (contents.find(funcName) != std::string::npos) {
      has_changed = true;
      for (auto& itm : get_for_register(funcName, contents, varMap)) {
        new_contents += itm.content+ ", ";
      }
    }
  }
  return new_contents.substr(0, new_contents.size() - 2);
}

void write_variables(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::string& smv, std::unordered_map<std::string, std::vector<registerStruct>> varRegs) {
  std::string varValues;
  for (const auto& var : varMap) {
    varValues = "";
    std::string varName = var.first;
    if (var.second.size() == 1) {
      varValues += "nondet, ";
    }
    for (auto value : var.second) {
      varValues += value + ", ";
    }
    varValues = varValues.substr(0, varValues.size() - 2);
    smv += "VAR " + varName + " : {" + varValues + "};\n";
  }
  std::unordered_map<std::string, std::string> register_mappings;
  for (auto& reglist : varRegs) {
    for (auto& regStruct : reglist.second) {
      std::string varRegName = regStruct.destinationRegister;
      std::string value = regStruct.content;
      std::vector<std::string> split;
      // only split on last index of an operator
      boost::split(split, value, boost::is_any_of(" "));
      std::string contents = split[0];
      
      
      // check if contents contains the name of any function in the program
      bool has_changed = false;
      std::string new_contents = "";
      new_contents = get_value_mappings(program, varRegs, contents, has_changed);

      if (!new_contents.empty() && has_changed) {
        regStruct.content = new_contents;
      }

      if (!register_mappings.contains(varRegName)) {
        register_mappings[varRegName] = regStruct.content;
      } else {
        register_mappings[varRegName] += ", " + regStruct.content;
      }
    }
  }
  for (auto& reg : register_mappings) {
    smv += "VAR " + reg.first + " : {" + reg.second + "};\n";
  }
}

void write_init(std::unordered_map<std::string, std::string> initMap, std::string& smv, std::unordered_map<std::string, std::vector<registerStruct>> varRegs) {
  for (const auto& init : initMap) {
    smv += init.second;
  }
  std::vector<std::string> used_regs;
  std::vector<std::string> used_funcs;
  for (auto& kv : varRegs){
    used_funcs.push_back(kv.first);
  }
  for (auto& regStruct : varRegs) {

    for (auto& reg : regStruct.second) {
      if (std::find(used_regs.begin(), used_regs.end(), reg.destinationRegister) == used_regs.end()) {
        std::string contentString = reg.content;
        std::vector<std::string> function_result;
        std::vector<std::string> register_result;
        boost::split(function_result, contentString, boost::is_any_of("-"));
        boost::split(register_result, contentString, boost::is_any_of(" "));
        if (get_for_register(function_result[0], register_result[0], varRegs).size() == 1) {
          used_regs.push_back(reg.destinationRegister);
          continue;
        }
        function_result.clear();
//        register_result.clear();

        boost::split(function_result, reg.regLocation, boost::is_any_of("-"));
        if (get_for_register(function_result[0], reg.destinationRegister, varRegs).size() == 1) {
          used_regs.push_back(reg.destinationRegister);
          continue;
        }


        if (std::find(used_funcs.begin(), used_funcs.end(), function_result[0]) != used_funcs.end()){
          smv += "ASSIGN init(" + reg.destinationRegister + ") :=" + get_for_register(function_result[0], register_result[0], varRegs)[0].content + ";\n";
        } else {
          smv += "ASSIGN init(" + reg.destinationRegister + ") :=" + reg.content + ";\n";
        }
        used_regs.push_back(reg.destinationRegister);
      }
    }
  }
}

void store_transitions(MiniMC::Model::Program program, MiniMC::Model::Function_ptr function, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::string& smv, std::vector<std::string>& known_locations, std::unordered_map<std::string, std::vector<std::string>>& transition_map) {

  for (auto& edge : function->getCFA().getEdges()) {
    std::string from = std::to_string(edge->getFrom()->getID());
    std::string to = std::to_string(edge->getTo()->getID());
    std::string currentLocation = function->getSymbol().getName() + "-bb" + from;
    std::string nextLocation = function->getSymbol().getName() + "-bb" + to;
    std::string next_stored = nextLocation;
    for (auto& instr : edge->getInstructions()) {
      if (instr.getOpcode() == MiniMC::Model::InstructionCode::Call) {
        auto call_instr = get<14>(instr.getContent());
        std::string func_name = call_instr.argument;
        nextLocation = func_name + "-bb0";
        if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
          nextLocation = "error";
        }
        // transition_map[currentLocation].push_back(next_stored);
        auto edge_count = program.getFunction(func_name)->getCFA().getEdges().size();
        auto func_edges = program.getFunction(func_name)->getCFA().getEdges();
        std::vector<std::string> ret_bb_edges = {};
        for (auto& edge : func_edges) {
          for (auto& edge_instr : edge->getInstructions()) {
            if (edge_instr.getOpcode() == MiniMC::Model::InstructionCode::Ret) {
              ret_bb_edges.push_back(std::to_string(edge->getTo()->getID()));
            }
          }
        }
        if (!ret_bb_edges.empty()) {
          for (auto& ret_bb : ret_bb_edges) {
            transition_map[func_name + "-bb" + ret_bb].push_back(next_stored);
          }
        } else {
          transition_map[func_name + "-bb" + std::to_string(edge_count)].push_back(next_stored);
        }
      }
    }
    if (std::find(known_locations.begin(), known_locations.end(), nextLocation) == known_locations.end()) {
      nextLocation = "error";
    }
    transition_map[currentLocation].push_back(nextLocation);
  }
}

void write_function_case(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, MiniMC::Model::Function_ptr function, std::string& smv, std::vector<std::string>& known_locations) {
  std::unordered_map<std::string, std::vector<std::string>> transitions;

  store_transitions(program, function, varMap, smv, known_locations, transitions);

  for (auto kv : transitions) {
    std::string transition = "{";
    for (auto next : kv.second) {
      transition += next + ", ";
    }
    transition = transition.substr(0, transition.size() - 2);
    transition += "}";
    smv += "    locations = " + kv.first + " : " + transition + ";\n";
  }
}

void write_assignment_case(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::string& smv, std::vector<std::string>& known_locations, std::string var_name) {
  // TODO Iterate vars, assign if var name matches. Fix all next cases
  std::string assignment_case = "";
  for (const auto& func : program.getFunctions()) {
    std::string funcName = func->getSymbol().getName();
    for (auto loc : func->getCFA().getLocations()) {
      if (loc->nbIncomingEdges() > 0) {
        for (auto edge : loc->getIncomingEdges()) {
          // Iterate through instructions on each edge
          for (auto instr : edge->getInstructions()) {
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

void write_next(MiniMC::Model::Program program, std::unordered_map<std::string, std::vector<std::string>>& varMap, std::string& smv, std::vector<std::string>& known_locations) {
  std::unordered_map<std::string, std::string> call_stack_map;
  smv += "ASSIGN next(locations) :=\n";
  smv += "  case\n";
  MiniMC::Model::Function_ptr function = program.getFunctions().back(); // get main

  for (const auto& func : program.getFunctions()) {
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
void write_registerVar_next(std::string& smv, std::unordered_map<std::string, std::vector<registerStruct>> varRegs) {
  std::string currentVarName = "";
  std::string lastVar = "nonemptyString!.";

  for (auto& var : varRegs) {
    currentVarName = "";
    for (auto &reg : var.second) {
      std::string contentString = reg.content;
      std::vector<std::string> function_result;
      std::vector<std::string> register_result;
      boost::split(function_result, contentString, boost::is_any_of("-"));
      boost::split(register_result, contentString, boost::is_any_of(" "));
      if (get_for_register(function_result[0], register_result[0], varRegs).size() == 1) {
        continue;
      }
      function_result.clear();
//        register_result.clear();

      boost::split(function_result, reg.regLocation, boost::is_any_of("-"));
      if (get_for_register(function_result[0], reg.destinationRegister, varRegs).size() == 1) {
        continue;
      }
      std::string transitionString = "    " + reg.condition + " & ( locations = " + reg.regLocation + ") : " + reg.content + "; \n";

      if(lastVar != currentVarName && currentVarName != reg.destinationRegister){
        currentVarName = reg.destinationRegister;
        smv += "ASSIGN next(" + currentVarName + ") :=\n";
        smv += "  case\n";
        smv += transitionString;
      } else if(currentVarName == reg.destinationRegister){
        smv += transitionString;
      } else if(lastVar != currentVarName && currentVarName != reg.destinationRegister){
        smv += transitionString;
        smv += "    " + currentVarName + " : " + "TRUE;\n";
        smv += "  esac;\n";
      } else {
        smv += "    TRUE : " + currentVarName + + ";\n";
        smv += "  esac;\n";
      }
      lastVar = reg.destinationRegister;
    }
  }
  if (!currentVarName.empty()){
    smv += "    TRUE : " + currentVarName + ";\n";
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
  std::unordered_map<std::string, std::vector<registerStruct>> varRegs;
  std::vector<std::string> known_locations;
  write_module("main", smv);
  for (const auto& func : program.getFunctions()) {
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
    int expect_false{0};
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

    po::options_description desc("CTL options");
    desc.add_options()("ctl.spec", po::value<std::string>()->default_value("")->notifier(setSpec), "Set an inital CTL spec");
    desc.add_options()("ctl.keep-smv", po::value<int>()->default_value(0)->notifier(setKeepSMV), "Keep the SMV file after analysis (0 = no, 1 = yes)");
    desc.add_options()("ctl.expect-false", po::value<int>()->default_value(0)->notifier(setExpectFalse), "Expect the CTL spec to be false (0 = no, 1 = yes)");
    od.add(desc);
  }

}; // namespace

MiniMC::Host::ExitCodes ctl_main(MiniMC::Model::Controller& ctrl, const MiniMC::CPA::AnalysisBuilder& cpa) {
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
        outfile << std::endl
                << "CTLSPEC NAME unsafe_fork := " << opts.spec << ";" << std::endl;
      } else {
        outfile << std::endl
                << "CTLSPEC " << std::endl
                << opts.spec << std::endl;
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
      outfile << std::endl
              << spec << std::endl;
    }
  } catch (std::exception& e) {
    messager.message(e.what());
    outfile.close();
    if (opts.keep_smv == 0)
      std::remove(filename.c_str());

    return MiniMC::Host::ExitCodes::RuntimeError;
  }
  outfile.close();
  std::string output = exec_cmd("nusmv output_file.smv");

  if (opts.keep_smv == 0)
    std::remove(filename.c_str());

  bool failed = false;
  if (output.find("is false") != std::string::npos) {
    failed = true;
  }

  messager.message(output);

  if (output.find("Parse Error") != std::string::npos) {
    messager.message("!!! Parse Error !!!");
    return MiniMC::Host::ExitCodes::RuntimeError;
  }
  if (opts.expect_false == 1 && !failed) {
    messager.message("!!! Expected CTL spec to be false !!!");
    return MiniMC::Host::ExitCodes::RuntimeError;
  } else if (opts.expect_false == 0 && failed) {
    messager.message("!!! Expected CTL spec to be true !!!");
    return MiniMC::Host::ExitCodes::RuntimeError;
  }
  return MiniMC::Host::ExitCodes::AllGood;
}

static CommandRegistrar ctl_reg("ctl", ctl_main, "CTL analysis", addOptions);
