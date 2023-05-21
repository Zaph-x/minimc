#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "loaders/loader.hpp"
#include "cpa/concrete.hpp"
#include "model/output.hpp"
#include "smv_types.hpp"

void LocationSpec::assign_next(SmvSpec& parent_spec, std::string identifier, std::string bb_location) {
  if (parent_spec.get_location(identifier+"-bb"+bb_location) == nullptr) {
    return;
  }
  next.push_back(std::make_shared<LocationSpec>(identifier, bb_location));
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
