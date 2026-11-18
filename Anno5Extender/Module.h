#pragma once
#pragma warning(push,0)
#include <Windows.h>
#pragma warning(pop)
#include <array>
#include <utility>
#include <bit>
#include <optional>
#include <string>
#include <filesystem>
namespace A5Ext {
#pragma warning(push)
#pragma warning(disable:4820)
	class Module {
		Module();
		Module(const Module&) = delete;
		Module(Module&&) = delete;
		Module& operator=(const Module&) = delete;
		Module& operator=(Module&&) = delete;

		std::array<uint8_t, 256 / 8> sha256{};
		HMODULE moduleHandle = nullptr;
		std::filesystem::path filePath;
		bool initialized = false;
		static void DisplayErrorMessageBoxAndExit(const char* message, const char* title);

	public:
		static std::optional<std::array<uint8_t, 32>> GetSha256FromFile(HMODULE moduleHandle);
		static Module& GetInstance();
		HMODULE GetHandle() const noexcept {
			return moduleHandle;
		}
		const std::array<uint8_t, 256 / 8>& GetSha256() const noexcept {
			return sha256;
		}
		uintptr_t GetBaseAddress() const noexcept {
			return std::bit_cast<uintptr_t>(moduleHandle);
		}
		std::filesystem::path GetFilePath() const;
		~Module();
	};
#pragma warning(pop)
}