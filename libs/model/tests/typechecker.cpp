#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "model/checkers/typechecker.hpp"
#include "model/cfg.hpp"
#include "model/variables.hpp"


#include "doctest/doctest.h"


auto buildProgram (bool ok) {
  auto tfac = std::make_shared<MiniMC::Model::TypeFactory64> ();
  auto cfac = std::make_shared<MiniMC::Model::ConstantFactory64> (tfac);
  auto regdescr = std::make_unique<MiniMC::Model::RegisterDescr> ();
  
  auto val = cfac->makeIntegerConstant (5,MiniMC::Model::TypeID::I8);
  auto sourceinfo = std::make_shared<MiniMC::Model::SourceInfo> ();
  
  MiniMC::Model::CFA cfa;
  
  auto from = cfa.makeLocation (MiniMC::Model::LocationInfo (MiniMC::Model::Symbol{"from"},0,*sourceinfo,regdescr.get()));
  auto to = cfa.makeLocation (MiniMC::Model::LocationInfo (MiniMC::Model::Symbol{"to"},0,*sourceinfo,regdescr.get()));
  cfa.setInitial (from);
  auto edge = cfa.makeEdge (from,to);
  MiniMC::Model::Program prgm{tfac,cfac};
  if (!ok) {
    edge->getInstructions ().template addInstruction<MiniMC::Model::InstructionCode::Ret> ({.value = val});
    auto func = prgm.addFunction ("test",
				  {},
				  val->getType (),
				  std::move(regdescr),
				  std::move(cfa));
    
    
  }

  else {
    edge->getInstructions ().template addInstruction<MiniMC::Model::InstructionCode::RetVoid> ({});
    auto func = prgm.addFunction ("test",
				  {},
				  tfac->makeVoidType(),
				  std::move(regdescr),
				  std::move(cfa));
    
    
  }

  prgm.addEntryPoint ("test");


  return prgm;

}


TEST_CASE ("TypeCheck Entry Point return value ") {
  auto prgm = buildProgram (false);
  REQUIRE (prgm.getEntryPoints ().size () == 1);
  MiniMC::Model::Checkers::TypeChecker checker;

  CHECK (!checker.run (prgm));
  
}


TEST_CASE ("TypeCheck Entry Point does not return value ") {
  auto prgm = buildProgram (true);
  REQUIRE (prgm.getEntryPoints ().size () == 1);
  MiniMC::Model::Checkers::TypeChecker checker;

  CHECK (checker.run (prgm));
  
}

