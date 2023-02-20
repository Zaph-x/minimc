#ifndef C__TPL_CTPLOPERATOR_H
#define C__TPL_CTPLOPERATOR_H
#include "ctplAtom.h"
#include "../parser/ctplParserTreeConstants.h"

class CtplOperator : public CtplAtom {
    const int AF = CtplParserTreeConstants::JJTAF;
    const int AX = CtplParserTreeConstants::JJTAX;
    const int AG = CtplParserTreeConstants::JJTAG;
    const int AU = CtplParserTreeConstants::JJTAU;
    const int EF = CtplParserTreeConstants::JJTEF;
    const int EX = CtplParserTreeConstants::JJTEX;
    const int EG = CtplParserTreeConstants::JJTEG;
    const int EU = CtplParserTreeConstants::JJTEU;
    const int AND = CtplParserTreeConstants::JJTAND;
    const int OR = CtplParserTreeConstants::JJTOR;
    const int NOT = CtplParserTreeConstants::JJTNOT;

public:
    CtplOperator(int type) {
        this->type = type;
    }
    ~CtplOperator() {}

     bool equals(int arg0) {
        return this->type == arg0;
    }

    std::string toString() {
        if (type == AF) {
            return "AF";
        } else if (type == AX) {
            return "AX";
        } else if (type == AG) {
            return "AG";
        } else if (type == AU) {
            return "AU";
        } else if (type == EF) {
            return "EF";
        } else if (type == EX) {
            return "EX";
        } else if (type == EG) {
            return "EG";
        } else if (type == EU) {
            return "EU";
        } else if (type == AND) {
            return "&";
        } else if (type == OR) {
            return "|";
        } else if (type == NOT) {
            return "-";
        } else {
            return "Invalid type";
        }
    }
};


#endif //C__TPL_CTPLOPERATOR_H
