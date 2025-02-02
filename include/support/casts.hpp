#ifndef _CASTS__
#define _CASTS__

#include "support/exceptions.hpp"
#include "support/types.hpp"

namespace MiniMC {
  namespace Support {
    template <class T, class P>
    P zext(const T t);

    template <class T, class P>
    P sext(const T t);

    template <class T, class P>
    P trunc(const T& t) {
      static_assert(sizeof(P) <= sizeof(T));
      using uns_from = typename EquivUnsigned<T>::type;
      using uns_to = typename EquivUnsigned<P>::type;

      uns_from casted = bit_cast<T, uns_from>(t);
      uns_to res = static_cast<uns_to>(casted);

      return bit_cast<uns_to, P>(res);
    }

  } // namespace Support
} // namespace MiniMC

#endif
