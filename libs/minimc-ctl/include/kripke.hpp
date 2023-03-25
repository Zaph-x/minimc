/*
 * This file defines a kripke structure class, and its components.
 * */

#ifndef KRIPKE_HPP_
#define KRIPKE_HPP_

#include <vector>
#include "model/cfg.hpp"

class k_relation {
  public:
    k_relation(const MiniMC::Model::Location_ptr &s1, const MiniMC::Model::Location_ptr &s2, const MiniMC::Model::Edge_ptr &e) {
      state1 = s1;
      state2 = s2;
      edge = e;
    }
    ~k_relation() = default;

    MiniMC::Model::Location_ptr get_state1() const { return state1; }
    MiniMC::Model::Location_ptr get_state2() const { return state2; }
    MiniMC::Model::Edge_ptr get_edge() const { return edge; }

  private:
    MiniMC::Model::Location_ptr state1;
    MiniMC::Model::Location_ptr state2;
    MiniMC::Model::Edge_ptr edge;
};

class k_proposition {
  public:
    k_proposition();
};

class k_label {
  public:
    k_label(const MiniMC::Model::Location_ptr &s, const std::vector<k_proposition> &p) {
      state = s;
      propositions = p;
    }

  private:
    MiniMC::Model::Location_ptr state;
    std::vector<k_proposition> propositions;
};

class kripke {
  public:
    kripke(const MiniMC::Model::CFA &c);
    ~kripke();




  private:
    MiniMC::Model::CFA cfa;
    std::vector<MiniMC::Model::Edge_ptr> edges;


    std::vector<MiniMC::Model::Location_ptr> states;
    std::vector<MiniMC::Model::Location_ptr> initial_states;
    std::vector<k_relation> relations;
    std::vector<k_label> labels;

};

#endif
