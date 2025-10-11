#ifndef MAGIX_EXECUTION_CONFIG_HPP_
#define MAGIX_EXECUTION_CONFIG_HPP_

#include <cstddef>
#include <cstdint>

namespace magix::execute
{

using object_id_type = uint64_t;

constexpr size_t stack_size_default = 65536;
constexpr size_t objbank_size_default = 4096;

/** Limit caster-slot memory usage to 256KiB. Slots are independent. This way there is no runaway OOM scenario. */
constexpr size_t memory_per_caster_max = 1024 * 256;
/** Layout assumes multiple of granularity. */
constexpr size_t memory_granularity = 64;
/** Amount of memory added to use per instance, to avoid spells being (nearly) free. */
constexpr size_t memory_assumed_instance_overhead = 64;

constexpr float maintenance_cost = 1.0 / 60.0;

} // namespace magix::execute

#endif // MAGIX_EXECUTION_CONFIG_HPP_
