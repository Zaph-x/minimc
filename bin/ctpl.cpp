#include "plugin.hpp"
#include "cpa/interface.hpp"
#include "host/host.hpp"
#include "model/controller.hpp"
#include "model/location.hpp"
#include "support/feedback.hpp"
#include "minimc-ctl/ctl-checker.hpp"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace {

  void addOptions(po::options_description& op) {


  };
}

std::vector<std::string> registersInUse(MiniMC::Model::Location_ptr loc) {
  std::vector<std::string> registers;
  MiniMC::Model::Location location = *loc.get();
  if (location.hasOutgoingEdge()) {

    std::vector<MiniMC::Model::Register_ptr> registersInLocation = location.getInfo().getRegisters().getRegisters();
    std::cout << "registersInUse entered" << std::endl;
    auto edge = location.iebegin();
    std::cout << location.getInfo().getName() << std::endl;
    for (auto reg : registersInLocation) {
      std::cout << reg->getName() << std::endl;
    }

    if (location.nbIncomingEdges() == 0) {
      return registers;
    }
    auto actual_edge = *edge;
    MiniMC::Model::InstructionStream incomingInstructions = actual_edge->getInstructions();
    std::cout << "Beginning iteration of incoming instructions." << std::endl;
    for (MiniMC::Model::Instruction instr : incomingInstructions) {
      auto cont = instr.getContent();
      if (cont.index() == 14) {
        MiniMC::Model::CallContent call = std::get<14>(cont);
        for (auto param : call.params) {
          std::cout << "Resulting register: " << call.res->string_repr() << std::endl;
          std::cout << "Function: " << call.function->string_repr() << std::endl;
          std::cout << "Parameter: " << param->string_repr() << std::endl;
        }
      }

    }
    return registers;
  };
}

MiniMC::Host::ExitCodes ctpl_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Model::Program& prgm = *controller.getProgram();

  checkSpec(prgm);

  auto main = prgm.getFunction("main");
  std::vector<std::shared_ptr<MiniMC::Model::Function>> functions = prgm.getFunctions();

  return MiniMC::Host::ExitCodes::AllGood;
}

bool writeToFile(MiniMC::Model::Program program){
  // Construct SMV string

        return true;
}



static CommandRegistrar ctpl_reg("ctpl", ctpl_main, "Checking CT(P)L specification", addOptions);
