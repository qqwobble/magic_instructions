#ifndef MAGIX_EXECUTION_CONFIG_HPP_
#define MAGIX_EXECUTION_CONFIG_HPP_

#include <cstddef>
#include <cstdint>

namespace magix::execute
{

constexpr size_t stack_size_default = 65536;
constexpr size_t objbank_size_default = 4096;

using object_id_type = uint64_t;

} // namespace magix::execute

#endif // MAGIX_EXECUTION_CONFIG_HPP_
