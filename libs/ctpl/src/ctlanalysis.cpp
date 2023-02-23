#include "ctlanalysis.h"
#include "plugin.hpp"
#include "cpa/interface.hpp"
#include "host/host.hpp"
#include "model/controller.hpp"
#include "model/location.hpp"
#include "support/feedback.hpp"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace {

  void addOptions(po::options_description& op) {


    };
}

std::vector<std::string> registersInUse(MiniMC::Model::Location_ptr loc){
    std::vector<std::string> registers;
    MiniMC::Model::Location location = *loc.get();
    if (location.hasOutgoingEdge()){

    }

    return registers;

};

MiniMC::Host::ExitCodes ctpl_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& cpa) {
    MiniMC::Support::Messager messager;
    messager.message("Initiating EnumStates");

    auto& prgm = *controller.getProgram();

    return MiniMC::Host::ExitCodes::AllGood;
}

static CommandRegistrar ctpl_reg("ctpl", ctpl_main, "Checking CT(P)L specification", addOptions);
