#include <intrin.h>
#include "CBehaviourAutoBuild.h"
#include <cstdint>

// Generated bindings: validate executable/signatures before initializing.
// Compile once; do not provide competing import-library symbols.
#pragma comment(linker, "/alternatename:__imp_?Init@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_init")
#pragma comment(linker, "/alternatename:__imp_?ScalarDeletingDestructor@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAPEAXI@Z=CBehaviourAutoBuild_scalarDeletingDestructor")
#pragma comment(linker, "/alternatename:__imp_?Serialize@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAA_NPEAX_KI@Z=CBehaviourAutoBuild_serialize")
#pragma comment(linker, "/alternatename:__imp_?Step@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_step")
#pragma comment(linker, "/alternatename:__imp_?unknown_0x10@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_unknown_0x10")
#pragma comment(linker, "/alternatename:__imp_?unknown_0x18@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_unknown_0x18")
#pragma comment(linker, "/alternatename:__imp_?unknown_0x30@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_unknown_0x30")
#pragma comment(linker, "/alternatename:__imp_?unknown_0x38@CBehaviourAutoBuild@Behaviour@AI@Anno2070@AddressLibrary@A5Ext@@UEAAXXZ=CBehaviourAutoBuild_unknown_0x38")

extern "C" {
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_init = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_scalarDeletingDestructor = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_serialize = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_step = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_unknown_0x10 = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_unknown_0x18 = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_unknown_0x30 = &A5Ext::AddressLibrary::Unbound;
    A5Ext::AddressLibrary::DllEntry CBehaviourAutoBuild_unknown_0x38 = &A5Ext::AddressLibrary::Unbound;
}

namespace A5Ext::AddressLibrary {

    template <>
    class BindingStatic<A5Ext::AddressLibrary::Anno2070::AI::Behaviour::CBehaviourAutoBuild> {
    public:
        BindingStatic() {
            CBehaviourAutoBuild_init = reinterpret_cast<DllEntry>(this->GetAddress(0x6b76d0)); // TODO: verified RVA
            CBehaviourAutoBuild_scalarDeletingDestructor = reinterpret_cast<DllEntry>(this->GetAddress(0x6b7670)); // TODO: verified RVA
            CBehaviourAutoBuild_serialize = reinterpret_cast<DllEntry>(this->GetAddress(0x6b7990)); // TODO: verified RVA
            CBehaviourAutoBuild_step = reinterpret_cast<DllEntry>(this->GetAddress(0x6b7890)); // TODO: verified RVA
            CBehaviourAutoBuild_unknown_0x10 = reinterpret_cast<DllEntry>(this->GetAddress(0x8bff0)); // TODO: verified RVA
            CBehaviourAutoBuild_unknown_0x18 = reinterpret_cast<DllEntry>(this->GetAddress(0x8bff0)); // TODO: verified RVA
            CBehaviourAutoBuild_unknown_0x30 = reinterpret_cast<DllEntry>(this->GetAddress(0x8bff0)); // TODO: verified RVA
            CBehaviourAutoBuild_unknown_0x38 = reinterpret_cast<DllEntry>(this->GetAddress(0x8bff0)); // TODO: verified RVA
        }

    private:
        // A specialization does not inherit the primary template helper.
        std::uintptr_t GetAddress(std::uintptr_t rva) const {
            return Module::GetInstance().GetBaseAddress() + rva;
        }
    };
        BindingStatic<A5Ext::AddressLibrary::Anno2070::AI::Behaviour::CBehaviourAutoBuild> binding;

} // namespace A5Ext::AddressLibrary
