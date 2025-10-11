#ifndef MAGIX_COMPILATION_HPP_
#define MAGIX_COMPILATION_HPP_

#include "magix_vm/types.hpp"

#include <cstddef>
#include <limits>
#include <string_view>

namespace magix::compile
{

constexpr size_t byte_code_size = 65536;
// addressable with immediates ...
// until i do some longjump shenanigans
static_assert(byte_code_size - 1 <= std::numeric_limits<magix::code_word>::max());

using SrcChar = char32_t;
using SrcView = std::basic_string_view<SrcChar>;

} // namespace magix::compile

#endif // MAGIX_COMPILATION_HPP_
