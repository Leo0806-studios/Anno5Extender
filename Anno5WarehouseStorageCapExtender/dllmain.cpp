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

	std::ifstream configFile(A5Ext::Module::GetInstance().GetFilePath()/ "Anno5WarehouseStorageCapExtender/config.txt");
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
	auto& module = A5Ext::Module::GetInstance();
	auto baseAddress = module.GetBaseAddress();
	return *(std::bit_cast<uint32_t*>(baseAddress + offset));
}
static	void PatchBytes() {
	auto& module = A5Ext::Module::GetInstance();
	auto baseAddress = module.GetBaseAddress();
	//get size of executalbe in bytes
	size_t bytes = 0;
	MODULEINFO moduleInfo;
	GetModuleInformation(GetCurrentProcess(), module.GetHandle(), &moduleInfo, sizeof(moduleInfo));
	bytes = moduleInfo.SizeOfImage;
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(baseAddress ), bytes, PAGE_EXECUTE_READWRITE, &oldProtect);
	uint32_t newCap = getNewCapFromConfigValue();
	address(baseAddress, 0x0F9943 + 2) = newCap;
	address(baseAddress, 0x162E3A + 2) = newCap;
	address(baseAddress, 0x1E7A48 + 1) = newCap;
	address(baseAddress, 0x1E864F + 1) = newCap;
	address(baseAddress, 0x1E8D0F + 1) = newCap;
	address(baseAddress, 0x2019EF + 1) = newCap;
	address(baseAddress, 0x23EF4B + 2) = newCap;
	address(baseAddress, 0x2DABB2 + 1) = newCap;
	address(baseAddress, 0x3F0A2A + 1) = newCap;
	address(baseAddress, 0x3FE552 + 2) = newCap;
	address(baseAddress, 0x3FE55A + 1) = newCap;
	address(baseAddress, 0x40DD82 + 1) = newCap;
	address(baseAddress, 0x40E0A5 + 1) = newCap;
	address(baseAddress, 0x40E37A + 1) = newCap;
	address(baseAddress, 0x447F1C + 1) = newCap;
	address(baseAddress, 0x448C26 + 1) = newCap;
	address(baseAddress, 0x4C3B61 + 1) = newCap;
	address(baseAddress, 0x4C414E + 1) = newCap;
	address(baseAddress, 0x4D720C + 1) = newCap;
	address(baseAddress, 0x52D916 + 2) = newCap;
	address(baseAddress, 0x52DD88 + 2) = newCap;
	address(baseAddress, 0x52E585 + 1) = newCap;
	address(baseAddress, 0x52E66E + 1) = newCap;
	address(baseAddress, 0x52E893 + 1) = newCap;
	address(baseAddress, 0x52F22D + 1) = newCap;
	address(baseAddress, 0x52F3FC + 1) = newCap;
	address(baseAddress, 0x52F926 + 1) = newCap;
	address(baseAddress, 0x53F8B9 + 1) = newCap;
	address(baseAddress, 0x53FB47 + 1) = newCap;
	address(baseAddress, 0x5E2BD7 + 1) = newCap;
	address(baseAddress, 0x5FEEA5 + 1) = newCap;
	address(baseAddress, 0x621FD4 + 2) = newCap;
	address(baseAddress, 0x622188 + 2) = newCap;
	address(baseAddress, 0x6222FD + 2) = newCap;
	address(baseAddress, 0x622376 + 2) = newCap;
	address(baseAddress, 0x622542 + 2) = newCap;
	address(baseAddress, 0x65DE12 + 1) = newCap;
	address(baseAddress, 0x6B43F4 + 1) = newCap;
	address(baseAddress, 0x736D9E + 1) = newCap;
	address(baseAddress, 0x761939 + 1) = newCap;
	VirtualProtect(reinterpret_cast<void*>(baseAddress), bytes, oldProtect, &oldProtect);


}

extern "C"  bool AnnoPluginAbi Init() {
	logger = std::move(A5Ext::Logger::Create("Anno5WarehouseStorageCapExtender"));
	return true;

}