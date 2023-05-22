#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <tuple>
#include "loaders/loader.hpp"

#include <boost/algorithm/string.hpp>

#include <boost/preprocessor.hpp>

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
  case elem : return BOOST_PP_STRINGIZE(elem);

#define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)                \
  enum name {                                                               \
    BOOST_PP_SEQ_ENUM(enumerators)                                        \
  };                                                                        \
  \
  constexpr std::string enum_string(name v)                                    \
{                                                                         \
  switch (v)                                                            \
  {                                                                     \
    BOOST_PP_SEQ_FOR_EACH(                                            \
        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
        name,                                                         \
        enumerators                                                   \
        )                                                                 \
    default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";         \
  }                                                                     \
}

DEFINE_ENUM_WITH_STRING_CONVERSIONS(SmvType, (Bool)(Int)(Real)(Enum)(Array)(Record)(Ptr)(Unknown));

class SmvSpec;


class Spec {
  public:
    virtual std::string to_string() = 0;
    virtual std::string write() = 0;
};

class ValueSpec : public Spec {
  public:
    virtual std::string to_string() = 0;
    virtual bool is_register() = 0;

  private:
    std::string identifier;
};

class RegisterSpec : ValueSpec {
  public:
    RegisterSpec(std::string identifier, SmvType type) 
      : identifier(identifier), type(type) {}
    RegisterSpec(MiniMC::Model::Value_ptr value) {
      identifier = std::dynamic_pointer_cast<MiniMC::Model::Register>(value)->getSymbol().getName();
      boost::replace_all(identifier, ".", "_");
      boost::replace_all(identifier, ":", "_");
      boost::replace_all(identifier, "tmp", "reg");
    }
    
    void set_type(SmvType type) {
      this->type = type;
    }

    constexpr std::string to_string() {return "Register: " + identifier + " : " + enum_string(type) + ";";}
    constexpr bool is_register() {return true;}
    constexpr std::string write() {return "VAR " + identifier + " : TODO:WRITE_HERE;\n";}

  private:
    std::string identifier;
    SmvType type = SmvType::Unknown;
};

class ImmediateValueSpec : public ValueSpec {
  public:

    ImmediateValueSpec(std::string value, SmvType type) 
      : value(value) {}
    ImmediateValueSpec(MiniMC::Model::Value_ptr& value) {
    }

    std::string to_string() {
      return value;
    }
    bool is_register() {
      return false;
    }

  private:
    std::string value;
};

class SmvVarSpec : public Spec {
  public:
    SmvVarSpec(std::string identifier, SmvType type) 
      : identifier(identifier), type(type) {}
    std::string write_init() {
      if (values.size() == 0) {
        return "";
      }
      return "ASSIGN init(" + identifier + ") := " + values[0] + ";";
    }

    std::string to_string() {
      std::string result = "Variable: " + identifier + " : " + enum_string(type) + " : ";
      for (auto& value : values) {
        result += value + ", ";
      }
      return result;
    }

    std::string write() {
      return "";
    }

  private:
    std::string identifier;
    SmvType type;
    std::vector<std::string> values;

};

class InstructionSpec : public Spec {
  public:
    std::string to_string() {return "UNFINISHED INSTRUCTION";}
    MiniMC::Model::Program_ptr prg;
    std::string write() = 0;
    std::string operation;

    InstructionSpec(MiniMC::Model::Program_ptr prg){
      this->prg = prg;
    };

    void set_prg(MiniMC::Model::Program_ptr prg) {
      this->prg = prg;
    }

    SmvType derive_type(std::string fallback) {
      if (operation.starts_with("ICMP")) {
        return SmvType::Bool;
      } else if (operation == "SExt" || operation.starts_with("Add") || operation.starts_with("Sub") || operation.starts_with("Mul") || operation.starts_with("Div")) {
        return SmvType::Int;
      } else if (operation.starts_with("FADD") || operation.starts_with("FSUB") || operation.starts_with("FMUL") || operation.starts_with("FDIV")) {
        return SmvType::Real;
      } else if (operation.ends_with("Ptr")) {
        return SmvType::Ptr;
      }

      if (fallback == "") {
        return SmvType::Unknown;
      } else if (boost::contains(fallback, "Int")) {
        return SmvType::Int;
      } else if (boost::contains(fallback, "Float")) {
        return SmvType::Real;
      } else if (boost::contains(fallback, "Bool")) {
        return SmvType::Bool;
      } else if (boost::contains(fallback, "Ptr")) {
        return SmvType::Ptr;
      } else {
        return SmvType::Unknown;
      }


      return SmvType::Unknown;
    }

    void address_heap(MiniMC::Model::Value_ptr content, std::string& result, MiniMC::Model::Program_ptr prg) {
      if (content->isConstant()) {
        auto constant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(content);
        if (constant->isPointer()) {
          auto ptr = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(constant);
          if (ptr->getValue().segment == (int)'H') {
            std::cerr << ptr->getType() << std::endl;
            // TODO This does not get any value currently. We want to get the register name we are pointing to.
            if (ptr->isRegister()) {
              if (prg->getHeapLayout().getSize() != 0){
                auto a = prg->getHeapLayout().getIndex(ptr->getValue().base);
                result = "heap[" + std::to_string(ptr->getValue().base) + ":" + std::to_string(ptr->getValue().offset) + "]";

              }
            } else if (ptr->isConstant() && prg->getHeapLayout().getSize() != 0) {
              if (prg->getHeapLayout().getSize() != 0){
                auto a = prg->getHeapLayout().getIndex(ptr->getValue().base + ptr->getValue().offset);
                result = "heap[" + std::to_string(ptr->getValue().base) + ":" + std::to_string(ptr->getValue().offset) + "]";

              }
            } else {

              result = "unknown";
            }
          } else {
            result = content->string_repr();
          }
        } else {
          result = content->string_repr();
        }
      } else {
        result = content->string_repr();
      }
    }
};

class TACInstruction : public InstructionSpec {
  public:
    TACInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instr)
        : InstructionSpec(prg), instruction(instr), result_register(std::get<MiniMC::Model::TACContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      auto lhs_content = std::get<MiniMC::Model::TACContent>(instruction.getContent()).op1;
      auto rhs_content = std::get<MiniMC::Model::TACContent>(instruction.getContent()).op2;

      address_heap(lhs_content, lhs, this->prg);
      address_heap(rhs_content, rhs, this->prg);

      if (lhs_content->isConstant()) {
        auto paramsConstant = std::dynamic_pointer_cast<MiniMC::Model::Constant>(lhs_content);
        if (paramsConstant->isPointer() && paramsConstant->getSize() == 8L) {
          auto paramsPointer = std::dynamic_pointer_cast<MiniMC::Model::TConstant<MiniMC::pointer64_t>>(paramsConstant);
          auto paramsPointerValue = paramsPointer->getValue();
          // Pointer struct segments use ascii values to describe the content. 72 = H = Heap. 70 = F = Function and so on.
          if (paramsPointerValue.segment == (int)'H') {
            for (auto initInstr : prg->getInitialiser()) {
              if (instr.getOpcode() == MiniMC::Model::InstructionCode::Store) {
                auto content = get<12>(instr.getContent());
                std::cerr << content.addr << std::endl;
              }
            }
          }
        }
      }

      result_register.set_type(derive_type(lhs));

    }

    std::string to_string() {
      return "TAC Instruction: " + operation + " (" + result_register.to_string() + ") <- " + lhs + " " + rhs + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string lhs;
    std::string rhs;

};

class UnaryInstruction : public InstructionSpec {
  public:
    UnaryInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instr) : InstructionSpec(prg), instruction(instr), result_register(std::get<MiniMC::Model::UnaryContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      lhs = std::get<MiniMC::Model::UnaryContent>(instruction.getContent()).op1->string_repr();
      result_register.set_type(derive_type(lhs));
    }

    std::string to_string() {
      return "Unary Instruction: " + operation + " (" + result_register.to_string() + ") <- " + lhs + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string lhs;
};

class BinaryInstruction : public InstructionSpec {
  public:
    BinaryInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      lhs = std::get<MiniMC::Model::BinaryContent>(instruction.getContent()).op1->string_repr();
      rhs = std::get<MiniMC::Model::BinaryContent>(instruction.getContent()).op2->string_repr();
    }

    std::string to_string() {
      return "Binary Instruction: " + operation + " " + lhs + " <-> " + rhs + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string lhs;
    std::string rhs;
};

class ExtractInstruction : public InstructionSpec {
  public:
    ExtractInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::ExtractContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      aggregate = std::get<MiniMC::Model::ExtractContent>(instruction.getContent()).aggregate->string_repr();
      offset = std::get<MiniMC::Model::ExtractContent>(instruction.getContent()).offset->string_repr();
    }

    std::string to_string() {
      return "Extract Instruction: " + operation + " (" + result_register.to_string() + ") " + aggregate + " | " + offset + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string aggregate;
    std::string offset;
};

class InsertInstruction : public InstructionSpec {
  public:
    InsertInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::InsertContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      aggregate = std::get<MiniMC::Model::InsertContent>(instruction.getContent()).aggregate->string_repr();
      offset = std::get<MiniMC::Model::InsertContent>(instruction.getContent()).offset->string_repr();
      value = std::get<MiniMC::Model::InsertContent>(instruction.getContent()).insertee->string_repr();
    }

    std::string to_string() {
      return "Insert Instruction: " + operation + " (" + result_register.to_string() + ") " + aggregate + " | " + offset + " <- " + value + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string aggregate;
    std::string offset;
    std::string value;
};

class PtrAddInstruction : public InstructionSpec {
  public:
    PtrAddInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::PtrAddContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      result_register.set_type(SmvType::Ptr);
      ptr = std::get<MiniMC::Model::PtrAddContent>(instruction.getContent()).ptr->string_repr();
      skip_size = std::get<MiniMC::Model::PtrAddContent>(instruction.getContent()).skipsize->string_repr();
      nb_skips = std::get<MiniMC::Model::PtrAddContent>(instruction.getContent()).nbSkips->string_repr();
    }

    std::string to_string() {
      return "PtrAdd Instruction: " + operation + " (" + result_register.to_string() + ")";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string ptr;
    std::string skip_size;
    std::string nb_skips;
};

class ExtendObjectInstruction : public InstructionSpec {
  public:
    ExtendObjectInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::ExtendObjContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      result_register.set_type(SmvType::Ptr);
      object = std::get<MiniMC::Model::ExtendObjContent>(instruction.getContent()).object->string_repr();
      size = std::get<MiniMC::Model::ExtendObjContent>(instruction.getContent()).size->string_repr();
    }

    std::string to_string() {
      return "ExtendObject Instruction: " + operation + " (" + result_register.to_string() + ") <- " + object + ":+" + size + " ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string object;
    std::string size;
};

class MallocInstruction : public InstructionSpec {
  public:
    MallocInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      object = std::get<MiniMC::Model::MallocContent>(instruction.getContent()).object->string_repr();
      size = std::get<MiniMC::Model::MallocContent>(instruction.getContent()).size->string_repr();

    }

    std::string to_string() {
      return "Malloc Instruction: " + operation + " " + object + ":+" + size;
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string object;
    std::string size;
};

class FreeInstruction : public InstructionSpec {
  public:
    FreeInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      object = std::get<MiniMC::Model::FreeContent>(instruction.getContent()).object->string_repr();
    }

    std::string to_string() {
      return "Free Instruction: " + operation + " " + object;
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string object;
};

class NonDetInstruction : public InstructionSpec {
  public:
    NonDetInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::NonDetContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      result_register.set_type(SmvType::Int);
      min = std::get<MiniMC::Model::NonDetContent>(instruction.getContent()).min->string_repr();
      max = std::get<MiniMC::Model::NonDetContent>(instruction.getContent()).max->string_repr();
      arguments = std::get<MiniMC::Model::NonDetContent>(instruction.getContent()).arguments;
    }

    std::string to_string() {
      return "NonDet Instruction: " + operation + " (" + result_register.to_string() + ") <- (" + min + " - " + max + ") [" + arguments + "] ";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string min;
    std::string max;
    std::string arguments;
};

class AssumeInstruction : public InstructionSpec {
  public:
    AssumeInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      condition = std::get<MiniMC::Model::AssertAssumeContent>(instruction.getContent()).expr->string_repr();
    }

    std::string to_string() {
      return "Assume Instruction: " + operation + " " + condition;
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string condition;
};

class LoadInstruction : public InstructionSpec {
  public:
    LoadInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::LoadContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      address = std::get<MiniMC::Model::LoadContent>(instruction.getContent()).addr->string_repr();
    }

    std::string to_string() {
      return "Load Instruction: " + operation + " (" + result_register.to_string() + ") <- " + address;
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string address;
};

class StoreInstruction : public InstructionSpec {
  public:
    StoreInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      address = std::get<MiniMC::Model::StoreContent>(instruction.getContent()).addr->string_repr();
      value = std::get<MiniMC::Model::StoreContent>(instruction.getContent()).storee->string_repr();
      variable_name = std::get<MiniMC::Model::StoreContent>(instruction.getContent()).variableName;
    }

    std::string to_string() {
      return "Store Instruction: " + operation + " " + address + " <- " + value + " (" + variable_name + ")";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string address;
    std::string value;
    std::string variable_name;
};

class ReturnInstruction : public InstructionSpec {
  public:
    ReturnInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      value = std::get<MiniMC::Model::RetContent>(instruction.getContent()).value->string_repr();
    }

    std::string to_string() {
      return "Return Instruction: " + operation + " " + value;
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    std::string value;
};

class CallInstruction : public InstructionSpec {
  public:
    CallInstruction(MiniMC::Model::Program_ptr prg, MiniMC::Model::Instruction instruction) : InstructionSpec(prg), instruction(instruction), result_register(std::get<MiniMC::Model::CallContent>(instruction.getContent()).res) {
      operation_ident = (std::string)std::visit([](const auto& value) 
          { return typeid(value).name(); }, std::variant(instruction.getContent()));
      std::ostringstream oss;
      oss << instruction.getOpcode();
      operation = oss.str();
      auto content = std::get<MiniMC::Model::CallContent>(instruction.getContent());
      function_name = content.function->string_repr();
      for (auto& arg : content.params) {
        params.push_back(arg->string_repr());
      }
      argument = content.argument;
      result_register.set_type(SmvType::Int);
    }

    std::string to_string() {
      std::string params_string = "";
      for (auto& param : params) {
        params_string += param + ", ";
      }
      return "Call Instruction: " + operation + " (" + result_register.to_string() + ") <- " + function_name + "(" + params_string + argument + ")";
    }

    std::string write() {
      return "";
    }

  private:
    MiniMC::Model::Instruction instruction;
    std::string operation_ident;
    RegisterSpec result_register;
    std::string function_name;
    std::vector<std::string> params;
    std::string argument;
};


inline std::shared_ptr<InstructionSpec> make_instruction(MiniMC::Model::Instruction instruction, MiniMC::Model::Program_ptr program) {
  int index = instruction.getContent().index();
  std::shared_ptr<InstructionSpec> instr = nullptr;
  if (index == 0) { // TAC
    instr = std::make_shared<TACInstruction>(program, instruction);
  } else if (index == 1) { // Unary
    instr = std::make_shared<UnaryInstruction>(program, instruction);
  } else if (index == 2) { // Binary
    instr = std::make_shared<BinaryInstruction>(program, instruction);
  } else if (index == 3) { // Extract
    instr = std::make_shared<ExtractInstruction>(program, instruction);
  } else if (index == 4) { // Insert
    instr = std::make_shared<InsertInstruction>(program, instruction);
  } else if (index == 5) { // PtrAdd
    instr = std::make_shared<PtrAddInstruction>(program, instruction);
  } else if (index == 6) { // ExtendObject
    instr = std::make_shared<ExtendObjectInstruction>(program, instruction);
  } else if (index == 7) { // Malloc
    instr = std::make_shared<MallocInstruction>(program, instruction);
  } else if (index == 8) { // Free
    instr = std::make_shared<FreeInstruction>(program, instruction);
  } else if (index == 9) { // NonDet
    instr = std::make_shared<NonDetInstruction>(program, instruction);
  } else if (index == 10) { // Assume
    instr = std::make_shared<AssumeInstruction>(program, instruction);
  } else if (index == 11) { // Load
    instr = std::make_shared<LoadInstruction>(program, instruction);
  } else if (index == 12) { // Store
    instr = std::make_shared<StoreInstruction>(program, instruction);
  } else if (index == 13) { // Return
    instr = std::make_shared<ReturnInstruction>(program, instruction);
  } else if (index == 14) { // Call
    instr = std::make_shared<CallInstruction>(program, instruction);
  } else {
    std::cerr << "Unknown Instruction indexed: " << instruction.getContent().index() << std::endl;
  }
  instr->set_prg(program);
  return instr;
}


class LocationSpec : public Spec {
  public:
    LocationSpec(std::string identifier, std::string bb_location) 
      : identifier(identifier), bb_location(bb_location) {}

    constexpr std::string to_string() {
      std::string result = "Location: " + get_full_name() + ";";
      for (auto& next_loc : next) {
        result += "\n  Next: " + next_loc->get_full_name() + ";";
      }
      for (auto& instruction : instructions) {
        if (instruction == nullptr) {
          continue;
        }
        result += "\n    " + instruction->to_string() + ";";
      }
      return result;
    }
    
    void assign_next(SmvSpec& parent_spec, std::string identifier, std::string bb_location);
    
    LocationSpec* add_instruction(MiniMC::Model::Instruction instruction, MiniMC::Model::Program_ptr program) {
      instructions.push_back(make_instruction(instruction, program));
      return this;
    }

    LocationSpec* add_instructions(MiniMC::Model::InstructionStream instructions, MiniMC::Model::Program_ptr program) {
      for (auto& instruction : instructions) {
        add_instruction(instruction, program);
      }
      return this;
    }

    std::string write() {
      return "";
    }

    constexpr std::string get_full_name() const {
      return identifier + "-bb" + bb_location;
    }

    bool operator==(const LocationSpec& other) const {
      return get_full_name() == other.get_full_name();
    }

  private:
    std::string identifier;
    std::string bb_location;
    std::vector<std::shared_ptr<LocationSpec>> next;
    std::vector<std::shared_ptr<InstructionSpec>> instructions;

};


class SmvSpec {
  public:
    std::string to_string() {
      std::string result = "MODULE main\n";
      return result;
    }
    std::shared_ptr<LocationSpec> add_location(std::string identifier, std::string location);
    void add_register(std::string identifier, SmvType type);
    void add_var(std::string identifier, SmvType type);
    std::shared_ptr<LocationSpec> get_location(std::string name); 
    void print();

  private:
    std::vector<std::shared_ptr<LocationSpec>> locations;
    std::vector<std::shared_ptr<RegisterSpec>> registers;
    std::vector<std::shared_ptr<SmvVarSpec>> vars;
};


