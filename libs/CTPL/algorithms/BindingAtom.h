#include <vector>
#include <string>
#include <unordered_set>
class BindingAtom{
    private:
    std::string variable;
    std::string constant;
    int type;

    public:
    const static int EQUAL = 0;
    const static int NOT_EQUAL = 1;

    BindingAtom(std::string variable, std::string constant, int type){
        this->variable = variable;
        this->constant = constant;
        this->type = type;
    }
    static BindingAtom negate(BindingAtom atom){
        if(atom.getType() == EQUAL){
            return BindingAtom(atom.getVariable(), atom.getConstant(), NOT_EQUAL);
        }
        else{
            return BindingAtom(atom.getVariable(), atom.getConstant(), EQUAL);
        }
    }

    bool BindingAtom::equals(void* arg0) {
        if (BindingAtom* b = dynamic_cast<BindingAtom*>((BindingAtom*)arg0)) {
            return ((b->constant == this->constant)
                    && (b->variable == this->variable)
                    && (b->type == this->type));
        } else {
            return false;
        }
    }

    std::string getConstant(){
        return this->constant;
    }

    std::string getVariable(){
        return this->variable;
    }

    int getType(){
        return this->type;
    }

    protected:
    BindingAtom* clone() {
    return new BindingAtom(this->variable,this->constant,this->type);
}
std::string toString() {
    std::string op = (type == EQUAL) ? "=" : "!=";
    return variable + op + constant;
}


};
