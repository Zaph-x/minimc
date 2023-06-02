#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include "loaders/loader.hpp"
#include "model/output.hpp"
#include "smv_types.hpp"
#include <boost/algorithm/string.hpp>


void LocationSpec::assign_next(SmvSpec& parent_spec, std::string identifier, std::string bb_location) {
  if (parent_spec.get_location(identifier+"-bb"+bb_location) == nullptr) {
    return;
  }
  next.push_back(std::make_shared<LocationSpec>(identifier, bb_location, parent_spec));
}

void assign_vars_and_registers(SmvSpec& spec, const std::shared_ptr<InstructionSpec>& instr, const std::string& location_name, const std::string& basic_block) {
  const auto& location = spec.get_location(location_name+"-bb"+basic_block);
  if (location == nullptr) return;
  if (std::dynamic_pointer_cast<CallInstruction>(instr) != nullptr) {
    const auto& call_instr = std::dynamic_pointer_cast<CallInstruction>(instr);
    if (!call_instr->get_result_register().is_null_register()) {
      spec.add_register(location_name + "-" + call_instr->get_result_register().get_identifier(), SmvType::Int)->add_next(location);
      
    }
    if (!call_instr->get_params().empty()) {
      for (const auto& args : call_instr->get_params()) {
        const auto& rgstr = spec.register_from_str_repr(args);
        std::string func_name = call_instr->get_function_name();
        const auto& new_loc = spec.get_location(func_name+"-bb0");
        if (rgstr != nullptr) {rgstr->add_next(new_loc);}
      }
    }
  } else if (std::dynamic_pointer_cast<TACInstruction>(instr)){
    // Does not seems to do anything yet. - Mkk
    const auto& tac_instr = std::dynamic_pointer_cast<TACInstruction>(instr);
    // Multiple TACOps of the same type can occur in a single block. This should be handled, but is probably an edge case(At least for Xor, this is for demo purpose).
    spec.add_register(location_name + "-" + tac_instr->get_result_register().get_identifier(), SmvType::Int)->add_next(location);
  } else if (std::dynamic_pointer_cast<PtrAddInstruction>(instr)){
    const auto& PtrAddInstr = std::dynamic_pointer_cast<PtrAddInstruction>(instr);
    spec.add_register(location_name + "-" + PtrAddInstr->get_result_register().get_identifier(), SmvType::Int)->add_next(location);
  } else if (std::dynamic_pointer_cast<LoadInstruction>(instr)){
    const auto& LoadInstr = std::dynamic_pointer_cast<LoadInstruction>(instr);
    spec.add_register(location_name + "-" + LoadInstr->get_result_register().get_identifier(), SmvType::Int)->add_next(location);
  } else if (std::dynamic_pointer_cast<StoreInstruction>(instr)){
    const auto& StoreInstr = std::dynamic_pointer_cast<StoreInstruction>(instr);
    spec.add_register(location_name + "-" + StoreInstr->get_stored_register().get_identifier(), SmvType::Int)->add_next(location);
  }


    // If POINTEROPS (PtrAdd, PtrEq) or LOAD/STORE (Memory)
}

void generate_spec_paths(SmvSpec &spec, MiniMC::Model::Program_ptr& prg, const std::shared_ptr<MiniMC::Model::Function>& function) {
  for (auto& edge : function->getCFA().getEdges()) {
    std::string locName = std::to_string(edge->getFrom()->getID());
    spec.add_location(function->getSymbol().getName(), locName)->add_instructions(edge->getInstructions(), prg);
    for (const auto& instr : spec.get_last_location()->get_instructions()) {
      assign_vars_and_registers(spec, instr, function->getSymbol().getName(), locName);
    }
  }
}

