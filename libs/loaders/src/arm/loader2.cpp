#include "loaders/loader.hpp"
#include "parser.hpp"
#include "model/cfg.hpp"
#include <fstream>
#include <sstream>


namespace MiniMC {
  namespace Loaders {


    class ARMLoader : public Loader {
    public:
      ARMLoader(Model::TypeFactory_ptr &tfac, Model::ConstantFactory_ptr &cfac)
          : Loader(tfac, cfac) {}

      LoadResult loadFromFile(const std::string &file) override {
        auto program = std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        ARM::Parser::Parser parser = ARM::Parser::Parser();
        parser.set_up(file);
        parser.assign_program();
        convert(parser, program);

        return {.program = program
        };
      }
      LoadResult loadFromString(const std::string &inp) override {
        auto program = std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        std::stringstream str;
        str.str(inp);
        ARM::Lexer::Lexer lexer = ARM::Lexer::Lexer(str);
        ARM::Parser::Parser parser = ARM::Parser::Parser();
        parser.set_up(lexer);
        auto prgm = parser.get_program();

        convert(parser, program);
        return {.program = program
        };
      }

      MiniMC::Model::Program_ptr convert(ARM::Parser::Parser par, MiniMC::Model::Program_ptr prog){

        parser = par;
        program = prog;
        functions();
        cfa();
        entrypoints(); //TODO: entrypoints
        heap(); // TODO: Finish heap for labels
        initialiser(); //TODO: initialiser

        return program;
      }

      void functions() {
        auto functions = parser.get_program()->get_functions();

        for (unsigned int i = 0; i < functions.size(); i++) {
          //Discover registers used.
          std::shared_ptr<ARM::Parser::Function> func = functions[i];

          int function_offset = 0;
          int last_offset = 0;
          int current_offset = 0;
          if (func->get_instructions()[2]->get_name() == ".cfi_def_cfa_offset") {
            auto cast_value = func->get_instructions()[2]->get_args()[0];
            auto offset = std::dynamic_pointer_cast<ARM::Parser::ImmediateValue>(cast_value);
            if (offset != nullptr) {
              function_offset = std::stoi(offset->get_value());
              last_offset = function_offset;
            }
          }


          const std::string& name = func->get_name();
          auto params = std::vector<Model::Register_ptr>();
          auto instructions = func->get_instructions();
          auto regDescr = MiniMC::Model::RegisterDescr(MiniMC::Model::Symbol::from_string(name));
          for (unsigned int j = 0; j < instructions.size(); j++) {
            //convertInstruction(instructions[j]);
            if (instructions[j]->get_args().empty()) {
              continue;
            }
            for (auto in : instructions[j]->get_args()) {
              // Find ud af om arg er register node

              if (std::dynamic_pointer_cast<ARM::Parser::Register>(in) != nullptr) {
                // Register node to minimc register helper in the future?
                auto reg = std::dynamic_pointer_cast<ARM::Parser::Register>(in);
                auto registerList = regDescr.getRegisters();

                // Check if register is already in register list
                if (std::find_if(registerList.begin(), registerList.end(), [&reg](const Model::Register_ptr& r) {
                  return r->getSymbol().getName() == reg->get_name();
                }) != registerList.end()) {
                  continue;
                }


                //Traversal of instructions to find latest offset

                auto res = regDescr.addRegister(MiniMC::Model::Symbol::from_string(reg->get_name()), program->getTypeFactory().makeIntegerType(32));
                }
              }
            }

            const Model::Type_ptr retType = program->getTypeFactory().makeVoidType();
            //MiniMC::Model::RegisterDescr_uptr&& registerdescr = std::make_unique<MiniMC::Model::RegisterDescr>(regDescr);
            //                CFA&& cfg
            //
            //            program->addFunction(name, params, retType, registerdescr, cfg)
          }
        }

        void cfa() {

        }

        void entrypoints() {
          // TODO
        }

          void heap() {
            auto global_vars = parser.get_program()->get_global_variables();
            for (std::shared_ptr<ARM::Parser::Variable> var: global_vars) {
                auto size = var->get_size();
                program->getHeapLayout().addBlock(size);

                //TODO: Take care of the labels

            }
          }

          void initialiser() {
            // Initialiser is an instruction stream with a store instruction for each heap element.
            // Presumably since it should be run before the entrypoint.
            std::vector<MiniMC::Model::Instruction> instructions;

            //Handle global variables
            auto global_vars = parser.get_program()->get_global_variables();
            for (std::shared_ptr<ARM::Parser::Variable> var: global_vars) {
                auto size = var->get_size();
                auto name = var->get_name();

            // Handle functions
                //TODO: Take care of the labels

            }
          }

          MiniMC::Model::Instruction convertInstruction(std::shared_ptr<ARM::Parser::Instruction> instruction){
              auto instr_args = instruction->get_args();
              if (instruction->get_name() == "add"){
                  auto instr_code = MiniMC::Model::InstructionCode::Add;
                  if (instr_args.size() != 3){
                    throw std::runtime_error("Invalid number of arguments for add instruction");
                  }
                  auto arg0 = std::dynamic_pointer_cast<ARM::Parser::Register>(instr_args[0]);
                  auto arg1 = std::dynamic_pointer_cast<ARM::Parser::Register>(instr_args[1]);

                  if (arg0 == nullptr || arg1 == nullptr){
                    throw std::runtime_error("Invalid arguments for add instruction");
                  }
//                  auto res = MiniMC::Model::Value_ptr
//                  auto res = MiniMC::Model::Register(MiniMC::Model::Symbol::from_string(arg0->get_name()), owner);
//                  auto op1 = MiniMC::Model::Register(MiniMC::Model::Symbol::from_string(arg1->get_name()), owner);
//
//                  MiniMC::Model::TACContent content ={
//                      .res = res,
//                            .op1 = op1;
//                  };

                  // Arg 1 and 2 are registers, check if 3 is as well.
                  if (std::dynamic_pointer_cast<ARM::Parser::ImmediateValue>(instr_args[2])){

                  }
                  for(std::shared_ptr<ARM::Parser::Node> operand:instruction->get_args()){

                  }
              }else{

              }

          }

    private:
      ARM::Parser::Parser parser;
      MiniMC::Model::Program_ptr program;

    };

    class ARMLoadRegistrar : public LoaderRegistrar {
    public:
      ARMLoadRegistrar()
          : LoaderRegistrar("ARM", {}) {}

      Loader_ptr makeLoader(MiniMC::Model::TypeFactory_ptr &tfac,
                            Model::ConstantFactory_ptr cfac) override {
        return std::make_unique<ARMLoader>(tfac, cfac);
      }
    };

    static ARMLoadRegistrar ARMloadregistrar;

  } // namespace Loaders
} // namespace MiniMC
