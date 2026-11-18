#include <Windows.h>
#include <atomic>
#include <cstdint>
static std::atomic<unsigned> calls{}, bad_error{};
extern "C" int __cdecl FixtureUpcInit(std::uint32_t a, std::uint32_t b)
{
    if (GetLastError() != 0x4321) ++bad_error;
    ++calls;
    SetLastError(0x8765);
    return static_cast<int>(a ^ b ^ 0xdeadbeefU);
}
extern "C" std::uint64_t __cdecl FixtureForward(std::uint64_t a, std::uint64_t b, std::uint64_t c, std::uint64_t d)
{
    return a ^ b ^ c ^ d;
}
extern "C" __declspec(dllexport) unsigned FixtureCalls() { return calls.load(); }
extern "C" __declspec(dllexport) unsigned FixtureBadError() { return bad_error.load(); }
