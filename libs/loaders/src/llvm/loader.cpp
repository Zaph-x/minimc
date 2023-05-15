
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/LoopRotation.h>
#include <llvm/Transforms/Scalar/LoopUnrollPass.h>
#include <llvm/Transforms/Scalar/SCCP.h>
#include <llvm/Transforms/Scalar/SROA.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/LCSSA.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>

#include <llvm/Analysis/LoopInfo.h>

#include <llvm/PassRegistry.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <functional>
#include <variant>
#include <type_traits>
#include <unordered_set>

#include "llvmpasses.hpp"
#include "loaders/loader.hpp"
#include "model/cfg.hpp"
#include "support/exceptions.hpp"
#include "support/localisation.hpp"
#include "support/feedback.hpp"

#include "model/builder.hpp"
#include "context.hpp"


namespace MiniMC {
  namespace Loaders {

    MiniMC::Model::TypeID getTypeID(llvm::Type* type);
 
    MiniMC::Model::Function_ptr createEntryPoint(std::size_t stacksize, MiniMC::Model::Program& program, MiniMC::Model::Function_ptr function, std::vector<MiniMC::Model::Value_ptr>&&) {
      auto source_loc = std::make_shared<MiniMC::Model::SourceInfo>();
      
      static std::size_t nb = 0;
      const std::string name = MiniMC::Support::Localiser("__minimc__entry_%1%-%2%").format(function->getSymbol(), ++nb);
      MiniMC::Model::CFA cfg;
      auto vstack = std::make_unique<MiniMC::Model::RegisterDescr> (MiniMC::Model::Symbol{name});
      MiniMC::Model::LocationInfoCreator locinf (MiniMC::Model::Symbol{name},vstack.get());
      
      auto funcpointer = program.getConstantFactory().makeFunctionPointer(function->getID());
      funcpointer->setType (program.getTypeFactory ().makePointerType ());
      auto init = cfg.makeLocation(locinf.make("init", 0, *source_loc));
      auto end = cfg.makeLocation(locinf.make("end", 0, *source_loc));
      
      cfg.setInitial(init);
      auto edge = cfg.makeEdge(init, end);

      std::vector<MiniMC::Model::Value_ptr> params;
      MiniMC::Model::Value_ptr result = nullptr;
      MiniMC::Model::Value_ptr sp = program.getConstantFactory().makeHeapPointer(program.getHeapLayout().addBlock(stacksize));
      sp->setType (program.getTypeFactory ().makePointerType ());
      params.push_back(sp);
      auto restype = function->getReturnType();
      if (restype->getTypeID() != MiniMC::Model::TypeID::Void) {
        result = vstack->addRegister(MiniMC::Model::Symbol{"_"}, restype);
      }

      edge->getInstructions () = MiniMC::Model::InstructionStream({MiniMC::Model::createInstruction<MiniMC::Model::InstructionCode::Call>({.res = result,
                                                                                                                                                                               .function = funcpointer,
                                                                                                                                                                               .params = params})});
      
      return program.addFunction(name, {},
                                 program.getTypeFactory().makeVoidType(),
                                 std::move(vstack),
                                 std::move(cfg),
				 false
				 );
    }
    

    
    class LLVMLoader : public Loader {
    public:
      LLVMLoader (MiniMC::Model::TypeFactory_ptr& tfac,
		  Model::ConstantFactory_ptr& cfac,
		  std::size_t stacksize,
		  std::vector<std::string> entry,
		  bool disablePromotion,
		  bool printPass
		  ) : Loader (tfac,cfac),stacksize(stacksize),entry(entry),disablePromotion(disablePromotion),printLLVMPass(printPass) {}
      LoadResult loadFromFile(const std::string& file) override {
        std::fstream str;
        str.open(file);
        std::string ir((std::istreambuf_iterator<char>(str)), (std::istreambuf_iterator<char>()));
        std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(ir));
	return {.program = readFromBuffer(buffer, tfactory, cfactory),
	};
      }

