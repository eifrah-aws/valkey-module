#include "random.hpp"
#include "valkeymodule.h"

#include <iostream>

#define VALKEY_API extern "C"

static void parse_command_line_args(ValkeyModuleString** argv, int argc)
{
    /* Log the list of parameters passing loading the module. */
    for (size_t j = 0; j < argc; ++j) {
        const char* s = ValkeyModule_StringPtrLen(argv[j], nullptr);
        std::cout << "Module loaded with ARGV: " << s << std::endl;
    }
}

int command_uuid(ValkeyModuleCtx* ctx, [[maybe_unused]] ValkeyModuleString** argv, [[maybe_unused]] int argc)
{
    // Sanity
    if (argc != 2) {
        ValkeyModule_ReplyWithError(ctx, "ERR wrong number of arguments");
        return VALKEYMODULE_OK;
    }

    auto lenstr = ValkeyModule_StringPtrLen(argv[1], nullptr);
    int len = ::atoi(lenstr);

    // Put a limit on the length, just for the fun
    if (len < 5) {
        ValkeyModule_ReplyWithError(ctx, "ERR uuid len must be greater or equal to 5");
        return VALKEYMODULE_OK;
    }

    auto str = valkey_module::generate_random_string(len);
    ValkeyModule_ReplyWithSimpleString(ctx, str.c_str());
    return VALKEYMODULE_OK;
}

/// This function must be present on each module. It is used in order to
/// register the commands into the server
VALKEY_API int ValkeyModule_OnLoad(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
    if (ValkeyModule_Init(ctx, "valkey-module", 1, VALKEYMODULE_APIVER_1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    // Register a new command, "uuid"
    if (ValkeyModule_CreateCommand(ctx, "uuid", command_uuid, "readonly", 0, 0, 0) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    // A module can accept arguments
    parse_command_line_args(argv, argc);
    return VALKEYMODULE_OK;
}
