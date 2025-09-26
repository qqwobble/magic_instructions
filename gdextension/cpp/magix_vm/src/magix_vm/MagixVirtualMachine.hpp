#ifndef MAGIX_MAGIXVIRTUALMACHINE_HPP_
#define MAGIX_MAGIXVIRTUALMACHINE_HPP_

#include <godot_cpp/classes/node.hpp>

#include "magix_vm/MagixByteCode.hpp"
#include "magix_vm/MagixCaster.hpp"
#include "magix_vm/execution/runner.hpp"

namespace magix
{

class MagixCaster;

class MagixVirtualMachine : public godot::Node
{
    GDCLASS(MagixVirtualMachine, godot::Node)

  public:
    MagixVirtualMachine() = default;
    ~MagixVirtualMachine() override = default;

    auto
    queue_execution(godot::Ref<MagixByteCode> bytecode, const godot::String entry, MagixCaster *caster) -> bool;

    void
    run(float)
    {
        runner.run_all();
    }

#if MAGIX_BUILD_TESTS
    static auto
    run_tests() -> int;
#endif

  protected:
    static void
    _bind_methods();

  private:
    ExecRunner runner;
};

} // namespace magix

#endif // MAGIX_MAGIXVIRTUALMACHINE_HPP_
