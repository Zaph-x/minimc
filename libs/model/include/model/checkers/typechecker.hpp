#ifndef _TYPECHECK__
#define _TYPECHECK__

#include "model/cfg.hpp"
#include "support/sequencer.hpp"
#include "support/feedback.hpp"

namespace MiniMC {
  namespace Model {
    namespace Checkers {
      struct TypeChecker : public MiniMC::Support::Sink<MiniMC::Model::Program> {
	TypeChecker (MiniMC::Support::Messager& ptr) : messager (ptr) {}
	virtual bool run (MiniMC::Model::Program&  prgm);
      private:
	MiniMC::Support::Messager& messager;
      };

    }
  }
}


#endif