SmvSpec generate_smv_spec(MiniMC::Model::Program_ptr& prg) {
  SmvSpec spec;

  for (auto& function : prg->getFunctions()) {
    std::string func_name = function->getSymbol().getName();
    boost::replace_all(func_name, ".", "-");
    generate_spec_paths(spec, prg, function);
  }

  for (auto& function : prg->getFunctions()) {
    std::string func_name = function->getSymbol().getName();
    boost::replace_all(func_name, ".", "-");
    //To handle the naming scheme of intrinsics
    for (auto& edge : function->getCFA().getEdges()) {
      std::string locName = std::to_string(edge->getFrom()->getID());
      std::string nextLocName = std::to_string(edge->getTo()->getID());
      if (spec.get_location(func_name+"-bb"+locName)->is_calling()) {

        auto& called_function = spec.get_location(func_name+"-bb"+locName)->get_called_function();

        for (auto& ret_loc : spec.get_returning_locations(called_function)) {
          ret_loc->assign_next(spec, func_name, nextLocName);
        }
        continue;
      }

      spec.get_location(func_name+"-bb"+locName)->assign_next(spec, func_name, nextLocName);
    }
  }

  return spec;
};

std::shared_ptr<LocationSpec> SmvSpec::add_location(std::string identifier, std::string location) {
  if (get_location(identifier+"-bb"+location) != nullptr) {
    return nullptr;
  }
  locations.push_back(std::make_shared<LocationSpec>(identifier, location, *this));
  // Convert this SmvSpec to a shared_ptr and assign it to a location
  return locations.back();
}

std::shared_ptr<RegisterSpec> SmvSpec::add_register(std::string identifier, SmvType type) {
  const auto& reg = std::make_shared<RegisterSpec>(identifier, type);
  registers.push_back(reg);
  return reg;
}

void SmvSpec::add_var(std::string identifier, SmvType type) {
  vars.push_back(std::make_shared<SmvVarSpec>(identifier, type));
}

// template the Spec class to allow for different types of transitions
template <class Spec>
std::string write_transitions(const std::string& name, const std::vector<std::shared_ptr<Spec>>& specs) {
  std::string output = "";
  output += "ASSIGN next(" + name + ") :=\n  case\n";
  for (const auto& spec : specs) {
    std::vector<std::string> next_names;
    for (const auto& next : spec->get_next()) {
      next_names.push_back(next->get_full_name());
    }
    if (next_names.empty()) {
      continue;
    }
    output += "    " + name + " = " + spec->get_full_name() + " : {" + boost::algorithm::join(next_names, ", ") + "};\n";
  }
  output += "    TRUE : " + name + ";\n";
  output += "  esac;\n";
  return output;
}

template <class Spec>
std::string write_listed_variable(const std::string& name, const std::vector<std::shared_ptr<Spec>>& specs) {
  std::string output = "";
  output += "VAR " + name + " : {";
  std::vector<std::string> spec_names;
  for (const auto& spec : specs) {
    spec_names.push_back(spec->get_full_name());
  }
  output += boost::algorithm::join(spec_names, ", ");
  output += "};\n";
  return output;
}

