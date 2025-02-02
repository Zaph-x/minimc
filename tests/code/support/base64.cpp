#include <iostream>
#include <limits>
#include <cassert>

#include "support/binary_encode.hpp"


int main () {
  MiniMC::Support::Base64Encode encoder;
  MiniMC::uint32_t val = std::numeric_limits<MiniMC::uint32_t>::max();
  std::cerr << val <<  std::endl;;

  std::string enc = encoder.encode (reinterpret_cast<const char*> (&val),4);
  
  MiniMC::Support::DecodeResult result = encoder.decode (enc);
  assert(result.size == 4);
  std::cerr << *reinterpret_cast<MiniMC::uint32_t*> (result.buffer.get()) << std::endl;;
}
