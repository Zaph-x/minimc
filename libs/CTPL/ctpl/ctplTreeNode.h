//
// Created by cbaae on 2/17/2023.
//

#ifndef C__TPL_CTPLTREENODE_H
#define C__TPL_CTPLTREENODE_H
#include <vector>
#include <string>
#include <stdexcept>
#include "ctplAtom.h"

class CtplTreeNode {
public:
    CtplTreeNode();
    ~CtplTreeNode();
     int getSize() {
        return 0;
    }
     CtplTreeNode(CtplAtom value) {
         std::vector<CtplTreeNode> children;
         children.resize(MAX_CHILDREN);
         this->value = value;
         this->parent = nullptr;
         this->numChildren = 0;
     }
    void addChild(CtplTreeNode* child) {
        if (numChildren >= MAX_CHILDREN) {
            throw std::invalid_argument("Node does not support more than " + std::to_string(MAX_CHILDREN) + " children");
        }
        children[numChildren++] = child;
        child->parent = this;
    }


protected:
     std::vector<CtplTreeNode*> children;
     CtplTreeNode* parent;
     CtplAtom value;
     int numChildren;

private:
    const int MAX_CHILDREN = 2;

};


#endif //C__TPL_CTPLTREENODE_H
