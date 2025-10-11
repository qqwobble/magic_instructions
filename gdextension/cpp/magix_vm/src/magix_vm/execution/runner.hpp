#ifndef MAGIX_EXECUTION_RUNNER_HPP_
#define MAGIX_EXECUTION_RUNNER_HPP_

#include "godot_cpp/classes/ref.hpp"

#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/allocators.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/execution/config.hpp"
#include "magix_vm/execution/executor.hpp"
#include "magix_vm/types.hpp"
#include "magix_vm/utility.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace magix::execute
{

/** Specify memory sizes and segments per storage duration type.
 * [PRIMITIVE][OBJECT]
 */
struct ExecLayout
{
    [[nodiscard]] constexpr auto
    roundup_size(size_t t) -> size_t
    {
        auto remainder = t % memory_granularity;
        if (remainder)
        {
            t += memory_granularity - remainder;
        }
        return t;
    }

    size_t primitive_end;
    size_t obj_end;

    ExecLayout() = default;
    ExecLayout(size_t bytes_primitive, size_t obj_count)
    {
        primitive_end = roundup_size(bytes_primitive);
        obj_end = primitive_end + roundup_size(obj_count * sizeof(ObjectVariant));
    }

    [[nodiscard]] auto
    total_size() const -> size_t
    {
        return obj_end;
    }
};

using spellmemvec = std::vector<std::byte, AlignedAllocator<std::byte, 64>>;

struct PerInstanceData
{
    PerInstanceData(ExecLayout layout, magix::u16 entry) : entry(entry), memory(layout.total_size(), std::byte{}) {}

    magix::u16 entry;
    magix::f32 bound_mana = 0.0;
    spellmemvec memory;
};

struct PerIDExecResult
{
    bool should_delete;
};

struct PerIDData
{
    object_id_type object_id;
    ExecLayout global_layout;
    ExecLayout local_layout;

    godot::Ref<MagixByteCode> _bytecode;

    PerIDData(object_id_type id, godot::Ref<MagixByteCode> bytecode);

    [[nodiscard]] auto
    max_invoc_count() const -> size_t;

    [[nodiscard]] auto
    free_invocation_count() const -> size_t;

    auto
    enqueue(magix::u16 entry) -> void;

    auto
    execute(ExecStack *stack, ExecutionContext &context) -> PerIDExecResult;

    spellmemvec global_memory;
    std::vector<PerInstanceData> instances;
};

class ExecRunner
{
  public:
    ExecRunner() : reusable_stack(std::make_unique<ExecStack>()) {}

    void
    enqueue_cast_spell(magix::MagixCaster *caster, godot::Ref<MagixByteCode> bytecode, magix::u16 entry);

    struct RunResult
    {
#ifdef MAGIX_BUILD_TESTS
        std::vector<std::vector<PrimitiveUnion>> test_records;
#endif
    };

    [[nodiscard]] auto
    run_all() -> RunResult;

    void
    clear();

  private:
    std::unique_ptr<ExecStack> reusable_stack;
    std::unordered_map<std::pair<object_id_type, const compile::ByteCodeRaw *>, PerIDData, magix::pair_hash> active_users;
};

} // namespace magix::execute

#endif // MAGIX_EXECUTION_RUNNER_HPP_
