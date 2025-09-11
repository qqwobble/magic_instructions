#include "magix_vm/convert_magix_godot.hpp"
#include "magix_vm/compilation/lexer.hpp"

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/variant/string.hpp"

#include <cstring>

[[nodiscard]] auto
magix::strview_to_godot(SrcView str) -> godot::String
{
    godot::String out;
    godot::Error err = out.resize(str.length() + 1);
    if (err != godot::Error::OK)
    {
        *std::copy(str.begin(), str.end(), out.ptrw()) = 0;
    }
    return out;
}
