#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace aegis {
std::vector<std::string> subsystem_signals(std::uint64_t subject, std::uint32_t epoch);
std::string subsystem_inventory();
}
