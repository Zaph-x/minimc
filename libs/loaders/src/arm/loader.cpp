#include "loaders/loader.hpp"
#include "../../../leg-assembly-loader/include/parser.hpp"
#include "model/cfg.hpp"
#include <fstream>
#include <sstream>

namespace MiniMC {
  namespace Loaders {

    //Renamed for multiple definitions reasons.
    MiniMC::Model::Function_ptr createARMEntryPoint(std::size_t stacksize,
                                                    MiniMC::Model::Program& program,
                                                    MiniMC::Model::Function_ptr function,
                                                    std::vector<MiniMC::Model::Value_ptr>&&) {
      throw MiniMC::Support::ConfigurationException ("Loader does not support defining entry points");
    }


    class ARMLoader : public Loader {
    public:
      ARMLoader(std::size_t stacksize, Model::TypeFactory_ptr &tfac, Model::ConstantFactory_ptr &cfac)
          : Loader(tfac, cfac),stacksize(stacksize) {}

      LoadResult loadFromFile(const std::string &file) override {
        auto program =std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        std::fstream str;
        str.open(file);
        ARM::Parser::Parser armParser = ARM::Parser::Parser();
        armParser.set_up(file);

        return {.program = program,
                .entrycreator = std::bind(createARMEntryPoint,stacksize,std::placeholders::_1, std::placeholders::_2,std::placeholders::_3)
        };
      }
      LoadResult loadFromString(const std::string &inp) override {
        auto program = std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        std::stringstream str;
        str.str(inp);
        ARM::Parser::Parser parser = ARM::Parser::Parser();
        ARM::Lexer::Lexer lexer = ARM::Lexer::Lexer(str);
        parser.set_up(lexer);
        return {.program = program,
                .entrycreator = std::bind(createARMEntryPoint,stacksize,std::placeholders::_1, std::placeholders::_2,std::placeholders::_3)
        };
      }

    private:
      std::size_t stacksize;

    };


    //Taken more or less directly from the llvm loader. Since their roles are similar.
    class ARMLoadRegistrar : public LoaderRegistrar {
    public:
      ARMLoadRegistrar()
          : LoaderRegistrar("ARM", {IntOption{.name = "stack",
                                              .description = "StackSize",
                                              .value = 200}}) {}

      Loader_ptr makeLoader(MiniMC::Model::TypeFactory_ptr &tfac,
                            Model::ConstantFactory_ptr cfac) override {
        auto stacksize = std::visit(
            [](auto &t) -> std::size_t {
              using T = std::decay_t<decltype(t)>;
              if constexpr (std::is_same_v<T, IntOption>)
                return t.value;
              else {
                throw MiniMC::Support::Exception("Horrendous error");
              }
            },
            getOptions().at(0));
        return std::make_unique<ARMLoader>(stacksize,tfac, cfac);
      }
    };

    static ARMLoadRegistrar ARMloadregistrar;

  } // namespace Loaders
} // namespace MiniMC
