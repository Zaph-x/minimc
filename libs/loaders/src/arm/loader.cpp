
#include "loaders/loader.hpp"
#include "model/cfg.hpp"

#include <fstream>
#include <sstream>


namespace MiniMC {
  namespace Loaders {

    MiniMC::Model::Function_ptr createEntryPoint(MiniMC::Model::Program& program, MiniMC::Model::Function_ptr function, std::vector<MiniMC::Model::Value_ptr>&&) {
      throw MiniMC::Support::ConfigurationException ("Loader does not support defining entry points");
    }


    class ARMLoader : public Loader {
    public:
      ARMLoader(Model::TypeFactory_ptr &tfac, Model::ConstantFactory_ptr &cfac)
          : Loader(tfac, cfac) {}

      LoadResult loadFromFile(const std::string &file) override {
        auto program =std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        std::fstream str;
        str.open(file);
        Parser parser = Parser(str, *tfactory, *cfactory, program);
        parser.run();
        ARM::Parser armParser = ARM::Parser(str, *tfactory, *cfactory, program);


        return {.program = program,
                .entrycreator = createEntryPoint
        };
      }
      LoadResult loadFromString(const std::string &inp) override {
        auto program = std::make_shared<MiniMC::Model::Program>(tfactory, cfactory);
        std::stringstream str;
        str.str(inp);
        Parser parser = Parser(str, *tfactory, *cfactory, program);
        parser.run();
        return {.program = program,
                .entrycreator = createEntryPoint
        };
      }
    };

    class ARMLoadRegistrar : public LoaderRegistrar {
    public:
      ARMLoadRegistrar()
          : LoaderRegistrar("MMC", {IntOption{.name = "stack",
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
        return std::make_unique<ARMLoader>(tfac, cfac);
      }
    };

    static ARMLoadRegistrar ARMloadregistrar;

  } // namespace Loaders
} // namespace MiniMC
