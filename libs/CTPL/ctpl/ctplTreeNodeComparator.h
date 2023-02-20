#ifndef C__TPL_CTPLTREENODECOMPARATOR_H
#define C__TPL_CTPLTREENODECOMPARATOR_H
#include "ctplTreeNode.h"

class CtplTreeNodeComparator {
    public:
        int compare(const void *arg0, const void *arg1) {
            int size0 = ((CtplTreeNode*)arg0)->getSize();
            int size1 = ((CtplTreeNode*)arg1)->getSize();
            if (size0 < size1) return -1;
            if (size0 == size1) return 0;
            return 1;
        }

};

#endif //C__TPL_CTPLTREENODECOMPARATOR_H
