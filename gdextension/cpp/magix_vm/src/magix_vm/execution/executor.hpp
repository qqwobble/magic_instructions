#ifndef MAGIX_EXECUTION_EXECUTOR_HPP_
#define MAGIX_EXECUTION_EXECUTOR_HPP_

#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/execution/config.hpp"
#include "magix_vm/macros.hpp"
#include "magix_vm/span.hpp"
#include "magix_vm/types.hpp"

#include <cstddef>
#include <cstring>
#include <type_traits>

namespace magix
{
class MagixCaster;
}

namespace magix::execute
{

enum class ObjectTag : object_id_type
{
    NONE = 0,
    GODOT_ID,
};

struct PrimitiveUnion
{
    enum class Tag
    {
        U8,
        U16,
        U32,
        U64,
        I8,
        I16,
        I32,
        I64,
        F32,
        F64,
    };

    Tag tag;
    union
    {
        magix::u8 val_u8;
        magix::u16 val_u16;
        magix::u32 val_u32;
        magix::u64 val_u64;
        magix::i8 val_i8;
        magix::i16 val_i16;
        magix::i32 val_i32;
        magix::i64 val_i64;
        magix::f32 val_f32;
        magix::f64 val_f64;
    };

    PrimitiveUnion(magix::u8 value) : tag{Tag::U8}, val_u8{value} {}
    PrimitiveUnion(magix::u16 value) : tag{Tag::U16}, val_u16{value} {}
    PrimitiveUnion(magix::u32 value) : tag{Tag::U32}, val_u32{value} {}
    PrimitiveUnion(magix::u64 value) : tag{Tag::U64}, val_u64{value} {}
    PrimitiveUnion(magix::i8 value) : tag{Tag::I8}, val_i8{value} {}
    PrimitiveUnion(magix::i16 value) : tag{Tag::I16}, val_i16{value} {}
    PrimitiveUnion(magix::i32 value) : tag{Tag::I32}, val_i32{value} {}
    PrimitiveUnion(magix::i64 value) : tag{Tag::I64}, val_i64{value} {}
    PrimitiveUnion(magix::f32 value) : tag{Tag::F32}, val_f32{value} {}
    PrimitiveUnion(magix::f64 value) : tag{Tag::F64}, val_f64{value} {}

    [[nodiscard]] constexpr auto
    operator==(const PrimitiveUnion &rhs) const -> bool
    {
        if (tag != rhs.tag)
        {
            return false;
        }
        switch (tag)
        {
        case Tag::U8:
        {
            return val_i8 == rhs.val_i8;
        }
        case Tag::U16:
        {
            return val_i16 == rhs.val_i16;
        }
        case Tag::U32:
        {
            return val_i32 == rhs.val_i32;
        }
        case Tag::U64:
        {
            return val_u64 == rhs.val_u64;
        }
        case Tag::I8:
        {
            return val_i8 == rhs.val_i8;
        }
        case Tag::I16:
        {
            return val_i16 == rhs.val_i16;
        }
        case Tag::I32:
        {
            return val_i32 == rhs.val_i32;
        }
        case Tag::I64:
        {
            return val_i64 == rhs.val_i64;
        }
        case Tag::F32:
        {
            return val_f32 == rhs.val_f32;
        }
        case Tag::F64:
        {
            return val_f64 == rhs.val_f64;
        }
        }
        MAGIX_UNREACHABLE("exhausted enum");
    }

    [[nodiscard]] constexpr auto
    operator!=(const PrimitiveUnion &rhs) const -> bool
    {
        return !(*this == rhs);
    }
};

struct ExecutionContext
{
    object_id_type caster_id = 0;
    MagixCaster *caster_node = nullptr;
#ifdef MAGIX_BUILD_TESTS
    std::vector<PrimitiveUnion> test_output;
#endif

    MAGIX_CONSTEXPR_NO_TEST("vector") ExecutionContext() = default;
    MAGIX_CONSTEXPR_NO_TEST("vector") ExecutionContext(object_id_type caster_id, MagixCaster *caster_node) : caster_id{caster_id}, caster_node{caster_node} {}
};

struct ObjectVariant
{
    ObjectTag tag;
    object_id_type id;
};
static_assert(std::is_trivially_constructible_v<ObjectVariant>);
static_assert(std::is_trivially_destructible_v<ObjectVariant>);

struct ExecStack
{
    alignas(64) std::byte stack[stack_size_default];
    alignas(64) ObjectVariant objbank[objbank_size_default];
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
    size_t object_count;
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
execute(const compile::ByteCodeRaw &code, magix::u16 entry, const PageInfo &pages, size_t steps, ExecutionContext &context) -> ExecResult;

} // namespace magix::execute

#ifdef MAGIX_BUILD_TESTS
#include <doctest.h>

namespace doctest
{
template <>
struct StringMaker<magix::execute::PrimitiveUnion>
{
  public:
    static auto
    convert(const magix::execute::PrimitiveUnion &in) -> String
    {
        switch (in.tag)
        {
        case magix::execute::PrimitiveUnion::Tag::U8:
        {
            return "U8:" + doctest::toString(in.val_u8);
        }
        case magix::execute::PrimitiveUnion::Tag::U16:
        {
            return "U16:" + doctest::toString(in.val_u16);
        }
        case magix::execute::PrimitiveUnion::Tag::U32:
        {
            return "U32:" + doctest::toString(in.val_u32);
        }
        case magix::execute::PrimitiveUnion::Tag::U64:
        {
            return "U64:" + doctest::toString(in.val_u64);
        }
        case magix::execute::PrimitiveUnion::Tag::I8:
        {
            return "I8:" + doctest::toString(in.val_i8);
        }
        case magix::execute::PrimitiveUnion::Tag::I16:
        {
            return "U16:" + doctest::toString(in.val_i16);
        }
        case magix::execute::PrimitiveUnion::Tag::I32:
        {
            return "I32:" + doctest::toString(in.val_i32);
        }
        case magix::execute::PrimitiveUnion::Tag::I64:
        {
            return "I64:" + doctest::toString(in.val_u64);
        }
        case magix::execute::PrimitiveUnion::Tag::F32:
        {
            return "F32:" + doctest::toString(in.val_f32);
        }
        case magix::execute::PrimitiveUnion::Tag::F64:
        {
            return "F64:" + doctest::toString(in.val_f64);
        }
        }
        MAGIX_UNREACHABLE("exhausted enum");
    }
};
} // namespace doctest
#endif

#endif // MAGIX_EXECUTION_EXECUTOR_HPP_