      LoadResult loadFromString(const std::string& inp) override {
        std::stringstream str;
        str.str(inp);
        std::string ir((std::istreambuf_iterator<char>(str)), (std::istreambuf_iterator<char>()));
        std::unique_ptr<llvm::MemoryBuffer> buffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(ir));
        return {.program = readFromBuffer(buffer, tfactory, cfactory),
	};


      }

      auto createFunctionWorkList (llvm::Module& module) {
	std::unordered_set<llvm::Function*> functions;
	for (auto& F : module) {
	  functions.insert (&F);
	  for (auto& B : F) {
	    for (auto& I : B) {
	      if (auto cinst = llvm::dyn_cast<llvm::CallInst>(&I)) {
		functions.insert (cinst->getCalledFunction ());
	      }
	    }
	  }
	}
	return functions;
      }
      
      void loadGlobals (GLoadContext& lcontext, MiniMC::Model::Program_ptr& prgm, llvm::Module& module) {
	std::vector<MiniMC::Model::Instruction> instr;
        
	MiniMC::func_t fid = 0;
	for (auto& F : createFunctionWorkList (module)) {
	  auto& Func = *F;
	  auto ptr = lcontext.getConstantFactory().makeFunctionPointer(fid);
	  ptr->setType(lcontext.getTypeFactory().makePointerType());
	  lcontext.addValue (&Func, ptr);
	  MiniMC::offset_t lid = 0;
	  for (auto& BB : Func) {
	    auto ptr = lcontext.getConstantFactory().makeLocationPointer(fid, lid);
	    ptr->setType(lcontext.getTypeFactory ().makePointerType());
	    lcontext.addValue(&BB, ptr);
	    lid++;
	  }
	  fid++;
	}
      
        for (auto& g : module.getGlobalList()) {
	  auto pointTySize = lcontext.computeSizeInBytes(g.getValueType());
	  
	  auto gvar = lcontext.getConstantFactory().makeHeapPointer(prgm->getHeapLayout().addBlock(pointTySize));
	  lcontext.addValue (&g, gvar);
	  if (g.hasInitializer()) {
	    auto val = lcontext.findValue(g.getInitializer());
	    instr.push_back(MiniMC::Model::createInstruction<MiniMC::Model::InstructionCode::Store>({.addr = gvar,
		  .storee = val}));
	  }
	}
	if (instr.size()) {
	  MiniMC::Model::InstructionStream str(instr);
	  prgm->setInitialiser(str);
	}
      }
      
