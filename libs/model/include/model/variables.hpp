#ifndef _VARIABLE__
#define _VARIABLE__

#include <string>
#include <limits>
#include <memory>
#include <vector>
#include <ostream>
#include "model/types.hpp"
namespace MiniMC {
  namespace Model {

    class Value {
    public:
      ~Value () {}
      const Type_ptr& getType  () const  {return type;}
      void setType (const Type_ptr& t) {type = t;}
      virtual bool isVariable () const {return false;}
      virtual bool isConstant () const {return false;}
      virtual std::ostream& output (std::ostream& os) const = 0;
    private:
      Type_ptr type;
    };

    inline std::ostream& operator<< (std::ostream& os, const Value& v) {
      return v.output (os);
    }
    
    using Value_ptr = std::shared_ptr<Value>;

    class IntegerConstant {
      IntegerConstant (uint64_t val) : value(val) {}
      auto getValue () const {return value;}
      bool isConstant () const {return true;}
      virtual std::ostream& output (std::ostream& os) const {return os << value;}
    private:
      uint64_t value;
    };
    template<class T>
    class Placed  {
    public:
      Placed () : place(unused) {}
      std::size_t getPlace () const {return place;}
      void setPlace (std::size_t p) {place = p;}

    private:
      static constexpr std::size_t unused = std::numeric_limits<std::size_t>::max ();
      std::size_t place;
    };

    
    
    class Variable : public Value,
		     public Placed<Variable> {
    public:
      friend class  VariableStackDescr;
      Variable (const std::string& name) : name(name) {}
      const std::string& getName () const {return name;}
      virtual std::ostream& output (std::ostream& os) const  {return os << getName();}
      bool isVariable () const {return true;}
    protected:
      void setOwnerId (std::size_t s) {stackOwnerId = s;}
    private:
      std::string name;
      Type_ptr type;
      std::size_t stackOwnerId = std::numeric_limits<std::size_t>::max ();
    };

    using Variable_ptr = std::shared_ptr<Variable>;
    
    class VariableStackDescr {
      VariableStackDescr (std::size_t id) : id(id) {}
      Variable_ptr addVariable (const std::string& name, const Type_ptr& type); 
      
    private:
      std::vector<Variable_ptr> variables;
      std::size_t id;
      std::size_t totalSize = 0;
    
    };

	using VariableStackDescr_ptr = std::shared_ptr<VariableStackDescr>;
    
  }
}

#endif
