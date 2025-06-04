#include "valkeymodule.h"

#include <string>

#define VALKEY_API extern "C"

ValkeyModuleType* PairType = nullptr;

namespace
{
std::string from_valkey_string(ValkeyModuleString* s, bool release = false)
{
    size_t len = 0;
    const char* str = ValkeyModule_StringPtrLen(s, &len);
    std::string res{ str, len };
    if (release) {
        ValkeyModule_FreeString(nullptr, s);
    }
    return res;
}
} // namespace

/// Our new data type
class ValkeyPair final
{
public:
    /// Called when a "pair.set" command is issued
    static int on_set(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc);

    /// Called when a "pair.get" command is issued
    static int on_get(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc);

    /// Register this type with Valkey
    static int register_type(ValkeyModuleCtx* ctx);

    /// Needed by valkey
    static void* rdb_load(ValkeyModuleIO* rdb, int encver);
    static void rdb_save(ValkeyModuleIO* rdb, void* value);
    static void aof_rewrite(ValkeyModuleIO* aof, ValkeyModuleString* key, void* value);
    static void free(void* value);

private:
    std::string first;
    std::string second;
};

int ValkeyPair::register_type(ValkeyModuleCtx* ctx)
{
    // Create the module type
    ValkeyModuleTypeMethods tm = { .version = VALKEYMODULE_TYPE_METHOD_VERSION,
                                   .rdb_load = ValkeyPair::rdb_load,
                                   .rdb_save = ValkeyPair::rdb_save,
                                   .aof_rewrite = ValkeyPair::aof_rewrite,
                                   .free = ValkeyPair::free };

    PairType = ValkeyModule_CreateDataType(ctx, "vk___pair", 0, &tm);

    // Register commands
    if (ValkeyModule_CreateCommand(ctx, "pair.set", ValkeyPair::on_set, "write", 1, 1, 1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    if (ValkeyModule_CreateCommand(ctx, "pair.get", ValkeyPair::on_get, "readonly", 1, 1, 1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }
    return VALKEYMODULE_OK;
}

// Command to create a new pair
int ValkeyPair::on_set(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
    if (argc != 4) {
        return ValkeyModule_WrongArity(ctx);
    }

    ValkeyModuleKey* key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_WRITE);

    // Create new pair
    ValkeyPair* pair = static_cast<ValkeyPair*>(ValkeyModule_Alloc(sizeof(ValkeyPair)));

    // Call the c-tor
    new (pair) ValkeyPair();

    pair->first = from_valkey_string(argv[2]);
    pair->second = from_valkey_string(argv[3]);

    // Set the value
    ValkeyModule_ModuleTypeSetValue(key, PairType, pair);
    ValkeyModule_ReplyWithSimpleString(ctx, "OK");

    ValkeyModule_CloseKey(key);
    return VALKEYMODULE_OK;
}

// Command to create a new pair
int ValkeyPair::on_get(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
    if (argc != 2) {
        return ValkeyModule_WrongArity(ctx);
    }

    ValkeyModuleKey* key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ);
    if (key == nullptr) {
        ValkeyModule_ReplyWithNullArray(ctx);
        return VALKEYMODULE_OK;
    }

    // Create new pair
    ValkeyPair* pair = static_cast<ValkeyPair*>(ValkeyModule_ModuleTypeGetValue(key));
    ValkeyModule_ReplyWithArray(ctx, 2);
    ValkeyModule_ReplyWithCString(ctx, pair->first.c_str());
    ValkeyModule_ReplyWithCString(ctx, pair->second.c_str());

    ValkeyModule_CloseKey(key);
    return VALKEYMODULE_OK;
}

void* ValkeyPair::rdb_load(ValkeyModuleIO* rdb, int encver)
{
    if (encver != 0) {
        // Handle wrong version
        return nullptr;
    }

    ValkeyPair* pair = static_cast<ValkeyPair*>(ValkeyModule_Alloc(sizeof(ValkeyPair)));
    pair->first = from_valkey_string(ValkeyModule_LoadString(rdb), true);
    pair->second = from_valkey_string(ValkeyModule_LoadString(rdb), true);
    return pair;
}

void ValkeyPair::rdb_save(ValkeyModuleIO* rdb, void* value)
{
    ValkeyPair* pair = static_cast<ValkeyPair*>(value);

    auto first = ValkeyModule_CreateString(nullptr, pair->first.c_str(), pair->first.length());
    auto second = ValkeyModule_CreateString(nullptr, pair->second.c_str(), pair->second.length());

    ValkeyModule_SaveString(rdb, first);
    ValkeyModule_SaveString(rdb, second);

    ValkeyModule_FreeString(nullptr, first);
    ValkeyModule_FreeString(nullptr, second);
}

void ValkeyPair::aof_rewrite(ValkeyModuleIO* aof, ValkeyModuleString* key, void* value)
{
    ValkeyPair* pair = static_cast<ValkeyPair*>(value);

    auto first = ValkeyModule_CreateString(nullptr, pair->first.c_str(), pair->first.length());
    auto second = ValkeyModule_CreateString(nullptr, pair->second.c_str(), pair->second.length());

    // Reconstruct the command that created this pair
    ValkeyModule_EmitAOF(aof,
                         "pair.set",
                         "sss", // 's' means ValkeyModuleString
                         key,   // the key name
                         first,
                         second);
    ValkeyModule_FreeString(nullptr, first);
    ValkeyModule_FreeString(nullptr, second);
}

void ValkeyPair::free(void* value)
{
    if (value == NULL)
        return;

    ValkeyPair* pair = static_cast<ValkeyPair*>(value);
    ValkeyModule_Free(pair);
}

/// This function must be present on each module. It is used in order to
/// register the commands into the server
VALKEY_API int ValkeyModule_OnLoad(ValkeyModuleCtx* ctx, ValkeyModuleString** argv, int argc)
{
    if (ValkeyModule_Init(ctx, "valkey-pair-module", 1, VALKEYMODULE_APIVER_1) == VALKEYMODULE_ERR) {
        return VALKEYMODULE_ERR;
    }

    // Register our type
    return ValkeyPair::register_type(ctx);
}
