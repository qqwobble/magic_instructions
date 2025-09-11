#ifndef MAGIX_CONVERT_MAGIX_GODOT_HPP_
#define MAGIX_CONVERT_MAGIX_GODOT_HPP_

#include "godot_cpp/variant/string.hpp"
#include "magix_vm/compilation/lexer.hpp"
namespace magix
{

[[nodiscard]] auto inline strview_from_godot(const godot::String &str) -> SrcView
{
    return {str.ptr(), (size_t)str.length()};
}

[[nodiscard]] auto
strview_to_godot(SrcView str) -> godot::String;

} // namespace magix

#endif // MAGIX_CONVERT_MAGIX_GODOT_HPP_
