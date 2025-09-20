#ifndef MAGIX_EXECUTION_EXECUTOR_HPP_
#define MAGIX_EXECUTION_EXECUTOR_HPP_

#include "godot_cpp/classes/object.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>
namespace magix
{

class ExecutionContext;

using object_id_type = uint64_t;
enum class ObjectTag : object_id_type
{
    NONE = 0,
    GODOT_ID,
};

struct ObjectVariant
{
    ObjectTag tag;
    object_id_type id;
};

constexpr size_t stack_size_default = 65536;
constexpr size_t objbank_size = 4096;

struct ExecStack
{
    alignas(64) std::byte stack[stack_size_default];
    alignas(64) ObjectVariant objbank[objbank_size];
    void
    clear()
    {
        std::memset(stack, 0, sizeof(stack));
        std::memset(objbank, 0, sizeof(objbank));
        static_assert(std::is_trivially_copyable_v<ObjectVariant>);
    }
};

struct PageInfo
{
    ExecStack *stack;
    size_t stack_size;
    magix::span<std::byte> primitive_shared;
    magix::span<std::byte> primitive_fork;
    magix::span<ObjectVariant> object_fork;
    magix::span<ObjectVariant> object_shared;
    // more later
};

struct ExecResult
{

    enum class Type
    {
        OK_EXIT,
        OK_YIELD,
        TRAP_INST,
        TRAP_MISALIGNED_IP,
        TRAP_MEM_ACCESS_IP,
        TRAP_MEM_ACCESS_SP,
        TRAP_MEM_UNALIGNED_SP,
        TRAP_MEM_ACCESS_USER,
        TRAP_MEM_UNALIGN_USER,
        TRAP_TOO_MANY_STEPS,
        TRAP_INVALID_INSTRUCTION,
    };

    magix::u16 instruction_pointer;
    Type type;
};

[[nodiscard]] auto
execute(const ByteCodeRaw &code, magix::u16 entry, const PageInfo &pages, size_t steps, ExecutionContext *context) -> ExecResult;

} // namespace magix

#endif // MAGIX_EXECUTION_EXECUTOR_HPP_
