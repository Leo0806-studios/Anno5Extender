// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <Macros.h>
#include <Module.h>
#include <psapi.h>
#include <bit>
#include <fstream>
#include <Log.h>
A5Ext::Logger logger;
BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;

	}
	return TRUE;
}
uint32_t getNewCapFromConfigValue() {
	//get the new cap value from the config file

	std::ifstream configFile(A5Ext::Module::GetInstance().GetFilePath().parent_path()/ "Anno5WarehouseStorageCapExtender/config.txt");
	if (!configFile.is_open()) {
		logger.Log("Failed to open config file.setting default value of 999999 \n");
		return 999999;
	}
	std::string line;
	while (std::getline(configFile, line)) {
		if (line.find("newCap") != std::string::npos) {
			uint32_t newCap = std::stoul(line.substr(line.find("=") + 1));
			return newCap;
		}
	}
	logger.Log("Failed to find newCap in config file. setting default value of 999999 \n");
	return 999999;
}
uint32_t& address(uintptr_t base, uint32_t offset) {

	return *(std::bit_cast<uint32_t*>(base + offset));
}
static	void PatchBytes() {
	auto& module = A5Ext::Module::GetInstance();
	auto baseAddress = module.GetBaseAddress();
	MODULEINFO moduleInfo{};

	if (baseAddress == 0 || module.GetHandle() == nullptr ||
		!GetModuleInformation(GetCurrentProcess(), module.GetHandle(), &moduleInfo, sizeof(moduleInfo)) ||
		moduleInfo.SizeOfImage == 0) {
		logger.Log("Failed to obtain the host module information; patches were not applied.\n");
		return;
	}

	struct Patch {
		uint32_t offset;
		uint32_t divisor;
	};

	const Patch patches[] = {
		{0x0F9943 + 2, 1}, {0x162E3A + 2, 1}, {0x1E7A48 + 1, 1},
		{0x1E864F + 1, 1}, {0x1E8D0F + 1, 1}, {0x2019EF + 1, 1},
		{0x23EF4B + 2, 1}, {0x2DABB2 + 1, 1}, {0x3F0A2A + 1, 1},
		{0x3FE552 + 2, 1}, {0x3FE55A + 1, 1000}, {0x40DD82 + 1, 1},
		{0x40E0A5 + 1, 1}, {0x40E37A + 1, 1}, {0x447F1C + 1, 1},
		{0x448C26 + 1, 1}, {0x4C3B61 + 1, 1}, {0x4C414E + 1, 1},
		{0x4D720C + 1, 1}, {0x52D916 + 2, 1}, {0x52DD88 + 2, 1},
		{0x52E585 + 1, 1}, {0x52E66E + 1, 1}, {0x52E893 + 1, 1},
		{0x52F22D + 1, 1}, {0x52F3FC + 1, 1}, {0x52F926 + 1, 1},
		{0x53F8B9 + 1, 1}, {0x53FB47 + 1, 1}, {0x5E2BD7 + 1, 1},
		{0x5FEEA5 + 1, 1}, {0x621FD4 + 2, 1}, {0x622188 + 2, 1},
		{0x6222FD + 2, 1}, {0x622376 + 2, 1}, {0x622542 + 2, 1},
		{0x65DE12 + 1, 1}, {0x6B43F4 + 1, 1}, {0x736D9E + 1, 1},
		{0x761939 + 1, 1}
	};

	SYSTEM_INFO systemInfo{};
	GetSystemInfo(&systemInfo);
	const uintptr_t pageSize = static_cast<uintptr_t>(systemInfo.dwPageSize);
	if (pageSize == 0 || (pageSize & (pageSize - 1)) != 0) {
		logger.Log("Failed to obtain a valid memory page size; patches were not applied.\n");
		return;
	}

	uintptr_t protectedPageAddresses[sizeof(patches) / sizeof(patches[0]) * 2]{};
	DWORD protectedPageProtections[sizeof(patches) / sizeof(patches[0]) * 2]{};
	size_t protectedPageCount = 0;
	const uintptr_t pageMask = ~(pageSize - 1);
	const uintptr_t imageEnd = baseAddress + static_cast<uintptr_t>(moduleInfo.SizeOfImage);

	for (const Patch& patch : patches) {
		const uintptr_t target = baseAddress + static_cast<uintptr_t>(patch.offset);
		if (target < baseAddress || target > imageEnd || imageEnd - target < sizeof(uint32_t)) {
			logger.Log("A patch address is outside the host module; patches were not applied.\n");
			return;
		}

		const uintptr_t firstPage = target & pageMask;
		const uintptr_t lastPage = (target + sizeof(uint32_t) - 1) & pageMask;
		for (uintptr_t page = firstPage;; page += pageSize) {
			bool alreadyTracked = false;
			for (size_t i = 0; i < protectedPageCount; ++i) {
				if (protectedPageAddresses[i] == page) {
					alreadyTracked = true;
					break;
				}
			}

			if (!alreadyTracked) {
				DWORD oldProtection = 0;
				if (!VirtualProtect(reinterpret_cast<void*>(page), pageSize,
					PAGE_EXECUTE_READWRITE, &oldProtection)) {
					for (size_t i = 0; i < protectedPageCount; ++i) {
						DWORD ignoredProtection = 0;
						VirtualProtect(reinterpret_cast<void*>(protectedPageAddresses[i]),
							pageSize, protectedPageProtections[i], &ignoredProtection);
					}
					logger.Log("Failed to change memory protection; patches were not applied.\n");
					return;
				}
				protectedPageAddresses[protectedPageCount] = page;
				protectedPageProtections[protectedPageCount] = oldProtection;
				++protectedPageCount;
			}

			if (page == lastPage) {
				break;
			}
		}
	}

	const uint32_t newCap = getNewCapFromConfigValue();
	for (const Patch& patch : patches) {
		const uint32_t value = newCap / patch.divisor;
		const uintptr_t target = baseAddress + static_cast<uintptr_t>(patch.offset);
		CopyMemory(reinterpret_cast<void*>(target), &value, sizeof(value));
	}

	FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(baseAddress), moduleInfo.SizeOfImage);

	for (size_t i = 0; i < protectedPageCount; ++i) {
		DWORD ignoredProtection = 0;
		if (!VirtualProtect(reinterpret_cast<void*>(protectedPageAddresses[i]), pageSize,
			protectedPageProtections[i], &ignoredProtection)) {
			logger.Log("Failed to restore memory protection after patching.\n");
		}
	}
}

extern "C"  __declspec(dllexport) bool AnnoPluginAbi Init() {
	logger = std::move(A5Ext::Logger::Create("Anno5WarehouseStorageCapExtender"));
	PatchBytes();
	return true;

}
extern "C" __declspec(dllexport) void AnnoPluginAbi Shutdown() {
	logger.Log("shutting down Anno5WarehouseStorageCapExtender\n");
}