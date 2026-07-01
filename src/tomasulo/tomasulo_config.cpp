#include "tomasulo_config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace Ripes::TomasuloSim {

static std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

Config parseConfig(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filename);
    }

    Config config;
    std::string line;
    int validCount = 0;

    while (std::getline(file, line)) {
        const auto separator = line.find(':');

        // Lines like "buffers" or "latencies" are ignored
        if (separator == std::string::npos) {
            continue;
        }

        std::string name = trim(line.substr(0, separator));
        std::string valueText = trim(line.substr(separator + 1));

        std::uint64_t value = std::stoull(valueText);

        if (name == "eff addr") {
            config.eff_addr_buffer_entries = value;
        } else if (name == "fp adds") {
            config.fp_add_buffer_entries = value;
        } else if (name == "fp muls") {
            config.fp_mul_buffer_entries = value;
        } else if (name == "ints") {
            config.int_buffer_entries = value;
        } else if (name == "reorder") {
            config.reorder_buffer_entries = value;
        } else if (name == "fp_add") {
            config.fp_add_buffer_latency = value;
        } else if (name == "fp_sub") {
            config.fp_sub_buffer_latency = value;
        } else if (name == "fp_mul") {
            config.fp_mul_buffer_latency = value;
        } else if (name == "fp_div") {
            config.fp_div_buffer_latency = value;
        } else {
            throw std::runtime_error("Unknown config parameter: " + name);
        }

        validCount++;
    }

    if (validCount != 9) {
        throw std::runtime_error(
            "Expected 9 config parameters, found " + std::to_string(validCount)
        );
    }

    return config;
}

void printConfig(const Config& config) {
    std::cout << "Configuration\n";
    std::cout << "-------------\n";
    std::cout << "buffers:\n";
    std::cout << "  eff addr: " << config.eff_addr_buffer_entries << "\n";
    std::cout << "  fp adds:  " << config.fp_add_buffer_entries << "\n";
    std::cout << "  fp muls:  " << config.fp_mul_buffer_entries << "\n";
    std::cout << "  ints:     " << config.int_buffer_entries << "\n";
    std::cout << "  reorder:  " << config.reorder_buffer_entries << "\n\n";

    std::cout << "latencies:\n";
    std::cout << "  fp add:   " << config.fp_add_buffer_latency << "\n";
    std::cout << "  fp sub:   " << config.fp_sub_buffer_latency << "\n";
    std::cout << "  fp mul:   " << config.fp_mul_buffer_latency << "\n";
    std::cout << "  fp div:   " << config.fp_div_buffer_latency << "\n";
}

} // namespace Ripes::TomasuloSim

