#include <vector>
#include <string>
#include <unordered_set>
#include"BindingAtom.h"

class BindingClause
{
    private:

    std::unordered_set<BindingAtom> atoms;
    public:
    static BindingClause emptyClause;
    static BindingClause res;


    static BindingClause purge(BindingClause b, std::string var) {
    for (auto iter = b.atoms.begin(); iter != b.atoms.end(); ++iter) {
        BindingAtom e = *iter;
        if (e.getVariable() != var)
            res.add(e);
    }
    return res;
    }
    bool add(const BindingAtom& e) {
        if (e.getType() == BindingAtom::EQUAL) {
            return bind(e.getVariable(), e.getConstant());
        } else {
            return forbid(e.getVariable(), e.getConstant());
        }
    }
    bool bind(std::string variable, std::string constant) {
        std::vector<BindingAtom>::iterator it = atoms.begin();
        std::vector<BindingAtom> removalList;
        while (it != atoms.end()) {
            BindingAtom e = *it;
            if (e.getVariable() == variable) {
                if (e.getType() == BindingAtom::EQUAL) {
                    if (e.getConstant() != constant) return false;
                    else return true;
                } else /* NOT_EQUAL */ {
                    if (e.getConstant() == constant) return false;
                    else removalList.push_back(e); // v != c2 is replaced by v = c1
                }
            }
            it++;
        }

        // Remove the replaced elements.
        if (removalList.size() != 0) log.info("Purging " + removalList);
        it = removalList.begin();
        while (it != removalList.end()) {
            atoms.remove(*it);
            it++;
        }

        atoms.push_back(BindingAtom(variable, constant, BindingAtom::EQUAL));
        return true;
    }
    bool BindingClause::forbid(std::string variable, std::string constant) {
        for (auto it = atoms.begin(); it != atoms.end(); ++it) {
            BindingAtom& e = *it;
            if (e.getVariable() == variable) {
                if (e.getType() == BindingAtom::EQUAL) {
                    if (e.getConstant() == constant) return false;
                    else return true; // this NOT_EQUAL binding is superfluous
                }
            }
        }
        atoms.emplace_back(variable, constant, BindingAtom::NOT_EQUAL);
        return true;
    }
    static BindingClause and(BindingClause b1, BindingClause b2) {
        BindingClause res = b1.clone();

        for (auto iter = b2.atoms.begin(); iter != b2.atoms.end(); iter++) {
            BindingAtom e = *iter;
            if (!res.add(e.clone())) return nullptr;
        }
        return res;
    }
    BindingSet negate(BindingClause b) {
    if (b == BindingClause::emptyClause) return nullptr;
    BindingSet res = BindingSet();
    for (auto iter = b.atoms.begin(); iter != b.atoms.end(); iter++) {
        BindingAtom e = *iter;
        BindingClause t = BindingClause();
        t.add(BindingAtom::negate(e));
        res.add(t);
    }
    return res;
}


};
