#pragma once

#include <string>
#include <cstdint>

namespace Ripes::TomasuloSim {

struct Config {
    std::uint64_t eff_addr_buffer_entries = 0;
    std::uint64_t fp_add_buffer_entries = 0;
    std::uint64_t fp_mul_buffer_entries = 0;
    std::uint64_t int_buffer_entries = 0;
    std::uint64_t reorder_buffer_entries = 0;

    std::uint64_t fp_add_buffer_latency = 0;
    std::uint64_t fp_sub_buffer_latency = 0;
    std::uint64_t fp_mul_buffer_latency = 0;
    std::uint64_t fp_div_buffer_latency = 0;
};

Config parseConfig(const std::string& filename);
void printConfig(const Config& config);

} // namespace Ripes::TomasuloSim

