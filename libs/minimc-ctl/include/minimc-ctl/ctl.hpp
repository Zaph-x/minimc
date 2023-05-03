//
// Created by zaph on 21/03/23.
//

#ifndef MINIMC_CTL_HPP
#define MINIMC_CTL_HPP

#include <vector>
#include <memory>

enum class ctl_type {
  Integer,
  String,
  Bool,
  Undefined,
};

class ctl_atom {
private:
  ctl_type type = ctl_type::Undefined;
};

class ctl_tree_node {
public:
  ctl_tree_node(ctl_atom atom) {
    value = atom;
  }

  void add_child(ctl_tree_node node) {
    children.push_back(std::move(node));
  }

  ctl_tree_node get_child(int index) {
    if (index > num_of_children) throw std::exception();
    return children.at(index);
  }

  std::shared_ptr<ctl_tree_node> get_parent() {
    return parent;
  }

  int get_size() {
    return num_of_children;
  }

  bool is_leaf() {
    return get_size() == 0;
  }

private:
  std::vector<ctl_tree_node> children {};
  std::shared_ptr<ctl_tree_node> parent;
  ctl_atom value;
  int num_of_children = children.size();
};


#endif // MINIMC_CTL_HPP
