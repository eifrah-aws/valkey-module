#include "random.hpp"

#include <random>
#include <string>

namespace valkey_module
{
namespace
{
    const std::string charset = "0123456789"
                                "abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

}

std::string generate_random_string(size_t length)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, charset.size() - 1);

    std::string output;
    output.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        output += charset[distribution(generator)];
    }
    return output;
}
} // namespace valkey_module