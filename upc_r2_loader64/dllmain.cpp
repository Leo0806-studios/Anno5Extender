#include <Windows.h>
#include <cstdint>

#include "Loader.h"

static_assert(sizeof(void*) == 8, "Anno5 requires an x64 proxy");
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);
		Anno5Proxy::GetLoaderState().selfModule = module;
	}
	return TRUE;
}

// Exported ONLY as UPC_Init @31 by the generated .def file.
extern "C" int __cdecl Proxy_UPC_Init(std::uint32_t version, std::uint32_t product) noexcept
{
	auto& state = Anno5Proxy::GetLoaderState();
	const DWORD incomingError = GetLastError();
	if (Anno5Proxy::loadingBackend) {
		Anno5Proxy::FatalBackend(ERROR_POSSIBLE_DEADLOCK);
	}
	if (!InitOnceExecuteOnce(&state.backendOnce, Anno5Proxy::PrepareBackend, nullptr, nullptr)) {
		Anno5Proxy::FatalBackend(GetLastError());
	}
	if (state.backendStatus != ERROR_SUCCESS || !state.backendInit) {
		Anno5Proxy::FatalBackend(state.backendStatus);
	}

	Anno5Proxy::StartExtender();
	SetLastError(incomingError);
	const int result = state.backendInit(version, product);
	const DWORD outgoingError = GetLastError();
	Anno5Proxy::Log("upc_init_return", static_cast<DWORD>(result));
	SetLastError(outgoingError);
	return result;
}

