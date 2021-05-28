/**
 * @file   state.hpp
 * @date   Mon Apr 20 17:21:09 2020
 * 
 * @brief  
 * 
 * 
 */

#ifndef _STATE__
#define _STATE__

#include <iostream>
#include <memory>
#include "model/variables.hpp"
#include "model/location.hpp"
#include "hash/hashing.hpp"
#include "support/exceptions.hpp"
#include "support/localisation.hpp"

namespace MiniMC {
  namespace CPA {

	using proc_id = std::size_t;
	
	
	class CanntEvaluateException : public MiniMC::Support::VerificationException {
	public:
	  CanntEvaluateException (const MiniMC::Model::Variable_ptr& var) : VerificationException (MiniMC::Support::Localiser ("Cannot Evaluate '%1%' to a value").format(var->getName ())) {}

		
	};

	
	class Concretizer {
	public:
	  enum class Feasibility {
		Feasible,
		Infeasible,
		Unknown
	  };

	  virtual Feasibility isFeasible () const { return Feasibility::Feasible;}
	  virtual MiniMC::Model::Value_ptr evaluate (proc_id, const MiniMC::Model::Variable_ptr& var) {
		throw CanntEvaluateException (var);
	  }

	  virtual std::ostream&  evaluate_str (proc_id, const MiniMC::Model::Variable_ptr& var,std::ostream& os) {
		return os << "??";
	  }
	  
	};

	using Concretizer_ptr = std::shared_ptr<Concretizer>; 
	

    /** A general CPA state interface. It is deliberately kept minimal to relay no information to observers besides what is absolutely needed 
     * 
     */ 
    class State  : public std::enable_shared_from_this<State>{
    public:
      ~State () {}
	  
      virtual std::ostream& output (std::ostream& os) const {return os << "_";}
      virtual MiniMC::Hash::hash_t hash (MiniMC::Hash::seed_t seed = 0) const {return reinterpret_cast<MiniMC::Hash::hash_t> (this);}
      virtual std::shared_ptr<State> copy () const = 0;
	  
	  /** 
       * Get the current Location of process \p id 
       * 
       *
       * @return the Location of \p id or nullptr if there no process
       * \p id
       */
	  virtual MiniMC::Model::Location_ptr getLocation (proc_id id) const  {
		throw MiniMC::Support::Exception ("Should not be called");
	  }

	  virtual size_t nbOfProcesses ( ) const {return 0;}
	  
      /** 
       * Function to tell whether it is deemed necessary to store this State during  explorations to guarantee termination. 
       *
       * @return 
       */
      virtual bool need2Store () const {return false;}
	  virtual bool ready2explore () const {return true;}
	  virtual bool assertViolated () const {	throw MiniMC::Support::Exception ("Should not be called");}

	  
	  
	  virtual const Concretizer_ptr getConcretizer () const {return std::make_shared<Concretizer> ();}
	};
	
    using State_ptr = std::shared_ptr<State>;
    
    std::ostream& operator<< (std::ostream& os, const State& state);
    
  }
}


namespace std {
  template<>
  struct hash<MiniMC::CPA::State> {
    std::size_t operator()(const MiniMC::CPA::State& s) const noexcept {
	  return s.hash ();
    }
  };
  
}

#endif