      void  instantiateFunction (llvm::Function& F,GLoadContext& lcontext, MiniMC::Model::Program_ptr& prgm) {
	
	auto source_loc = std::make_shared<MiniMC::Model::SourceInfo>();
	std::string fname = F.getName().str();
	MiniMC::Model::CFA cfg;
	std::vector<MiniMC::Model::Register_ptr> params;
	auto variablestack = std::make_unique<MiniMC::Model::RegisterDescr> (MiniMC::Model::Symbol{fname});
	MiniMC::Model::LocationInfoCreator locinfoc(MiniMC::Model::Symbol{fname},variablestack.get());
	
	LoadContext load{lcontext,*variablestack,variablestack->addRegister (MiniMC::Model::Symbol{"__minimc.sp"},lcontext.getTypeFactory().makePointerType ())};
	auto returnTy = load.getType(F.getReturnType());
	
	auto makeVariable = [&load,this](auto val) {
	  if (!load.hasValue (val)) {
	    auto type = load.getType (val->getType ());
	    load.addValue (val,load.getStack().addRegister (MiniMC::Model::Symbol{val->getName().str()},type));
	  }
	  return load.findValue (val);
	};
	
	auto makeVar = [&load,this,makeVariable](auto op) {
	  const llvm::Constant* oop = llvm::dyn_cast<const llvm::Constant>(op);
	  auto lltype = op->getType();
	  if (lltype->isLabelTy() ||
	      lltype->isVoidTy())
	      return;
	  auto type = load.getType (op->getType());
	  if (oop) {
	    return;
	  } else {
	    makeVariable(op);
	  }
	};

	params.push_back(std::static_pointer_cast<MiniMC::Model::Register>(load.getStackPointer ()));
	for (auto itt = F.arg_begin(); itt != F.arg_end(); itt++) {
	  auto lltype = itt->getType();
	  auto type = load.getType (lltype);
	  params.push_back (std::static_pointer_cast<MiniMC::Model::Register> (makeVariable(itt)));
	}
	
	if (F.isDeclaration () ) {
	  MiniMC::Support::Messager{}.message<MiniMC::Support::Severity::Warning> (MiniMC::Support::Localiser{"Function '%1%' only declared in LLVM assembly. Treating it as a non-determinstic function"}.format (F.getName ().str()));
										     
	  auto init = cfg.makeLocation (locinfoc.make(std::string{"Init"}, 0,*source_loc));
	  auto end = cfg.makeLocation (locinfoc.make(std::string{"end"}, 0,*source_loc));
	  
	  cfg.setInitial (init);
	  {
	    MiniMC::Model::EdgeBuilder edgebuilder{cfg,init,end};
	    if (returnTy->getTypeID () != MiniMC::Model::TypeID::Void) {
	      std::size_t bitwidth = returnTy ->getSize ();//inst->getType()->getIntegerBitWidth();
	      MiniMC::Model::Value_ptr min, max;
	      
	      switch (bitwidth) {
	      case 1:
		min = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV8>::min(), MiniMC::Model::TypeID::I8);
		max = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV8>::max(), MiniMC::Model::TypeID::I8);
		break;
	      case 2:
		min = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV16>::min(), MiniMC::Model::TypeID::I16);
		max = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV16>::max(), MiniMC::Model::TypeID::I16);
		break;
	      case 4:
		min = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV32>::min(), MiniMC::Model::TypeID::I32);
		max = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV32>::max(), MiniMC::Model::TypeID::I32);
		break;
	      case 8:
		min = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV64>::min(), MiniMC::Model::TypeID::I64);
		max = prgm->getConstantFactory().makeIntegerConstant(std::numeric_limits<MiniMC::BV64>::max(), MiniMC::Model::TypeID::I64);
		break;
	      default:
		throw MiniMC::Support::Exception("Error");
	      }
	      
	      auto retVar = variablestack->addRegister (MiniMC::Model::Symbol{"_ret_"},returnTy);
	      
	      edgebuilder.addInstr<MiniMC::Model::InstructionCode::NonDet> ({.res = retVar, .min = min,.max=max});
	      edgebuilder.addInstr<MiniMC::Model::InstructionCode::Ret> ({retVar});
	      
	    }
	    
	    else {
	      edgebuilder.addInstr<MiniMC::Model::InstructionCode::RetVoid> (0);
	    }
	    
	    }
	  
	  prgm->addFunction(F.getName().str(), params, returnTy, std::move(variablestack), std::move(cfg),F.isVarArg ());
	}
	
	else  {
	  for (const llvm::BasicBlock& bb : F) {
	    for (auto& inst : bb) {
	      makeVar(&inst);
	      auto ops = inst.getNumOperands();
	      for (std::size_t i = 0; i < ops; i++) {
		auto op = inst.getOperand(i);
		makeVar(op);
	      }
	    }
	  }
	  
	  std::unordered_map<llvm::BasicBlock*, MiniMC::Model::Location_ptr> locmap;
	  std::vector<llvm::BasicBlock*> waiting;
	  auto enqueue = [&locinfoc,&cfg,&locmap,&waiting,&source_loc](llvm::BasicBlock* BB)->MiniMC::Model::Location_ptr {
	    if (locmap.count (BB)) {
	      return locmap.at(BB);
	    }
	    else {
	      auto location = cfg.makeLocation (locinfoc.make(BB->getName().str(), 0, *source_loc));
	      locmap.insert (std::make_pair(BB,location));
	      waiting.push_back (BB);
	      return location;
	    }
	  };
	  
	  auto buildphi = [&load](llvm::BasicBlock* from, llvm::BasicBlock* to,auto&& builder) {
	    for (auto& phi : to->phis()) {
	      auto ass = load.findValue(&phi);
	      auto incoming = load.findValue(phi.getIncomingValueForBlock(from));
	      builder.template addInstr<MiniMC::Model::InstructionCode::Assign>({.res = ass, .op1 = incoming});
	    }
	  };
	    
	  auto& BB = F.getEntryBlock();
	  auto entry = enqueue (&BB);
	  cfg.setInitial(entry);
	  InstructionTranslator translate {load};
	  
	  while (waiting.size ()) {
	    auto cur_bb = waiting.back();
	    waiting.pop_back ();
	    auto from = locmap.at (cur_bb);
	    auto to = cfg.makeLocation (from->getInfo ());
	    MiniMC::Model::EdgeBuilder edgebuilder{cfg,from,to};
	    auto term = cur_bb->getTerminator();
	    
	      
	      
	    for (llvm::Instruction& inst : *cur_bb) {
	      
	      if (llvm::isa<llvm::PHINode> (inst) || term == &inst ) {
		continue;
	      }
		
	      translate(&inst,edgebuilder);
	      
	    }
	      
	    if (term) {
	      if (term->getOpcode() == llvm::Instruction::Br) {
		  
		auto brterm = llvm::dyn_cast<llvm::BranchInst>(term);
		if (brterm->isUnconditional()) {
		  auto succ = enqueue(term->getSuccessor(0));
		  MiniMC::Model::EdgeBuilder<true> builder {cfg,to,succ};
		  buildphi (cur_bb,term->getSuccessor (0),builder);
		}
		else {
		  auto cond = load.findValue(brterm->getCondition());
		  {
		    auto ttloc = enqueue (term->getSuccessor (0));
		    auto ttloc_tmp = cfg.makeLocation (to->getInfo ());
		    MiniMC::Model::EdgeBuilder {cfg,to,ttloc_tmp}.addInstr<MiniMC::Model::InstructionCode::Assume> ({.expr = cond});
		    buildphi (cur_bb,term->getSuccessor (0),MiniMC::Model::EdgeBuilder<true> {cfg,ttloc_tmp,ttloc});
		  }
		  
		  
		  {
		    auto ffloc = enqueue (term->getSuccessor (1));
		    auto ffloc_tmp = cfg.makeLocation (to->getInfo ());
		    MiniMC::Model::EdgeBuilder {cfg,to,ffloc_tmp}.addInstr<MiniMC::Model::InstructionCode::NegAssume> ({
			.expr = cond});
		    buildphi (cur_bb,term->getSuccessor (1),MiniMC::Model::EdgeBuilder<true> {cfg,ffloc_tmp,ffloc});
		    
		  }
		}
	      }
	      
	      else if (term->getOpcode() == llvm::Instruction::IndirectBr) {
		auto brterm = llvm::dyn_cast<llvm::IndirectBrInst>(term);
		std::size_t dests = brterm->getNumDestinations();
		auto value = load.findValue(brterm->getAddress());
		for (std::size_t i = 0; i < dests; ++i) {
		  auto splitloc = cfg.makeLocation (to->getInfo ());
		  auto dest = enqueue (brterm->getDestination (i));
		  auto valComp = load.findValue(brterm->getDestination(i));
		  auto btype = load.getTypeFactory().makeBoolType();
		  auto cond = load.getStack().addRegister(MiniMC::Model::Symbol{"-"}, btype);
		  
		  MiniMC::Model::EdgeBuilder {cfg,to,splitloc}.addInstr<MiniMC::Model::InstructionCode::PtrEq>({
		      .res = cond,
		      .op1 = value,
		      .op2 = valComp}).
		    addInstr<MiniMC::Model::InstructionCode::Assume> ({
			.expr = cond}
		      );
		  buildphi (cur_bb,brterm->getDestination (i),MiniMC::Model::EdgeBuilder<true> {cfg,splitloc,dest});
		}
	      }
	      else if (term->getOpcode() == llvm::Instruction::Ret) {
		translate(term,edgebuilder);
		
	      }
	      
	    }
	  }
	  prgm->addFunction(F.getName().str(), params, returnTy, std::move(variablestack), std::move(cfg),F.isVarArg ());
	}
      }
    
      void instantiateFunctions (GLoadContext& lcontext, MiniMC::Model::Program_ptr& prgm, llvm::Module& module) {
	std::unordered_set<llvm::Function*> functions = createFunctionWorkList (module);
	for (auto& F : functions) {
	  instantiateFunction (*F,lcontext,prgm);
	}
	
      }
      
      void llvmModifications (llvm::Module& module) {
		
        llvm::legacy::PassManager lpm;
	lpm.add(llvm::createLowerSwitchPass());
        lpm.run(module);
	llvm::PassBuilder PB;

        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        llvm::LoopPassManager loopmanager;
        llvm::FunctionPassManager funcmanagerllvm;
        llvm::FunctionPassManager funcmanager;
        llvm::ModulePassManager mpm;
        
        PB.registerFunctionAnalyses(fam);
        PB.registerModuleAnalyses(mam);
        PB.registerLoopAnalyses(lam);
        PB.registerCGSCCAnalyses(cgam);
        PB.crossRegisterProxies(lam, fam, cgam, mam);

        funcmanagerllvm.addPass(ConstExprRemover());
        funcmanagerllvm.addPass(RemoveUnusedInstructions());
	if (!disablePromotion)
	  funcmanager.addPass(llvm::PromotePass());
        funcmanagerllvm.addPass(GetElementPtrSimplifier());
        funcmanagerllvm.addPass(InstructionNamer());
        mpm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(funcmanagerllvm)));
	mpm.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(funcmanager)));
	if (printLLVMPass)
	  mpm.addPass(llvm::PrintModulePass(llvm::errs()));
        
        mpm.run(module, mam);
	
      }

      void setupEntryPoints (MiniMC::Model::Program_ptr& prgm) {
	for (const auto& e : entry) {
	  auto func = prgm->getFunction (e);
	  auto entry = createEntryPoint (stacksize,*prgm,func,{});
	  
	  prgm->addEntryPoint (entry->getSymbol().getName ());
	}
      }
      
      virtual MiniMC::Model::Program_ptr readFromBuffer(std::unique_ptr<llvm::MemoryBuffer>& buffer, MiniMC::Model::TypeFactory_ptr& tfac, MiniMC::Model::ConstantFactory_ptr& cfac) {
        auto prgm = std::make_shared<MiniMC::Model::Program>(tfac, cfac);
	GLoadContext lcontext {*cfac,*tfac};
	
	
        llvm::SMDiagnostic diag;
        std::unique_ptr<llvm::LLVMContext> context = std::make_unique<llvm::LLVMContext>();
        std::unique_ptr<llvm::Module> module = parseIR(*buffer, diag, *context);
	if (!module) {
	  throw LoadError{};
	}
        
        llvmModifications (*module);
	loadGlobals (lcontext,prgm,*module);
	instantiateFunctions (lcontext,prgm,*module);
	
	setupEntryPoints (prgm);
	
        return prgm;
      }

    private:
      std::size_t stacksize;
      std::vector<std::string> entry;
      bool disablePromotion;
      bool printLLVMPass;
    };


    class LLVMLoadRegistrar : public LoaderRegistrar {
    public:
      LLVMLoadRegistrar () : LoaderRegistrar("LLVM") {
	addOption<IntOption> ("stack","StackSize",200uL);
	addOption<VecStringOption> ("entry","Entry point function",std::vector<std::string> {});
	addOption<BoolOption> ("disable_promote_pass","Disable the promotion of allocas to registers",false);
	addOption<BoolOption> ("print","Print LLVM module to stderr",false);
      }
      
      Loader_ptr makeLoader (MiniMC::Model::TypeFactory_ptr& tfac, Model::ConstantFactory_ptr cfac) override {
	auto stacksize = getOption<IntOption> (0).value;
	auto entry = getOption<VecStringOption> (1).value;
	auto disablePromotion = getOption<BoolOption> (2).value;
	auto printPass = getOption<BoolOption> (3).value;	
	return std::make_unique<LLVMLoader> (tfac,cfac,stacksize,entry,disablePromotion,printPass);
      }
    };

    static LLVMLoadRegistrar llvmloadregistrar;
    
    
  } // namespace Loaders
} // namespace MiniMC

