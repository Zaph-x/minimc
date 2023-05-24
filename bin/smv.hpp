#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "loaders/loader.hpp"
#include "cpa/concrete.hpp"
#include "model/output.hpp"
#include "smv_types.hpp"
#include <boost/algorithm/string.hpp>


void LocationSpec::assign_next(SmvSpec& parent_spec, std::string identifier, std::string bb_location) {
  if (parent_spec.get_location(identifier+"-bb"+bb_location) == nullptr) {
    return;
  }
  next.push_back(std::make_shared<LocationSpec>(identifier, bb_location));
}

void generate_spec_paths(SmvSpec spec, MiniMC::Model::Program_ptr& prg, const std::shared_ptr<MiniMC::Model::Function>& function, const std::shared_ptr<MiniMC::Model::Function>& return_function) {
  for (auto& edge : function->getCFA().getEdges()) {
    std::string locName = std::to_string(edge->getFrom()->getID());
    spec.add_location(function->getSymbol().getName(), locName)->add_instructions(edge->getInstructions(), prg);
  }
}

SmvSpec generate_smv_spec(MiniMC::Model::Program_ptr& prg) {
  SmvSpec spec;


  for (auto& function : prg->getFunctions()) {
    for (auto& edge : function->getCFA().getEdges()) {
      std::string locName = std::to_string(edge->getFrom()->getID());
      spec.add_location(function->getSymbol().getName(), locName)->add_instructions(edge->getInstructions(), prg);
    }
    for (auto& edge : function->getCFA().getEdges()) {
      std::string locName = std::to_string(edge->getFrom()->getID());
      std::string nextLocName = std::to_string(edge->getTo()->getID());
      if (spec.get_location(function->getSymbol().getName()+"-bb"+locName)->is_calling()) {

        auto& called_function = spec.get_location(function->getSymbol().getName()+"-bb"+locName)->get_called_function();
        std::cout << "Called function: " << called_function << std::endl;
        std::cout << "Called function exists: " << (prg->getFunction(called_function) != nullptr) << std::endl;

        for (auto& ret_loc : spec.get_returning_locations(called_function)) {
          std::cout << "Returning to: " << ret_loc->get_identifier() << std::endl;
          ret_loc->assign_next(spec, function->getSymbol().getName(), nextLocName);
        }
        continue;
      }
      spec.get_location(function->getSymbol().getName()+"-bb"+locName)->assign_next(spec, function->getSymbol().getName(), nextLocName);
    }
  }

  return spec;
};

std::shared_ptr<LocationSpec> SmvSpec::add_location(std::string identifier, std::string location) {
  if (get_location(identifier+"-bb"+location) != nullptr) {
    return nullptr;
  }
  locations.push_back(std::make_shared<LocationSpec>(identifier, location));
  return locations.back();
}

void SmvSpec::add_register(std::string identifier, SmvType type) {
  registers.push_back(std::make_shared<RegisterSpec>(identifier, type));
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

template <class Spec>
std::string write_variable_block(const std::string& name, const std::vector<std::shared_ptr<Spec>>& specs) {
  return write_listed_variable(name, specs) + write_transitions(name, specs);
}

std::string write_ctl_spec(const std::string& spec) {
  return "CTLSPEC\n" + spec + ";\n";
}

void SmvSpec::write(const std::string file_name, const std::string& ctl_spec, std::unordered_map<std::string, std::vector<std::string>>& ctl_specs) {
  std::ofstream file(file_name);
  std::string output = "";
  output += "MODULE main\n";
  output += write_variable_block("locations", locations);

  if (ctl_specs.find(ctl_spec) != ctl_specs.end()) {
    for (const auto& spec : ctl_specs[ctl_spec]) {
      output += write_ctl_spec(spec);
    }
  } else {
    output += write_ctl_spec(ctl_spec);
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
