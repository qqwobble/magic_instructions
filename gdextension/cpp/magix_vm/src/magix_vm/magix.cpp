#include "magix_vm/MagixAsmProgram.hpp"
#include "magix_vm/MagixVirtualMachine.hpp"

void
magix_vm_init_lib(godot::ModuleInitializationLevel p_level)
{
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    GDREGISTER_RUNTIME_CLASS(magix::MagixByteCode);
    GDREGISTER_RUNTIME_CLASS(magix::MagixAsmProgram);
    GDREGISTER_RUNTIME_CLASS(magix::MagixVirtualMachine);
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

auto GDE_EXPORT
magix_vm_lib_entry(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    const GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization
) -> GDExtensionBool
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(magix_vm_init_lib);
    init_obj.register_terminator(magix_vm_deinit_lib);
    init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
