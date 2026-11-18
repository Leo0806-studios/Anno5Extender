#pragma once

#include <Windows.h>
#include <atomic>
#include <cstdint>

namespace Anno5Proxy {

using BackendInitFunction = int(__cdecl*)(std::uint32_t, std::uint32_t);
using ExtenderInitFunction = int(__cdecl*)();

inline constexpr DWORD PathCapacity = 32768;
inline constexpr char ExtenderInitExportName[] = "Anno5Extender_Init";
inline constexpr char BackendInitExportName[] = "UPC_Init";

struct LoaderState {
	HMODULE selfModule = nullptr;
	HMODULE backendModule = nullptr;
	HMODULE extenderModule = nullptr;
	BackendInitFunction backendInit = nullptr;
	INIT_ONCE backendOnce = INIT_ONCE_STATIC_INIT;
	std::atomic_flag extenderInitializationAttempted = ATOMIC_FLAG_INIT;
	DWORD backendStatus = ERROR_IO_PENDING;
	wchar_t backendPath[PathCapacity]{};
	wchar_t extenderPath[PathCapacity]{};
	wchar_t logPath[PathCapacity]{};
};

extern thread_local bool loadingBackend;

LoaderState& GetLoaderState() noexcept;

void Log(const char* event, DWORD status = ERROR_SUCCESS) noexcept;

DWORD ResolveSiblingPath(const wchar_t* name, wchar_t (&output)[PathCapacity]) noexcept;

BOOL CALLBACK PrepareBackend(PINIT_ONCE initOnce, PVOID parameter, PVOID* context) noexcept;

void StartExtender() noexcept;

[[noreturn]] void FatalBackend(DWORD status) noexcept;

} // namespace Anno5Proxy
