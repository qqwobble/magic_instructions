#ifndef MAGIX_EXECUTION_RUNNER_HPP_
#define MAGIX_EXECUTION_RUNNER_HPP_

#include "godot_cpp/classes/ref.hpp"
#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/allocators.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/execution/executor.hpp"
#include "magix_vm/types.hpp"
#include "magix_vm/utility.hpp"
#include <cstddef>

#include <memory>
#include <vector>

namespace magix
{

constexpr size_t magix_max_mem = 1024 * 256;
constexpr size_t magix_assumed_instance_overhead = 64;

struct ExecLayout
{
    [[nodiscard]] constexpr auto
    roundup_size(size_t t) -> size_t
    {
        constexpr size_t mult = 64;
        auto remainder = t % mult;
        if (remainder)
        {
            t += mult - remainder;
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
    execute(ExecStack *stack) -> PerIDExecResult;

    spellmemvec global_memory;
    std::vector<PerInstanceData> instances;
};

class ExecRunner
{
  public:
    ExecRunner() : reusable_stack(std::make_unique<ExecStack>()) {}

    void
    enqueue_cast_spell(object_id_type id, godot::Ref<MagixByteCode> bytecode, magix::u16 entry);

    void
    run_all();

  private:
    std::unique_ptr<ExecStack> reusable_stack;
    std::unordered_map<std::pair<object_id_type, const ByteCodeRaw *>, PerIDData, magix::pair_hash> active_users;
};

} // namespace magix

#endif // MAGIX_EXECUTION_RUNNER_HPP_
