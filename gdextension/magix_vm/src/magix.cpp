#include "godot_cpp/classes/node.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

class MagixVM : public godot::Node
{
    GDCLASS(MagixVM, godot::Node)

  protected:
    static void
    _bind_methods();

  public:
    MagixVM() {}
    ~MagixVM() {}

    void
    reset_and_execute(float delta)
    {
        process_index = 0;
        execute_remaining(delta);
    }

    void
    execute_remaining(float delta)
    {}

  private:
    size_t process_index = 0;
};

void
MagixVM::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("reset_and_execute", "delta"), &MagixVM::reset_and_execute);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_remaining", "delta"), &MagixVM::execute_remaining);
}

void
magix_vm_init_lib(godot::ModuleInitializationLevel p_level)
{
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_RUNTIME_CLASS(MagixVM);
}

void
magix_vm_deinit_lib(godot::ModuleInitializationLevel p_level)
{
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
}

extern "C"
{

GDExtensionBool GDE_EXPORT
magix_vm_lib_entry(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
)
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(magix_vm_init_lib);
    init_obj.register_terminator(magix_vm_deinit_lib);
    init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
