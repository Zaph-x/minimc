#include "parser.hpp"
#include "lexer.hpp"
#include "loaders/loader.hpp"
#include "model/cfg.hpp"

#include <fstream>
#include <sstream>

namespace MiniMC {
  namespace Loaders {

    // Definition stub might not be the correct approach, but does contain sizes of types.
    MiniMC::Model::TypeID getTypeID(ARM::Parser::DefinitionStub type);



    class ARMLoader : public Loader {
    public:
      ARMLoader(Model::TypeFactory_ptr &tfac, Model::ConstantFactory_ptr &cfac)
          : Loader(tfac, cfac) {}

      LoadResult loadFromFile(const std::string &file) override {
        auto program =std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        ARM::Parser::Parser parser = ARM::Parser::Parser();
        parser.set_up(file);
        parser.assign_program();




        return {.program = program,
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

        return {.program = ARMtoMiniMC(prgm, tfactory, cfactory)
        };
      }

      virtual MiniMC::Model::Program_ptr ARMtoMiniMC(const std::shared_ptr<ARM::Parser::Program> program, MiniMC::Model::TypeFactory_ptr& tfac, MiniMC::Model::ConstantFactory_ptr& cfac){
        auto prgm = std::make_shared<MiniMC::Model::Program>(tfac, cfac);

        return prgm;
      }

    };




    class ARMLoadRegistrar : public LoaderRegistrar {
    public:
      ARMLoadRegistrar()
          : LoaderRegistrar("ARM", {IntOption{.name="stack",
                                              .description ="StackSize",
                                              .value = 200
                                    },
                                    VecStringOption {.name = "entry",
                                                    .description="Entry point function",
                                                    .value = {}
                                    }}) {}

      Loader_ptr makeLoader(MiniMC::Model::TypeFactory_ptr &tfac,
                            Model::ConstantFactory_ptr cfac) override {

        auto stacksize = 2;



        return std::make_unique<ARMLoader>(tfac, cfac);
      }
    };

    static ARMLoadRegistrar ARMloadregistrar;

  } // namespace Loaders
} // namespace MiniMC
