#ifndef C__TPL_CTPLATOM_H
#define C__TPL_CTPLATOM_H

class CtplAtom {

protected:
    int type = -1;
public:
    CtplAtom() {}
    ~CtplAtom() {}
    bool equals(int id) {
        return false;
    }
};


#endif //C__TPL_CTPLATOM_H
