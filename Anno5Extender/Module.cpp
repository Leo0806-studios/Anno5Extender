#pragma warning(push,0)
#include <Windows.h>
#include <bcrypt.h>
#pragma warning(pop)
#include "Module.h"
#include <vector>
#include <optional>
#include <filesystem>

#pragma comment(lib, "bcrypt.lib")

namespace A5Ext {
	Module::Module() {
		moduleHandle = GetModuleHandleW(nullptr);
		if (auto sha256Opt = GetSha256FromFile(moduleHandle); sha256Opt.has_value()) {
			sha256 = *sha256Opt;
		}
		else {
			DisplayErrorMessageBoxAndExit("Failed to compute SHA-256 hash of the module.", "Error");
		}
		std::vector<wchar_t> localFilePath(MAX_PATH);

		// Resolve full module path.
		for (;;)
		{
			DWORD length = GetModuleFileNameW(
				moduleHandle,
				localFilePath.data(),
				static_cast<DWORD>(localFilePath.size())
			);

			if (length == 0)
				DisplayErrorMessageBoxAndExit("Failed to get module file name.", "Error");

			if (length < localFilePath.size())
				break;

			if (localFilePath.size() >= 32768)
				DisplayErrorMessageBoxAndExit("Module file path is too long.", "Error");

			localFilePath.resize(localFilePath.size() * 2);
		}

		this->filePath = std::filesystem::path(localFilePath.data());
		initialized = true;
	}
	std::optional<std::array<uint8_t, 32>> Module::GetSha256FromFile(HMODULE moduleHandle)
	{
		std::vector<wchar_t> filePath(MAX_PATH);

		// Resolve full module path.
		for (;;)
		{
			DWORD length = GetModuleFileNameW(
				moduleHandle,
				filePath.data(),
				static_cast<DWORD>(filePath.size())
			);

			if (length == 0)
				return std::nullopt;

			if (length < filePath.size())
				break;

			if (filePath.size() >= 32768)
				return std::nullopt;

			filePath.resize(filePath.size() * 2);
		}

		HANDLE fileHandle = CreateFileW(
			filePath.data(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);

		if (fileHandle == INVALID_HANDLE_VALUE)
			return std::nullopt;

		BCRYPT_ALG_HANDLE hAlg = nullptr;
		BCRYPT_HASH_HANDLE hHash = nullptr;

		std::optional<std::array<uint8_t, 32>> result;

		do
		{
			NTSTATUS status = BCryptOpenAlgorithmProvider(
				&hAlg,
				BCRYPT_SHA256_ALGORITHM,
				nullptr,
				0
			);

			if (!BCRYPT_SUCCESS(status))
				break;

			status = BCryptCreateHash(
				hAlg,
				&hHash,
				nullptr,
				0,
				nullptr,
				0,
				0
			);

			if (!BCRYPT_SUCCESS(status))
				break;

			std::array<uint8_t, 64 * 1024> buffer{};

			bool success = true;

			for (;;)
			{
				DWORD bytesRead = 0;

				if (!ReadFile(
					fileHandle,
					buffer.data(),
					static_cast<DWORD>(buffer.size()),
					&bytesRead,
					nullptr))
				{
					success = false;
					break;
				}

				if (bytesRead == 0)
					break;

				status = BCryptHashData(
					hHash,
					buffer.data(),
					bytesRead,
					0
				);

				if (!BCRYPT_SUCCESS(status))
				{
					success = false;
					break;
				}
			}

			if (!success)
				break;

			std::array<uint8_t, 32> sha256{};

			status = BCryptFinishHash(
				hHash,
				sha256.data(),
				static_cast<ULONG>(sha256.size()),
				0
			);

			if (!BCRYPT_SUCCESS(status))
				break;

			result = sha256;

		} while (false);

		// Cleanup always happens.
		if (hHash)
			BCryptDestroyHash(hHash);

		if (hAlg)
			BCryptCloseAlgorithmProvider(hAlg, 0);

		CloseHandle(fileHandle);

		return result;
	}
	void Module::DisplayErrorMessageBoxAndExit(const char* message, const char* title)
	{
		MessageBoxA(nullptr, message, title, MB_ICONERROR | MB_OK);
		ExitProcess(EXIT_FAILURE);
	
	
	}
	A5Ext::Module& A5Ext::Module::GetInstance()
	{
		static Module instance;
		return instance;
	}
	std::filesystem::path Module::GetFilePath() const
	{
		return filePath;
	}
	Module::~Module()
	{}
}