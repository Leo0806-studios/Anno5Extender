#pragma once
#include <Windows.h>
#include <cstdint>
namespace A5Ext
{
	void Init();

	// Suggested later lifecycle hook. Not implemented/exported/called by this proxy.
	void Shutdown();

	namespace Bootstrap
	{
		inline constexpr char InitExportName[] = "Anno5Extender_Init";
		using InitFn = DWORD(__cdecl*)();
		// Optional future explicit shutdown boundary; never call it from DllMain.
		using ShutdownFn = DWORD(__cdecl*)();
	}
}

#ifdef ANNO5EXTENDER_EXPORTS
#define A5EXT_BOOTSTRAP_API __declspec(dllexport)
#else
#define A5EXT_BOOTSTRAP_API __declspec(dllimport)
#endif

// Minimal bootstrap boundary, separate from the future public plugin API.
// Zero = initialized; nonzero = failed/partial initialization. Never throw across it.
extern "C" A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Init() noexcept;
