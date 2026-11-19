// Standalone MSVC x64 binding example. NOT part of a project.
// Includes the production declarations without changing them.
// This demonstrates manually supplied __imp_ pointer symbols, NOT a PE import
// descriptor referencing nonexistent exports in Anno5.exe.
#include "Anno5Extender/CBehaviourAutoBuild.h"
#include <atomic>
#include <cstdlib>
#include <limits>

namespace AutoBuildBindingExample {
using GameObject = A5Ext::AddressLibrary::Anno2070::AI::Behaviour::CBehaviourAutoBuild;
using Entry = void (__cdecl*)(GameObject*);

[[noreturn]] void __cdecl Unbound(GameObject*) { std::abort(); }
static std::atomic<bool> ready{false};

// Initialize once, BEFORE publishing objects/callers or starting plugin threads.
// The caller must first validate the supported executable hash, function bytes
// and exact signatures. RVAs below are inputs, not guessed engine addresses.
// Do not rebind these non-atomic pointer slots while calls are possible.
bool Bind(std::uintptr_t imageBase, std::size_t imageSize,
          std::uintptr_t initRva, std::uintptr_t stepRva);
}

// These identifiers are ordinary C++ identifiers with C linkage (x64: no leading
// underscore). Their values are function pointers, not executable stub bodies.
extern "C" {
    AutoBuildBindingExample::Entry AnnoAutoBuildExample_InitSlot = &AutoBuildBindingExample::Unbound;
    AutoBuildBindingExample::Entry AnnoAutoBuildExample_StepSlot = &AutoBuildBindingExample::Unbound;
}

// Exact decorated references emitted for the production imported methods.
// /alternatename resolves each otherwise-unresolved __imp_ name to our DATA slot.
// Punctuation is legal here because the linker names are inside strings.
// Do NOT also link an import library supplying these same __imp_ symbols: an
// existing definition takes precedence over this fallback alias.
#pragma comment(linker, "/alternatename:__imp_?Init@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=AnnoAutoBuildExample_InitSlot")
#pragma comment(linker, "/alternatename:__imp_?Step@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=AnnoAutoBuildExample_StepSlot")

namespace AutoBuildBindingExample {
bool Bind(std::uintptr_t imageBase, std::size_t imageSize,
          std::uintptr_t initRva, std::uintptr_t stepRva) {
    if (ready.load(std::memory_order_acquire)) return false;
    if (!imageBase || !imageSize || !initRva || !stepRva ||
        initRva >= imageSize || stepRva >= imageSize ||
        imageBase > (std::numeric_limits<std::uintptr_t>::max)() - imageSize)
        return false;
    AnnoAutoBuildExample_InitSlot = reinterpret_cast<Entry>(imageBase + initRva);
    AnnoAutoBuildExample_StepSlot = reinterpret_cast<Entry>(imageBase + stepRva);
    ready.store(true, std::memory_order_release);
    return true;
}

// Normal virtual dispatch: uses the object's EXISTING native vfptr and hence
// the actual dynamic object's override. Does not reference our imported slots.
void VirtualStep(GameObject* object) {
    if (!object) std::abort();
    object->Step();
}

// Explicit qualification suppresses virtual dispatch. MSVC emits an imported
// call through __imp_?Step..., resolved above to our manually populated slot.
// This intentionally calls this class's bound implementation, even when the
// object's dynamic type would override it. It is NOT normal virtual dispatch.
void ExactClassStep(GameObject* object) {
    if (!object || !ready.load(std::memory_order_acquire)) std::abort();
    object->GameObject::Step();
}

void ExactClassInit(GameObject* object) {
    if (!object || !ready.load(std::memory_order_acquire)) std::abort();
    object->GameObject::Init();
}

// No constructor, destructor, local instance, new, delete, placement-new or
// vfptr write is used anywhere above. No local vtable is required by these calls.
// Newly created objects must come from a separately verified native factory /
// allocator + constructor + registration sequence, then can use the same calls.
// Do not instantiate the production mirror with C++ new: its local constructors
// and std::vector layout/lifetime do not become native merely by binding methods.
// Slot/signature/layout verification is still required for ANY engine object.
// This example only supplies Init/Step imports. Generate additional slots/aliases
// from compiler-emitted raw symbol names as other DIRECT imported calls are used.
}

#ifdef ANNO_AUTOBUILD_BINDING_EXAMPLE_TEST
// Owned ABI fixture only; never loads a game DLL or calls an engine address.
// Fake raw storage exercises MSVC x64 dispatch, not ISO C++ object construction.
static unsigned virtualCount, initCount, directCount;
static void __cdecl MockVirtual(AutoBuildBindingExample::GameObject*) { ++virtualCount; }
static void __cdecl MockInit(AutoBuildBindingExample::GameObject*) { ++initCount; }
static void __cdecl MockStep(AutoBuildBindingExample::GameObject*) { ++directCount; }
int main() {
    using namespace AutoBuildBindingExample;
    const auto init = reinterpret_cast<std::uintptr_t>(&MockInit);
    const auto step = reinterpret_cast<std::uintptr_t>(&MockStep);
    const auto low = (init < step ? init : step) - 1;
    const auto high = init > step ? init : step;
    if (Bind(0, 0, 0, 0)) return 1;
    if (!Bind(low, high - low + 1, init - low, step - low)) return 2;
    if (Bind(low, high - low + 1, init - low, step - low)) return 3;
    // Slot4 follows the CURRENT production header. No claim of native discovery.
    Entry slots[8]{}; slots[4] = &MockVirtual;
    struct RawObject { Entry* vfptr; } storage{slots};
    auto* view = reinterpret_cast<GameObject*>(&storage);
    VirtualStep(view);
    ExactClassStep(view);
    ExactClassInit(view);
    return virtualCount == 1 && directCount == 1 && initCount == 1 &&
           storage.vfptr == slots ? 0 : 4;
}
#endif
