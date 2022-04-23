#include "cpa/interface.hpp"
#include "cpa/location.hpp"
#include "hash/hashing.hpp"
#include "model/cfg.hpp"
#include "support/pointer.hpp"

namespace MiniMC {
  namespace CPA {
    namespace Location {

      struct LocationState {
        void push(gsl::not_null<MiniMC::Model::Location*> l) {
          stack.push_back(l.get());
        }

        void pop() {
          if (stack.size() > 1)
            stack.pop_back();
          assert(stack.back());
        }

        auto& cur() {
          assert(stack.size());
          return stack.back();
        }
        auto& cur() const {
          assert(stack.size());
          return stack.back();
        }
        virtual MiniMC::Hash::hash_t hash(MiniMC::Hash::seed_t seed = 0) const {
          MiniMC::Hash::hash_t s = seed;
          for (auto& t : stack)
            MiniMC::Hash::hash_combine(s, *t);
          return s;
        }

        std::vector<MiniMC::Model::Location*> stack;
      };
    } // namespace Location
  }   // namespace CPA
} // namespace MiniMC
namespace std {
  template <>
  struct hash<MiniMC::CPA::Location::LocationState> {
    std::size_t operator()(const MiniMC::CPA::Location::LocationState& s) {
      return s.hash();
    }
  };

} // namespace std

namespace MiniMC {
  namespace CPA {
    namespace Location {
      class State : public MiniMC::CPA::State {
      public:
        State(const std::vector<LocationState>& locations) : locations(locations) {
        }

        State(const State&) = default;

        virtual std::ostream& output(std::ostream& os) const override {
          os << "[ ";
          for (auto l : locations) {
            assert(l.cur());
            os << l.cur()->getInfo() << ", ";
          }
          return os << "]";
        }
        virtual MiniMC::Hash::hash_t hash(MiniMC::Hash::seed_t seed = 0) const override {
          MiniMC::Hash::hash_t s = seed;
          for (auto& t : locations)
            MiniMC::Hash::hash_combine(s, t);
          return s;
        }
        virtual std::shared_ptr<MiniMC::CPA::Location::State> lcopy() const { return std::make_shared<State>(*this); }
        virtual std::shared_ptr<MiniMC::CPA::State> copy() const override { return lcopy(); }

        size_t nbOfProcesses() const override { return locations.size(); }
        MiniMC::Model::Location_ptr getLocation(size_t i) const override { return locations[i].cur()->shared_from_this(); }
        void setLocation(size_t i, MiniMC::Model::Location* l) {
          locations[i].cur() = l;
        }
        void pushLocation(size_t i, MiniMC::Model::Location* l) { locations[i].push(l); }
        void popLocation(size_t i) { locations[i].pop(); }
        bool need2Store() const override {
          for (auto& locState : locations) {
            if (locState.cur()->getInfo().template is<MiniMC::Model::Attributes::NeededStore>())
              return true;
          }
          return false;
        }

        virtual bool assertViolated() const override {
          for (auto& locState : locations) {
            if (locState.cur()->getInfo().template is<MiniMC::Model::Attributes::AssertViolated>())
              return true;
          }
          return false;
        }

        virtual bool hasLocationAttribute(MiniMC::Model::AttrType tt) const override  {
          for (auto& locState : locations) {
            if (locState.cur()->getInfo().isFlagSet(tt))
              return true;
          }
          return false;
        }

      private:
        std::vector<LocationState> locations;
        std::vector<bool> ready;
      };

      MiniMC::CPA::State_ptr MiniMC::CPA::Location::Transferer::doTransfer(const State_ptr& s, const MiniMC::Model::Edge_ptr& edge, proc_id id) {
        auto state = static_cast<const State*>(s.get());
        assert(id < state->nbOfProcesses());
        if (edge->getFrom().get() == state->getLocation(id)) {
          auto nstate = state->lcopy();
          nstate->setLocation(id, edge->getTo().get().get());

          if (edge->hasAttribute<MiniMC::Model::AttributeType::Instructions>()) {
            auto& inst = edge->getAttribute<MiniMC::Model::AttributeType::Instructions>().last();
            if (inst.getOpcode() == MiniMC::Model::InstructionCode::Call) {
	      auto& content = inst.getOps<MiniMC::Model::InstructionCode::Call> ();
	      if (content.function->isConstant()) {
                auto constant = std::static_pointer_cast<MiniMC::Model::Pointer>(content.function);
                pointer_t loadPtr = constant->getValue ();
                auto func = edge->getProgram().getFunction(MiniMC::Support::getFunctionId(loadPtr));
                nstate->pushLocation(id, func->getCFG()->getInitialLocation().get());
              } else
                return nullptr;
            }

            else if (MiniMC::Model::isOneOf<MiniMC::Model::InstructionCode::RetVoid,
                                            MiniMC::Model::InstructionCode::Ret>(inst)) {
              nstate->popLocation(id);
            }
          }
          return nstate;
        }

        return nullptr;
      }

      State_ptr MiniMC::CPA::Location::StateQuery::makeInitialState(const InitialiseDescr& p) {
        std::vector<LocationState> locs;
        for (auto& f : p.getEntries()) {
          locs.emplace_back();
          locs.back().push(f->getCFG()->getInitialLocation().get());
        }
        return std::make_shared<State>(locs);
      }

      
    } // namespace Location
  }   // namespace CPA
} // namespace MiniMC
