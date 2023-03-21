//#include "parser.hpp"
//#include "model/cfg.hpp"
//#include "loaders/loader.hpp"
//
//
//namespace MiniMC {
//  namespace Loaders{
//    class ARMToMiniMC{
//    public:
//      ARMToMiniMC(std::shared_ptr<ARM::Parser::Program> prgm,
//                  MiniMC::Model::ConstantFactory_ptr cfact,
//                  MiniMC::Model::TypeFactory_ptr tfact): armprogram(prgm), cfact(cfact), tfact(tfact){
//        program = std::make_shared<MiniMC::Model::Program>(cfact, tfact);
//      }
//
//      virtual MiniMC::Model::Program_ptr convert(){
//
//
//        functions();
//        entrypoints();
//        heap();
//        initialiser();
//
//
//        return program;
//      }
//
//    private:
//      std::shared_ptr<ARM::Parser::Program> armprogram;
//      std::shared_ptr<MiniMC::Model::Program> program;
//      std::shared_ptr<Model::ConstantFactory> cfact;
//      std::shared_ptr<MiniMC::Model::TypeFactory> tfact;
//    };
//
//    };
//  }
//}