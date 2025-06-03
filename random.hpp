#pragma once

#include <cstddef>
#include <string>

namespace valkey_module
{
/// Generate random string with a given `length`
std::string generate_random_string(size_t length);
} // namespace valkey_module
