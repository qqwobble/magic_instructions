#ifndef GLOBAL_DECLARATIONS_HPP_
#define GLOBAL_DECLARATIONS_HPP_

#include <array>
#include <cstddef>
#include <cstdint>

namespace magix
{

constexpr size_t OPCODE_SIZE = 2;
constexpr size_t BYTE_CODE_SIZE = 1 << 16;
constexpr size_t BYTE_CODE_ALIGN = 16;

struct RawByteCode
{
    alignas(BYTE_CODE_ALIGN) std::array<std::byte, BYTE_CODE_SIZE> raw;
};

using byte_code_addr = uint16_t;

} // namespace magix

#endif // GLOBAL_DECLARATIONS_HPP_
