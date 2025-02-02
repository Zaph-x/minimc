/**
 * @file   rremoveretsentry.hpp
 * @date   Mon Apr 20 17:09:12 2020
 * 
 * @brief  
 * 
 * 
 */
#ifndef _removeretsentry__
#define _removeretsentry__

#include "model/cfg.hpp"
#include "support/sequencer.hpp"

namespace MiniMC {
  namespace Model {
    namespace Modifications {
      /** Replaces return instructions by Skip instructions in entry
	  points. By doing so  we avoid managing special cases (in
	  exploration algorithms) when an entry point terminates. 
      */
      struct RemoveRetEntryPoints : public MiniMC::Support::Sink<MiniMC::Model::Program> {
        virtual bool run(MiniMC::Model::Program& prgm) {
          for (auto& F : prgm.getEntryPoints()) {
            for (auto& E : F->getCFG()->getEdges()) {
              if (E->hasAttribute<MiniMC::Model::AttributeType::Instructions>()) {
                for (auto& I : E->getAttribute<MiniMC::Model::AttributeType::Instructions>()) {
                  if (I.getOpcode() == MiniMC::Model::InstructionCode::Ret ||
                      I.getOpcode() == MiniMC::Model::InstructionCode::RetVoid) {
                    MiniMC::Model::InstBuilder<MiniMC::Model::InstructionCode::Skip> skip;
                    I.replace(skip.BuildInstruction());
                  }
                }
              }
            }
          }
          return true;
        }
      };

    } // namespace Modifications
  }   // namespace Model
} // namespace MiniMC

#endif
