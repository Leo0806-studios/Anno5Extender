#include <Windows.h>
#include <atomic>
#include <cstring>
#include <cstdint>
static std::atomic<unsigned> calls{};
static DWORD WINAPI Reenter(void*)
{
    using Init = int(__cdecl*)(std::uint32_t, std::uint32_t);
    auto init = reinterpret_cast<Init>(GetProcAddress(GetModuleHandleW(L"upc_r2_loader64.dll"), "UPC_Init"));
    SetLastError(0x4321);
    return static_cast<DWORD>(init(3, 4));
}
extern "C" __declspec(dllexport) DWORD __cdecl Anno5Extender_Init() noexcept
{
    ++calls;
    char mode[64]{};
    GetEnvironmentVariableA("ANNO_FIXTURE_MODE", mode, sizeof(mode));
    if (!strcmp(mode, "concurrent")) Sleep(150);
    if (!strcmp(mode, "reentrant")) {
        HANDLE worker = CreateThread(nullptr, 0, Reenter, nullptr, 0, nullptr);
        if (!worker) return 1001;
        DWORD wait = WaitForSingleObject(worker, 5000);
        CloseHandle(worker);
        if (wait != WAIT_OBJECT_0) return 1002;
    }
    return !strcmp(mode, "failed") ? ERROR_ACCESS_DENIED : ERROR_SUCCESS;
}
extern "C" __declspec(dllexport) unsigned FixtureCalls() { return calls.load(); }
