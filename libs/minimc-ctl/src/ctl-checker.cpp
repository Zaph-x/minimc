#include "minimc-ctl/ctl-checker.hpp"
#include "host/host.hpp"
#include "model/cfg.hpp"



//Should take Program and Spec datastructure(of some sort).
MiniMC::Host::ExitCodes checkSpec(MiniMC::Model::Program program){

    //Get globals into map for easier checking the 'A' formula.
    std::unordered_map<std::string, MiniMC::Model::Value_ptr> globals;

    for (auto global : program.getInitialiser()){
      if ( global.getOpcode() == MiniMC::Model::InstructionCode::Store){
        auto storeContent = std::get<12>(global.getContent());
        globals.insert({storeContent.variableName, storeContent.storee});
      }
    }

    auto initialMain = program.getFunction("main")->getCFA().getInitialLocation();



    return MiniMC::Host::ExitCodes::AllGood;
}