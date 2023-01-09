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
        /* Ting der ordnes i Lars Bo's loader
        functions();
         Lav en Program->addFunction med navn, registre
        entrypoints();
        heap();
        initialiser();
         */
        functions();

        return program;
      }

      void functions() {
        auto functions = parser.get_program()->get_functions();

        for (unsigned int i = 0; i < functions.size(); i++) {
          std::shared_ptr<ARM::Parser::Function> func = functions[i];
          const std::string& name = func->get_name();
          auto params = std::vector<Model::Register_ptr>();
          auto instructions = func->get_instructions();
          auto regDescr = MiniMC::Model::RegisterDescr(MiniMC::Model::Symbol::from_string(name));
          for (unsigned int j = 0; j < instructions.size(); j++) {
            if (instructions[j]->get_args().empty()) {
              continue;
            }
            for (auto in : instructions[j]->get_args()) {
              // Find ud af om arg er register node

              if (std::dynamic_pointer_cast<ARM::Parser::Register>(in) != nullptr) {
                // Register node to minimc register helper in the future?
                auto reg = std::dynamic_pointer_cast<ARM::Parser::Register>(in);
                // If getregisters contains register, continue.
                  auto res = regDescr.addRegister(MiniMC::Model::Symbol::from_string(reg->get_name()), program->getTypeFactory().makeIntegerType(32));
                }
              }
            }

            const Model::Type_ptr retType = program->getTypeFactory().makeVoidType();
            MiniMC::Model::RegisterDescr_uptr&& registerdescr = std::make_unique<>()regDescr;
            //                CFA&& cfg
            //
            //            program->addFunction(name, params, retType, registerdescr, cfg)
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
