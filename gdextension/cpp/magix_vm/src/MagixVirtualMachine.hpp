#ifndef MAGIXVIRTUALMACHINE_HPP_
#define MAGIXVIRTUALMACHINE_HPP_

#include <godot_cpp/classes/node.hpp>

#include "MagixByteCode.hpp"

namespace magix
{

class MagixVirtualMachine : public godot::Node
{
    GDCLASS(MagixVirtualMachine, godot::Node)

  public:
    MagixVirtualMachine() {}
    ~MagixVirtualMachine() {}

    void
    queue_execution(magix::MagixByteCode *byte_code, const godot::String &entry);

    void
    reset_and_execute(float delta);

    void
    execute_remaining(float delta);

#if MAGIX_BUILD_TESTS
    static int
    run_tests();
#endif

  protected:
    static void
    _bind_methods();

  private:
    size_t process_index = 0;
    size_t process_end = 0;
};

} // namespace magix

#endif // MAGIXVIRTUALMACHINE_HPP_
