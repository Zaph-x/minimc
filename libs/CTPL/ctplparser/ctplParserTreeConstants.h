#ifndef C__TPL_CTPLPARSERTREECONSTANTS_H
#define C__TPL_CTPLPARSERTREECONSTANTS_H

#include <string>
#include <vector>

class CtplParserTreeConstants {
public:
    const int JJTONELINE = 0;
    const int JJTVOID = 1;
    static const int JJTOR = 2;
    static const int JJTAND = 3;
    static const int JJTAU = 4;
    static const int JJTEU = 5;
    static const int JJTAX = 6;
    static const int JJTAF = 7;
    static const int JJTAG = 8;
    static const int JJTEX = 9;
    static const int JJTEF = 10;
    static const int JJTEG = 11;
    const int JJTEXISTS = 12;
    static const int JJTNOT = 13;
    const int JJTPREDICATE = 14;
    const int JJTPARAMETER = 15;
    const int JJTVARIABLE = 16;

    std::vector<std::string> jjtNodeName = {
            "oneline",
            "void",
            "or",
            "and",
            "AU",
            "EU",
            "AX",
            "AF",
            "AG",
            "EX",
            "EF",
            "EG",
            "exists",
            "not",
            "predicate",
            "parameter",
            "variable"
    };
};


#endif //C__TPL_CTPLPARSERTREECONSTANTS_H
