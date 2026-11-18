#pragma once
#include <Windows.h>
#include <array>
#include <utility>
#include <bit>
#include <optional>
namespace A5Ext {
	class Module {
		Module();
		Module(const Module&) = delete;
		Module(Module&&) = delete;
		Module& operator=(const Module&) = delete;
		Module& operator=(Module&&) = delete;

		inline static Module* instance = nullptr;
		std::array<uint8_t, 256 / 8> sha256{};
		HMODULE moduleHandle = nullptr;
		bool initialized = false;
		static std::optional<std::array<uint8_t, 32>> GetSha256FromFile(HMODULE moduleHandle);
		static void DisplayErrorMessageBoxAndExit(const char* message, const char* title);

	public:
		static Module& Get();
		HMODULE GetHandle() const noexcept {
			return moduleHandle;
		}
		const std::array<uint8_t, 256 / 8>& GetSha256() const noexcept {
			return sha256;
		}
		uintptr_t GetBaseAddress() const noexcept {
			return std::bit_cast<uintptr_t>(moduleHandle);
		}
		~Module();
	};
}