std::string write_register_transitions(const std::string& name, const std::vector<std::shared_ptr<LocationSpec>>& locations) {
  if (locations.empty()) {
    return "";
  }
  std::string output = "";
  output += "ASSIGN next(" + name + ") :=\n  case\n";
  for (unsigned long int i = 0; i < locations.size(); i++) {
    if (locations[i] == nullptr) continue;
    auto instr_list = locations[i]->get_instructions();
    output += "    locations = " + locations[i]->get_full_name() + " : {";
    if (locations[i]->get_full_name().starts_with("free")) {
      output += "NonDet";
    } else if (i > 0) {
      output += "Modified";
    } else {
      output += "Assigned";
    }
    if (!instr_list.empty()){
      std::vector<std::string> reg_name;
      boost::split(reg_name, name, boost::is_any_of("-"));
      for (auto instr: instr_list){
        if (std::dynamic_pointer_cast<TACInstruction>(instr)){
          auto tac_cast = std::dynamic_pointer_cast<TACInstruction>(instr);
          if (tac_cast->operation == "Xor"){
            output += ", Xored";
          } else if (tac_cast->operation == "ICMP"){
            output += ", Compared";
          }
        } else if (std::dynamic_pointer_cast<LoadInstruction>(instr) != nullptr){
          output += ", Load";
        } else if (std::dynamic_pointer_cast<StoreInstruction>(instr) != nullptr){
          auto store_cast = std::dynamic_pointer_cast<StoreInstruction>(instr);
          if (store_cast->get_stored_register().get_identifier() == reg_name[1]){
            output += ", Store";
          }
        } else if (std::dynamic_pointer_cast<PtrAddInstruction>(instr) != nullptr){
          auto ptradd_cast = std::dynamic_pointer_cast<PtrAddInstruction>(instr);
          if (ptradd_cast->get_result_register().get_identifier() == reg_name[1]){
            output += ", PtrAdd";
          }
        }

      }
    }
    output += "};\n";
  }
  output += "    TRUE : " + name + ";\n";
  output += "  esac;\n";
  return output;
}

std::string write_listed_variable(std::string& name, std::vector<std::string>& values) {
  std::string output = "";
  output += "VAR " + name + " : {";
  output += boost::algorithm::join(values, ", ");
  output += "};\n";
  output += "ASSIGN init(" + name + ") := {" + values[0] + "};\n";
  return output;
}

template <class Spec>
std::string write_variable_block(const std::string& name, const std::vector<std::shared_ptr<Spec>>& specs) {
  return write_listed_variable(name, specs) + write_transitions(name, specs);
}

std::string write_ctl_spec(const std::string& spec) {
  if (spec.empty()) {
    return "";
  }
  return "CTLSPEC\n" + spec + ";\n";
}

void SmvSpec::write(const std::string file_name, const std::string& spec, std::vector<ctl_spec>& ctl_specs) {
  std::ofstream file(file_name);
  std::string output = "-- Output generated automatically by MiniMC\n";
  output += "MODULE main\n";
  output += write_variable_block("locations", locations);
  output += "ASSIGN init(locations) := {main-bb0};\n";
  for (const auto& reg : registers) {
    output += write_listed_variable(reg->get_identifier(), reg->get_values());
    output += write_register_transitions(reg->get_identifier(), reg->get_next());
  }
  for (const auto& var : vars) {
    output += write_listed_variable(var->get_identifier(), var->get_values());
  }

  std::string ctl_spec_output = "";

  for (const auto& ctlspec : ctl_specs) {
    for (const auto& name : ctlspec.ctl_spec_name) {
      if (name == spec) {
        if (ctlspec.replace_type == CTLReplaceType::None) {
          for (const auto& spec : ctlspec.ctl_specs) {
            ctl_spec_output += write_ctl_spec(spec);
          }
        } else if (ctlspec.replace_type == CTLReplaceType::Register) {

          for (const auto& reg : registers) {
            std::string result = write_ctl_spec(spec);
            boost::replace_all(result, "%1", reg->get_identifier());
            ctl_spec_output += result;
          }
        }
      }
    }
  }

  if (ctl_spec_output.empty()) {
    output += write_ctl_spec(spec);
  } else {
    output += ctl_spec_output;
  }

  file << output << std::endl;
}



void SmvSpec::print() {
  for (auto& loc : locations) {
    std::cout << loc->to_string() << std::endl;
  }
  for (auto& reg : registers) {
    std::cout << reg->to_string() << std::endl;
  }
  for (auto& var : vars) {
    std::cout << var->to_string() << std::endl;
  }
}


std::shared_ptr<LocationSpec> SmvSpec::get_location(std::string name) {
  for (auto& loc : locations) {
    if (loc->get_full_name() == name) {
      return loc;
    }
  }
  return nullptr;
}
