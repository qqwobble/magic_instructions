#ifndef MAGIX_COMPILATION_COMPILED_HPP_
#define MAGIX_COMPILATION_COMPILED_HPP_

#include "godot_cpp/templates/rb_map.hpp"
#include "magix_vm/types.hpp"
#include <cstddef>
#include <limits>

namespace magix
{

constexpr size_t byte_code_size = 65536;
// addressable with immediates ...
// until i do some longjump shenanigans
static_assert(byte_code_size - 1 <= std::numeric_limits<magix::code_word>::max());

struct ByteCodeRaw
{
    alignas(64) std::byte code[byte_code_size] = {};
    godot::RBMap<godot::String, magix::u16> entry_points;
};

} // namespace magix

#endif // MAGIX_COMPILATION_COMPILED_HPP_
