#include "algorithms/algorithms.hpp"
#include "algorithms/successorgen.hpp"
#include "cpa/concrete.hpp"
#include "loaders/loader.hpp"
#include "model/edge.hpp"
#include "model/output.hpp"
#include "model/types.hpp"
#include "plugin.hpp"
#include "interpreter/task.hpp"
#include "vm/concrete/concrete.hpp"
#include <boost/program_options/options_description.hpp>
#include <queue>
#include <utility>

namespace po = boost::program_options;

struct LocalOptions {
  std::string path{""};
};

LocalOptions locoptions;

void addOptions (po::options_description& op) {
  auto setPath = [&](std::string val) { locoptions.path = std::move(val); };

  po::options_description desc("MC Options");
  desc.add_options()("intp.path",
                     po::value<std::string>()->default_value("")->notifier(setPath),
                     "The indexes of the edges needed to form a path");

  op.add(desc);
}

MiniMC::Support::ExitCodes intp_main(MiniMC::Model::Controller& controller, const MiniMC::CPA::AnalysisBuilder& builder) {
  std::queue<MiniMC::Interpreter::Task*> queue;
  std::string s;
  std::unordered_map<std::string,MiniMC::CPA::AnalysisState> statemap;

  auto& prgm = *controller.getProgram ();
  auto transferer = builder.makeTransfer(prgm);
  // Build Initial state
  statemap["current"] = builder.makeInitialState(
      MiniMC::CPA::InitialiseDescr{prgm.getEntryPoints(), prgm.getHeapLayout(),
                                   prgm.getInitialiser(), prgm});

  MiniMC::Interpreter::InterpreterTaskFactory factory(&statemap, transferer);

  // Command Line
  while(true){
    std::cin >> s;
    factory.pushTask(s, &queue);
    while (!queue.empty()){
      MiniMC::Interpreter::Task* task = queue.front();
      queue.pop();
      task->performTask();
    }
  }

  return MiniMC::Support::ExitCodes::AllGood;

}

static CommandRegistrar intp_reg ("intp",intp_main,"Running the interpreter on the given configuration. ", addOptions);
