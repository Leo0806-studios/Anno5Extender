#include <atomic>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <strsafe.h>
#include <Windows.h>
#include "../Anno5Extender/INIT.h"
#include "proxy_exports.generated.h"

static_assert(sizeof(void*) == 8, "Anno5 requires an x64 proxy");
namespace {
	using UpcInit = int(__cdecl*)(std::uint32_t, std::uint32_t);
	constexpr DWORD path_capacity = 32768;
	HMODULE self_module;
	HMODULE backend_module; // Deliberately retained until process exit.
	HMODULE extender_module;
	UpcInit real_init;
	INIT_ONCE backend_once = INIT_ONCE_STATIC_INIT;
	std::atomic_flag extender_attempted = ATOMIC_FLAG_INIT;
	thread_local bool loading_backend = false;
	DWORD backend_status = ERROR_IO_PENDING;
	wchar_t backend_path[path_capacity], extender_path[path_capacity], log_path[path_capacity];

	// Logging is best-effort; a read-only installation must not break delegation.
	void Log(const char* event, DWORD status = ERROR_SUCCESS) noexcept
	{
		const DWORD saved = GetLastError();
		char line[256]{};
		const int length = sprintf_s(line, "[Anno5Extender proxy] pid=%lu %s status=%lu\r\n",
									GetCurrentProcessId(), event, status);
		OutputDebugStringA(line);
		if (length > 0 && log_path[0]) {
			HANDLE file = CreateFileW(log_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
									  nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file != INVALID_HANDLE_VALUE) {
				DWORD written = 0;
				WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr);
				CloseHandle(file);
			}
		}
		SetLastError(saved);
	}

	DWORD SiblingPath(const wchar_t* name, wchar_t(&output)[path_capacity]) noexcept
	{
		const DWORD size = GetModuleFileNameW(self_module, output, path_capacity);
		if (!size) return GetLastError();
		if (size >= path_capacity) return ERROR_INSUFFICIENT_BUFFER;
		wchar_t* separator = wcsrchr(output, L'\\');
		if (!separator) return ERROR_BAD_PATHNAME;
		const auto available = path_capacity - static_cast<DWORD>(separator + 1 - output);
		return SUCCEEDED(StringCchCopyW(separator + 1, available, name)) ? ERROR_SUCCESS : ERROR_INSUFFICIENT_BUFFER;
	}

	DWORD CheckBackendHash() noexcept
	{
		HANDLE file = CreateFileW(backend_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
								  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return GetLastError();
		BCRYPT_ALG_HANDLE algorithm = nullptr;
		BCRYPT_HASH_HANDLE hash = nullptr;
		DWORD object_size = 0, returned = 0;
		PUCHAR object = nullptr;
		DWORD result = ERROR_INVALID_DATA;
		if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0 &&
			BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size),
							  sizeof(object_size), &returned, 0) >= 0) {
			object = static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(), 0, object_size));
			if (!object) result = ERROR_NOT_ENOUGH_MEMORY;
			else if (BCryptCreateHash(algorithm, &hash, object, object_size, nullptr, 0, 0) >= 0) {
				BYTE buffer[32768] = {0};
				bool valid = true;
				for (;;) {
					DWORD bytes = 0;
					if (!ReadFile(file, buffer, sizeof(buffer), &bytes, nullptr)) {
						result = GetLastError(); valid = false; break;
					}
					if (!bytes) break;
					if (BCryptHashData(hash, buffer, bytes, 0) < 0) {
						valid = false; break;
					}
				}
				BYTE digest[32]{};
				if (valid && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0) {
					char text[65]{};
					constexpr char digits[] = "0123456789abcdef";
					for (unsigned i = 0; i < 32; ++i) {
						text[2 * i] = digits[digest[i] >> 4]; text[2 * i + 1] = digits[digest[i] & 15];
					}
					result = strcmp(text, A5Ext::Proxy::Generated::BackendSha256) == 0
						? ERROR_SUCCESS : ERROR_REVISION_MISMATCH;
				}
			}
		}
		if (hash) BCryptDestroyHash(hash);
		if (object) HeapFree(GetProcessHeap(), 0, object);
		if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
		CloseHandle(file);
		return result;
	}

	BOOL CALLBACK PrepareBackend(PINIT_ONCE, PVOID, PVOID*) noexcept
	{
		loading_backend = true;
		backend_status = SiblingPath(L"Anno5Extender.proxy.log", log_path);
		if (!backend_status) backend_status = SiblingPath(A5Ext::Proxy::Generated::BackendFileName, backend_path);
		if (!backend_status) backend_status = SiblingPath(L"Anno5Extender.dll", extender_path);
		if (!backend_status) backend_status = CheckBackendHash();
		if (!backend_status) {
			backend_module = LoadLibraryExW(backend_path, nullptr,
				LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
			if (!backend_module) backend_status = GetLastError();
			else {
				wchar_t loaded[path_capacity]{};
				const DWORD loaded_size = GetModuleFileNameW(backend_module, loaded, path_capacity);
				if (!loaded_size || loaded_size >= path_capacity || _wcsicmp(loaded, backend_path))
					backend_status = ERROR_INVALID_DLL;
				else {
					real_init = reinterpret_cast<UpcInit>(GetProcAddress(backend_module, "UPC_Init"));
					if (!real_init) backend_status = ERROR_PROC_NOT_FOUND;
				}
			}
		}
		loading_backend = false;
		Log("backend_prepared", backend_status);
		return TRUE; // Cache failures as well as success; never fabricate a UPC result.
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
		// Do not wait for another initializer. Recursive/concurrent UPC calls must
		// still delegate, rather than deadlocking on an initializer/worker join.
		if (extender_attempted.test_and_set(std::memory_order_relaxed)) return;
		extender_module = LoadLibraryExW(extender_path, nullptr,
			LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (!extender_module) {
			Log("extender_load_failed", GetLastError()); return;
		}
		auto init = reinterpret_cast<A5Ext::Bootstrap::InitFn>(
			GetProcAddress(extender_module, A5Ext::Bootstrap::InitExportName));
		if (!init) {
			Log("extender_init_export_missing", ERROR_PROC_NOT_FOUND); return;
		}
		DWORD result = ERROR_UNHANDLED_EXCEPTION;
		try {
			result = init();
		}
		catch (...) {
			Log("extender_init_threw", result);
		}
		Log("extender_init_finished", result);
		// Even failed/partial Init may have callbacks. Retain the DLL; do not unload
		// unknown live code or retry initialization automatically.
	}
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) self_module = module;
	return TRUE;
}

// Exported ONLY as UPC_Init @31 by the generated .def file.
extern "C" int __cdecl Proxy_UPC_Init(std::uint32_t version, std::uint32_t product) noexcept
{
	const DWORD incoming_error = GetLastError();
	if (loading_backend) FatalBackend(ERROR_POSSIBLE_DEADLOCK);
	if (!InitOnceExecuteOnce(&backend_once, PrepareBackend, nullptr, nullptr)) FatalBackend(GetLastError());
	if (backend_status != ERROR_SUCCESS || !real_init) FatalBackend(backend_status);
	StartExtender();
	SetLastError(incoming_error);
	const int result = real_init(version, product);
	const DWORD outgoing_error = GetLastError();
	Log("upc_init_return", static_cast<DWORD>(result));
	SetLastError(outgoing_error);
	return result;
}

