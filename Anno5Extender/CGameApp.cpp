#include <intrin.h>
#include "CGameApp.h"
#include <cstdint>
#include "Log.h"

// Generated bindings: validate executable/signatures before initializing.
// Compile once; do not provide competing import-library symbols.
#pragma comment(linker, "/alternatename:__imp_?InitFileSystem@CGameApp@Application@Runtime@Anno2070@AddressLibrary@A5Ext@@QEAA_NXZ=CGameApp_initFileSystem")
#pragma comment(linker, "/alternatename:__imp_?InitInstance@CGameApp@Application@Runtime@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CGameApp_initInstance")
#pragma comment(linker, "/alternatename:__imp_?RunMainLoop@CGameApp@Application@Runtime@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CGameApp_runMainLoop")

extern "C" {
    A5Ext::AddressLibrary::DllEntry CGameApp_initFileSystem = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CGameApp_initInstance = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CGameApp_runMainLoop = &A5Ext::AddressLibrary::Unbound;
}

namespace A5Ext::AddressLibrary {

template <>
class BindingStatic<A5Ext::AddressLibrary::Anno2070::Runtime::Application::CGameApp> {
public:
    // Deliberate compile blocker: never run a zero-RVA binding.
    static_assert(false, "Unfinished binding template: fill every zero RVA and remove this assertion, or regenerate with --rva-map.");
    BindingStatic() {
Logger::GlobalLog("Binding A5Ext::AddressLibrary::Anno2070::Runtime::Application::CGameAppfunctions to their addresses in the executable.");
        CGameApp_initFileSystem = reinterpret_cast<DllEntry>(this->GetAddress(0x0)); // TODO: verified RVA
        CGameApp_initInstance = reinterpret_cast<DllEntry>(this->GetAddress(0x0)); // TODO: verified RVA
        CGameApp_runMainLoop = reinterpret_cast<DllEntry>(this->GetAddress(0x0)); // TODO: verified RVA
Logger::GlobalLog("Binding A5Ext::AddressLibrary::Anno2070::Runtime::Application::CGameAppfunctions complete.");
    }

private:
    // A specialization does not inherit the primary template helper.
    std::uintptr_t GetAddress(std::uintptr_t rva) const {
        return Module::GetInstance().GetBaseAddress() + rva;
    }
};

// No global instance: call explicitly AFTER executable validation.
// Declared in the corresponding class header.
void Initialize_CGameAppBindings() {
    static BindingStatic<A5Ext::AddressLibrary::Anno2070::Runtime::Application::CGameApp> binding;
    (void)binding;
}

} // namespace A5Ext::AddressLibrary
