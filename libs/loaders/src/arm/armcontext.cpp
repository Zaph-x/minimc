//#include "armcontext.hpp"
//#include "model/variables.hpp"
//#include "parser.hpp"
//
//#define ERROR(msg) error(__LINE__, msg)
//
//
//void error(int line, const std::string &msg) {
//  std::cerr << "ARM Loader error: " << msg << std::endl
//            << "    in " << __FILE__ << ":" << line << std::endl;
//  throw std::exception();
//}
//
//
//
//namespace MiniMC {
//  namespace Loaders {
//
//    MiniMC::Model::TypeID getTypeID(ARM::Parser::DefinitionStub type) {
//
//      switch (type.get_size()){
//        case 1:
//          //TODO Handle Char vs Bool
//            return MiniMC::Model::TypeID::I8;
//        case 2:
//            return MiniMC::Model::TypeID::I16;
//        case 4:
//          return MiniMC::Model::TypeID::I32;
//        case 8:
//          return MiniMC::Model::TypeID::I64;
//        case 16:
//          //Probably not relevant
//          ERROR("I128 is not supported yet.");
//      }
////      if (type->isVoidTy()) {
////        return MiniMC::Model::TypeID::Void;
////      } else if (type->isPointerTy()) {
////        return MiniMC::Model::TypeID::Pointer;
////      }
//
//
////      LLVM supports arbitrary bit widths.
////      if (type->isDecNumber()) {
////        unsigned bits = type->getIntegerBitWidth();
////        if (bits == 1) {
////          return MiniMC::Model::TypeID::Bool;
////        } else if (bits <= 8)
////          return MiniMC::Model::TypeID::I8;
////        else if (bits <= 16)
////          return MiniMC::Model::TypeID::I16;
////        else if (bits <= 32)
////          return MiniMC::Model::TypeID::I32;
////        else if (bits <= 64)
////          return MiniMC::Model::TypeID::I64;
////      }
//
//      //We cannot parse structs at the moment.
////      else if (type->isStructTy()) {
////        return MiniMC::Model::TypeID::Struct;
////      }
//
////      else if (type->isArrayTy()) {
////        return MiniMC::Model::TypeID::Array;
////      }
//
//      throw MiniMC::Support::Exception("Unknown Type");
//    }
//
//    MiniMC::Model::Type_ptr GLoadContext::getType (ARM::Parser::DefinitionStub type ) {
//
//      auto type_id = getTypeID (type);
//      switch  (type_id) {
//        case MiniMC::Model::TypeID::Bool:
//          return tfact.makeBoolType ();
//        case MiniMC::Model::TypeID::I8:
//          return tfact.makeIntegerType (8);
//        case MiniMC::Model::TypeID::I16:
//          return tfact.makeIntegerType (16);
//        case MiniMC::Model::TypeID::I32:
//          return tfact.makeIntegerType (32);
//        case MiniMC::Model::TypeID::I64:
//          return tfact.makeIntegerType (64);
//        case MiniMC::Model::TypeID::Pointer:
//          return tfact.makePointerType ();
//        case MiniMC::Model::TypeID::Array:
//          return tfact.makeArrayType (computeSizeInBytes (type));
//        case MiniMC::Model::TypeID::Struct:
//          return tfact.makeStructType (computeSizeInBytes (type));
//        case MiniMC::Model::TypeID::Void:
//          return tfact.makeVoidType ();
//        case MiniMC::Model::TypeID::Float:
//          return tfact.makeFloatType ();
//        case MiniMC::Model::TypeID::Double:
//          return tfact.makeDoubleType ();
//
//        default:
//          throw MiniMC::Support::Exception ("Unsupported type");
//      }
//    }
//      // Computing size of structs and arrays.
////    MiniMC::BV32 GLoadContext::computeSizeInBytes (llvm::Type* ty ) {
////      if (ty->isArrayTy()) {
////        return ty->getArrayNumElements() * computeSizeInBytes(ty->getArrayElementType());
////      }
////
////      else if (ty->isStructTy()) {
////        auto it = static_cast<llvm::StructType*>(ty);
////        std::size_t size = 0;
////        for (std::size_t i = 0; i < it->getNumElements(); ++i) {
////          size += computeSizeInBytes(it->getElementType(i));
////        }
////        return size;
////      }
////
////      else if (ty->isIntegerTy ()) {
////        return tfact.makeIntegerType (ty->getIntegerBitWidth())->getSize ();
////      }
////      else if (ty->isPointerTy ()) {
////        return tfact.makePointerType ()->getSize ();
////      }
////      throw MiniMC::Support::Exception("Can't calculate size of type");
////    }
//
//
//    MiniMC::Model::Value_ptr GLoadContext::findValue(const llvm::Value* val) {
//      auto constant = llvm::dyn_cast<llvm::Constant>(val);
//      if (constant) {
//        assert(constant);
//        auto ltype = constant->getType();
//
//        auto cst_undef = llvm::dyn_cast<llvm::UndefValue>(val);
//        if (cst_undef) {
//          auto type = getTypeID(constant->getType());
//          return cfact.makeUndef(type);
//        } else if (ltype->isIntegerTy()) {
//          const llvm::ConstantInt* csti = llvm::dyn_cast<const llvm::ConstantInt>(constant);
//          if (csti) {
//            auto type = getTypeID(csti->getType());
//            auto cst = cfact.makeIntegerConstant(csti->getZExtValue(), type);
//            return cst;
//          }
//        } else if (ltype->isStructTy() || ltype->isArrayTy()) {
//
//          if (auto cstAggr = llvm::dyn_cast<llvm::ConstantDataSequential>(val)) {
//            MiniMC::Model::ConstantFactory::aggr_input vals;
//            const size_t oper = cstAggr->getNumElements();
//            for (size_t i = 0; i < oper; ++i) {
//              auto elem = cstAggr->getElementAsConstant(i);
//              auto nconstant = findValue(elem);
//              assert(nconstant->isConstant());
//              vals.push_back(std::static_pointer_cast<MiniMC::Model::Constant>(nconstant));
//            }
//            auto cst = cfact.makeAggregateConstant(vals, ltype->isArrayTy());
//
//            return cst;
//          }
//
//          if (auto cstAggr2 = llvm::dyn_cast<llvm::ConstantAggregate>(val)) {
//            MiniMC::Model::ConstantFactory::aggr_input const_vals;
//
//            const size_t oper = cstAggr2->getNumOperands();
//            for (size_t i = 0; i < oper; ++i) {
//              auto elem = cstAggr2->getOperand(i);
//              auto nconstant =findValue (elem);
//              const_vals.push_back(std::static_pointer_cast<MiniMC::Model::Constant>(nconstant));
//            }
//            auto cst = cfact.makeAggregateConstant(const_vals, ltype->isArrayTy());
//            return cst;
//          }
//          // assert(false && "FAil");
//
//        } else if (llvm::isa<llvm::Function>(val) ||
//                   llvm::isa<llvm::GlobalVariable>(val)) {
//          return values.at(val);
//        } else if (const llvm::BlockAddress* block = llvm::dyn_cast<const llvm::BlockAddress>(val)) {
//          return values.at(block->getBasicBlock());
//        }
//
//        else if (ltype->isPointerTy()) {
//          constant->print(llvm::errs(), true);
//          throw MiniMC::Support::Exception("Pointer Not Quite there");
//        }
//
//        MiniMC::Support::Localiser local("LLVM '%1%' not implemented");
//        std::string str;
//        llvm::raw_string_ostream output(str);
//        val->print(output);
//        throw MiniMC::Support::Exception(local.format(str));
//      }
//
//      else {
//        return values.at (val);
//      }
//    }
//
//
//  } // namespace Loaders
//} // namespace MiniMC
