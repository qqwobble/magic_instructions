#ifndef MAGIX_EXECUTION_EXECUTOR_HPP_
#define MAGIX_EXECUTION_EXECUTOR_HPP_

#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/types.hpp"
namespace magix
{

class ExecutionContext;

constexpr size_t stack_size_default = 65536;

struct ExecStack
{
    alignas(64) std::byte stack[stack_size_default];
};

struct PageInfo
{
    ExecStack *stack;
    size_t stack_size;
    // more late
};

struct ExecResult
{

    enum class Type
    {
        OK_EXIT,
        TRAP_INST,
        TRAP_MISALIGNED_IP,
        TRAP_MEM_ACCESS_IP,
        TRAP_MEM_ACCESS_SP,
        TRAP_MEM_UNALIGNED_SP,
        TRAP_TOO_MANY_STEPS,
        TRAP_INVALID_INSTRUCTION,
    };

    Type type;
};

[[nodiscard]] ExecResult
execute(const ByteCodeRaw &code, magix::u16 entry, const PageInfo &pages, size_t steps, ExecutionContext &context);

} // namespace magix

#endif // MAGIX_EXECUTION_EXECUTOR_HPP_
