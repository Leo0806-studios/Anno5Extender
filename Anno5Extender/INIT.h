#pragma once
#pragma warning(push,0)
#include <Windows.h>
#pragma warning(pop)
#include <cstdint>
namespace A5Ext
{
	void Init();

	void Shutdown();

}

#ifdef ANNO5EXTENDER_EXPORTS
#define A5EXT_BOOTSTRAP_API __declspec(dllexport)
#else
#define A5EXT_BOOTSTRAP_API __declspec(dllimport)
#endif

// Minimal bootstrap boundary, separate from the future public plugin API.
// Zero = initialized; nonzero = failed/partial initialization. Never throw across it.
extern "C" A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Init() noexcept;

extern "C" A5EXT_BOOTSTRAP_API void __cdecl Anno5Extender_Shutdown() noexcept;
