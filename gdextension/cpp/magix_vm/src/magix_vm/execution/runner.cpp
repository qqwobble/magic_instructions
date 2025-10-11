#include "magix_vm/execution/runner.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/core/object.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/compilation/compiled.hpp"
#include "magix_vm/execution/executor.hpp"

namespace
{

/** Split the given, raw memory into typed spans. */
[[nodiscard]] auto
get_spans(magix::execute::spellmemvec &vec, magix::execute::ExecLayout layout)
    -> std::pair<magix::span<std::byte>, magix::span<magix::execute::ObjectVariant>>
{
    magix::span<std::byte> data(vec);
    auto prim = data.subspan(0, layout.primitive_end);
    auto obj = data.subspan(layout.primitive_end, layout.obj_end).reinterpret_resize<magix::execute::ObjectVariant>();
    return {prim, obj};
}
} // namespace

auto
magix::execute::ExecRunner::run_all() -> RunResult
{

    RunResult run_result;

    const auto it_beg = active_users.begin();
    const auto it_end = active_users.end();
    for (auto it = it_beg; it != it_end;)
    {
        // need to keep next_it around, because we might delete and invalidate it
        auto next_it = it;
        ++next_it;

        auto [id, bc] = it->first;

        // if owner somehow died, all spells die
        MagixCaster *caster = godot::Object::cast_to<MagixCaster>(godot::ObjectDB::get_instance(id));
        if (caster == nullptr)
        {
            active_users.erase(it);
            it = next_it;
            continue;
        }

        ExecutionContext context{
            id,
            caster,
        };

        PerIDData &per_id = it->second;

        // no crosstalk between users!
        reusable_stack->clear();

        auto result = per_id.execute(reusable_stack.get(), context);
        if (result.should_delete)
        {
            active_users.erase(it);
        }

        #ifdef MAGIX_BUILD_TESTS
        run_result.test_records.emplace_back(std::move(context.test_output));
        #endif

        // next_it is valid, even if it invalidated
        it = next_it;
    }

    return run_result;
}

void
magix::execute::ExecRunner::enqueue_cast_spell(magix::MagixCaster *caster, godot::Ref<MagixByteCode> bytecode, magix::u16 entry)
{
    const compile::ByteCodeRaw *bc = &bytecode->get_code();
    const auto id = caster ? caster->get_instance_id() : 0;
    auto [it, is_new] = active_users.try_emplace({id, bc}, id, std::move(bytecode));
    PerIDData &data = it->second;

    if (data.free_invocation_count() == 0)
    {
        // OOM kill
        // TODO: proper notification!
        active_users.erase(it);
        return;
    }

    data.enqueue(entry);
}
auto
magix::execute::PerIDData::execute(ExecStack *stack, ExecutionContext &context) -> PerIDExecResult
{
    const auto max_invoc = max_invoc_count();
    std::vector<PerInstanceData> new_invocations;

    auto [prim_shared, obj_shared] = get_spans(global_memory, global_layout);
    for (auto &instance : instances)
    {
        auto [prim_fork, obj_fork] = get_spans(instance.memory, local_layout);
        PageInfo pageinfo{stack, array_size(stack->stack), array_size(stack->objbank), prim_shared, prim_fork, obj_fork, obj_shared};

        auto result = magix::execute::execute(_bytecode->get_code(), instance.entry, pageinfo, 100, context);
        switch (result.type)
        {
        case ExecResult::Type::OK_EXIT:
        {
            // exit, do nothing
            break;
        }
        case ExecResult::Type::OK_YIELD:
        {
            if (new_invocations.size() >= max_invoc)
            {
                // OOM kill
                return PerIDExecResult{true};
            }
            instance.entry = result.instruction_pointer;
            new_invocations.emplace_back(std::move(instance));
            break;
        }
        default:
            // traps
            return PerIDExecResult{true};
        }
    }
    instances = std::move(new_invocations);
    return PerIDExecResult{false};
}

auto
magix::execute::PerIDData::enqueue(magix::u16 entry) -> void
{
    instances.emplace_back(local_layout, entry);
}

auto
magix::execute::PerIDData::free_invocation_count() const -> size_t
{
    size_t max_instances = max_invoc_count();
    size_t instance_count = instances.size();
    if (max_instances < instance_count)
    {
        return 0;
    }
    else
    {
        return max_instances - instance_count;
    }
}

auto
magix::execute::PerIDData::max_invoc_count() const -> size_t
{
    if (magix_caster_max_mem < global_layout.total_size())
    {
        return 0;
    }
    size_t left_mem = magix_caster_max_mem - global_layout.total_size();
    size_t per_inst = magix_assumed_instance_overhead + local_layout.total_size();
    return left_mem / per_inst;
}

magix::execute::PerIDData::PerIDData(object_id_type id, godot::Ref<MagixByteCode> bytecode) : object_id(id), _bytecode(std::move(bytecode))
{
    if (!_bytecode.is_valid())
    {
        return;
    }
    auto &code = _bytecode->get_code();
    global_layout = ExecLayout(code.shared_size, code.obj_shared_count);
    local_layout = ExecLayout(code.fork_size, code.obj_fork_count);
    if (max_invoc_count() > 0)
    {
        global_memory.resize(global_layout.total_size());
    }
}
void
magix::execute::ExecRunner::clear()
{
    active_users.clear();
}
