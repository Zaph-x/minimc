#include <vector>
#include <string>
#include <unordered_set>
#include"BindingAtom.h"
#include"BindingClause.h"
class BindingSet
{
    private:
    std::unordered_set<BindingClause> bindings;

    public:
        static BindingSet emptySet;
        BindingSet();
        ~ BindingSet();

static BindingSet purge(BindingSet& c, const std::string& var) {
    BindingSet res;
    for (auto& clause : c.bindings) {
        BindingClause purged = BindingClause::purge(clause, var);
        res.add(purged);
        if (purged == BindingClause::emptyClause) break;
    }
    return res;
}
bool add(BindingClause* b) {
    if (b == nullptr) {
        return false;
    }

    if (bindings.find(BindingClause::emptyClause) != bindings.end()) {
        return false;
    }
    if (b->equals(BindingClause::emptyClause)) {
        bindings.clear();
        bindings.insert(BindingClause::emptyClause);
        return true;
    }
    return bindings.insert(b).second;
}

bool equals(const void* arg0) const {
    return bindings == ((BindingSet*)arg0)->bindings;
}







};