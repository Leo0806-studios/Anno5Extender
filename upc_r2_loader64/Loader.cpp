#include "Loader.h"

#include <bcrypt.h>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <string>
#include <strsafe.h>

#include "proxy_exports.generated.h"

#pragma comment(lib, "bcrypt.lib")

namespace Anno5Proxy {

thread_local bool loadingBackend = false;

LoaderState& GetLoaderState() noexcept
{
	static LoaderState state;
	return state;
}

void Log(const char* event, DWORD status) noexcept
{
	const DWORD savedError = GetLastError();
	try {
		const std::string line = "[Anno5Extender proxy] pid=" +
			std::to_string(GetCurrentProcessId()) + " " + event +
			" status=" + std::to_string(status) + "\r\n";
		OutputDebugStringA(line.c_str());
		const auto& logPath = GetLoaderState().logPath;
		if (!line.empty() && logPath[0]) {
			std::ofstream file(logPath, std::ios::out | std::ios::app | std::ios::binary);
			if (file) {
				file.write(line.data(), static_cast<std::streamsize>(line.size()));
			}
		}
	}
	catch (...) {
		OutputDebugStringA("[Anno5Extender proxy] logging failed\r\n");
	}
	SetLastError(savedError);
}

DWORD ResolveSiblingPath(const wchar_t* name, wchar_t (&output)[PathCapacity]) noexcept
{
	const auto& state = GetLoaderState();
	const DWORD size = GetModuleFileNameW(state.selfModule, output, PathCapacity);
	if (!size) return GetLastError();
	if (size >= PathCapacity) return ERROR_INSUFFICIENT_BUFFER;
	wchar_t* separator = wcsrchr(output, L'\\');
	if (!separator) return ERROR_BAD_PATHNAME;
	const auto available = PathCapacity - static_cast<DWORD>(separator + 1 - output);
	return SUCCEEDED(StringCchCopyW(separator + 1, available, name))
		? ERROR_SUCCESS
		: ERROR_INSUFFICIENT_BUFFER;
}

namespace {

DWORD CheckBackendHash() noexcept
{
	auto& state = GetLoaderState();
	HANDLE file = CreateFileW(state.backendPath, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return GetLastError();

	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	DWORD objectSize = 0;
	DWORD returned = 0;
	PUCHAR object = nullptr;
	DWORD result = ERROR_INVALID_DATA;

	if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0 &&
		BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &returned, 0) >= 0) {
		object = static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(), 0, objectSize));
		if (!object) {
			result = ERROR_NOT_ENOUGH_MEMORY;
		}
		else if (BCryptCreateHash(algorithm, &hash, object, objectSize, nullptr, 0, 0) >= 0) {
			BYTE buffer[32768]{};
			bool valid = true;
			for (;;) {
				DWORD bytes = 0;
				if (!ReadFile(file, buffer, sizeof(buffer), &bytes, nullptr)) {
					result = GetLastError();
					valid = false;
					break;
				}
				if (!bytes) break;
				if (BCryptHashData(hash, buffer, bytes, 0) < 0) {
					valid = false;
					break;
				}
			}

			BYTE digest[32]{};
			if (valid && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0) {
				char text[65]{};
				constexpr char digits[] = "0123456789abcdef";
				for (unsigned i = 0; i < 32; ++i) {
					text[2 * i] = digits[digest[i] >> 4];
					text[2 * i + 1] = digits[digest[i] & 15];
				}
				result = std::strcmp(text, A5ext::Proxy::Generated::BackendSha256) == 0
					? ERROR_SUCCESS
					: ERROR_REVISION_MISMATCH;
			}
		}
	}

	if (hash) BCryptDestroyHash(hash);
	if (object) HeapFree(GetProcessHeap(), 0, object);
	if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
	CloseHandle(file);
	return result;
}

} // namespace

BOOL CALLBACK PrepareBackend(PINIT_ONCE, PVOID, PVOID*) noexcept
{
	auto& state = GetLoaderState();
	loadingBackend = true;
	state.backendStatus = ResolveSiblingPath(L"Anno5Extender.proxy.log", state.logPath);
	if (!state.backendStatus) {
		state.backendStatus = ResolveSiblingPath(
			A5ext::Proxy::Generated::BackendFileName, state.backendPath);
	}
	if (!state.backendStatus) {
		state.backendStatus = ResolveSiblingPath(L"Anno5Extender.dll", state.extenderPath);
	}
	if (!state.backendStatus) state.backendStatus = CheckBackendHash();
	if (!state.backendStatus) {
		state.backendModule = LoadLibraryExW(
			state.backendPath, nullptr,
			LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (!state.backendModule) {
			state.backendStatus = GetLastError();
		}
		else {
			wchar_t loadedPath[PathCapacity]{};
			const DWORD loadedSize = GetModuleFileNameW(
				state.backendModule, loadedPath, PathCapacity);
			if (!loadedSize || loadedSize >= PathCapacity ||
				_wcsicmp(loadedPath, state.backendPath)) {
				state.backendStatus = ERROR_INVALID_DLL;
			}
			else {
				state.backendInit = reinterpret_cast<BackendInitFunction>(
					GetProcAddress(state.backendModule, BackendInitExportName));
				if (!state.backendInit) state.backendStatus = ERROR_PROC_NOT_FOUND;
			}
		}
	}
	loadingBackend = false;
	Log("backend_prepared", state.backendStatus);
	return TRUE;
}

[[noreturn]] void FatalBackend(DWORD status) noexcept
{
	Log("fatal_backend_error", status);
	SetLastError(status);
	RaiseFailFastException(nullptr, nullptr, 0);
	TerminateProcess(GetCurrentProcess(), status);
	__assume(false);
}

void StartExtender() noexcept
{
	auto& state = GetLoaderState();
	if (state.extenderInitializationAttempted.test_and_set(std::memory_order_relaxed)) return;

	state.extenderModule = LoadLibraryExW(
		state.extenderPath, nullptr,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	if (!state.extenderModule) {
		Log("extender_load_failed", GetLastError());
		return;
	}

	auto init = reinterpret_cast<ExtenderInitFunction>(
		GetProcAddress(state.extenderModule, ExtenderInitExportName));
	if (!init) {
		Log("extender_init_export_missing", ERROR_PROC_NOT_FOUND);
		return;
	}

	DWORD result = ERROR_UNHANDLED_EXCEPTION;
	try {
		result = init()== ERROR_SUCCESS ? ERROR_SUCCESS : ERROR_DLL_INIT_FAILED;
	}
	catch (...) {
		Log("extender_init_threw", result);
	}
	Log("extender_init_finished", result);
	// Retain the module because failed or partial initialization may register callbacks.
}

} // namespace Anno5Proxy
