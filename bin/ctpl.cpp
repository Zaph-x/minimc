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

  std::vector<MiniMC::Model::Register_ptr> registersInLocation = location.getInfo().getRegisters().getRegisters();

  auto edge = location.iebegin();
  std::cout << location.nbIncomingEdges() << std::endl;
  if (location.nbIncomingEdges() == 0){
    return registers;
  }
  auto actual_edge = *edge;
  MiniMC::Model::InstructionStream incomingInstructions = actual_edge->getInstructions();
  std::cout << "Beginning iteration of incoming instructions." << std::endl;
  for (MiniMC::Model::Instruction instr: incomingInstructions){
    std::cout << "Instruction: " << instr << std::endl;
    std::cout << "Instruction opcode: " << instr.getOpcode() << std::endl;
    std::cout << "Instruction operands: " << instr.getContent().index() << std::endl;

  }

  return registers;

};

MiniMC::Host::ExitCodes ctpl_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Support::Messager messager;
  messager.message("Initiating EnumStates");

  auto& prgm = *controller.getProgram();

  std::vector<std::shared_ptr<MiniMC::Model::Function>> functions = prgm.getFunctions();
// EFFICIENCY ITSELF:
  for (const std::shared_ptr<MiniMC::Model::Function>& function: functions){
    for (const std::shared_ptr<MiniMC::Model::Location>& location: function->getCFA().getLocations()){
       std::cout << registersInUse(location). << std::endl;

    }
    std::vector<std::shared_ptr<MiniMC::Model::Edge>> functionEdges = function->getCFA().getEdges();
    for (std::shared_ptr<MiniMC::Model::Edge> edge: functionEdges){
      MiniMC::Model::Edge derefEdge = *edge.get();
      MiniMC::Model::InstructionStream instrStream = derefEdge.getInstructions();
      for (auto instr: instrStream){
        std::cout << instr.getOpcode() << std::endl;
        auto content = instr.getContent();

        if (content.index() == 0){
            auto reg = std::get<0>(content);
            std::cout << reg.res.get()->string_repr() << std::endl;
            std::cout << reg.op1.get()->string_repr() << std::endl;
            std::cout << reg.op2.get()->string_repr() << std::endl;
        }

        if (content.index() == 9){
            auto reg = std::get<9>(content);
            std::cout << reg.res.get()->string_repr() << std::endl;
            std::cout << reg.min.get()->string_repr() << std::endl;
            std::cout << reg.max.get()->string_repr() << std::endl;

        }

        if (content.index() == 14){
                auto reg = std::get<14>(content);
                //std::cout << reg.res.get()->string_repr() << std::endl;
                std::cout << reg.function.get() << std::endl;
                std::cout << *reg.function << std::endl;

                std::vector<MiniMC::Model::Value_ptr> params = reg.params;
                std::cout << "params" << std::endl;
                for (auto param: params){
                  std::cout << param.get()->type_t() << std::endl;

                }
                std::cout << "----------------" << std::endl;
        }



      }
    }
  }

  return MiniMC::Host::ExitCodes::AllGood;
}





static CommandRegistrar ctpl_reg("ctpl", ctpl_main, "Checking CT(P)L specification", addOptions);
