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
      //    if (cont.index() == 9){
      //      MiniMC::Model::NonDetContent non = std::get<9>(cont);
      //      for (auto param: call.params) {
      //          std::cout << "Resulting register: " << call.res->string_repr() << std::endl;
      //          std::cout << "Function: " << call.function->string_repr() << std::endl;
      //          std::cout << "Parameter: " << param->string_repr() << std::endl;
      //      }
      //    }
    }
    auto abe = "abe";

    return registers;
  };
}

MiniMC::Host::ExitCodes ctpl_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& cpa) {
  MiniMC::Support::Messager messager;
  messager.message("Initiating EnumStates");

  MiniMC::Model::Program& prgm = *controller.getProgram();

  std::vector<std::shared_ptr<MiniMC::Model::Function>> functions = prgm.getFunctions();
// EFFICIENCY ITSELF:
  for (const std::shared_ptr<MiniMC::Model::Function>& function: functions){
    for (const std::shared_ptr<MiniMC::Model::Location>& location: function->getCFA().getLocations()){
       for (std::string str : registersInUse(location)){
         std::cout << str << std::endl;
       }

    }
    std::vector<std::shared_ptr<MiniMC::Model::Edge>> functionEdges = function->getCFA().getEdges();
    for (const std::shared_ptr<MiniMC::Model::Edge>& edge: functionEdges){
      MiniMC::Model::Edge derefEdge = *edge;
      MiniMC::Model::InstructionStream instrStream = derefEdge.getInstructions();
      for (const auto& instr: instrStream){
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
             //   std::cout << reg.res.get()->string_repr() << std::endl;
                std::cout << reg.function.get() << std::endl;
                std::cout << reg.function << std::endl;

                std::vector<MiniMC::Model::Value_ptr> params = reg.params;

                for (auto param: params){
                  std::cout << param.get()->string_repr() << std::endl;
                }
        }



      }
    }
  }

  return MiniMC::Host::ExitCodes::AllGood;
}





static CommandRegistrar ctpl_reg("ctpl", ctpl_main, "Checking CT(P)L specification", addOptions);
