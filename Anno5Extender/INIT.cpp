#include "INIT.h"
#include "Plugins.h"
#include "Module.h"
#include <filesystem>
#include <fstream>
void A5Ext::Init()
{
	std::filesystem::path configPath = A5Ext::Module::GetInstance().GetFilePath().parent_path() / "Anno5Extender/config/config.txt";
	std::filesystem::path pluginDir = A5Ext::Module::GetInstance().GetFilePath().parent_path() / "Anno5Extender/Plugins";
	std::filesystem::path logDir = A5Ext::Module::GetInstance().GetFilePath().parent_path() / "Anno5Extender/Logs";
	if (!std::filesystem::exists(configPath.parent_path())) {
		std::filesystem::create_directories(configPath.parent_path());
	}
	if (!std::filesystem::exists(configPath)) {
		std::ofstream configFile(configPath);
		if (!configFile.is_open()) {
			throw std::runtime_error("Failed to create config file");
		}
		configFile << "EnableLogging=true" << std::endl;
		configFile.close();
	}
	if (!std::filesystem::exists(pluginDir)) {
		std::filesystem::create_directory(pluginDir);
	}
	if (!std::filesystem::exists(logDir)) {
		std::filesystem::create_directory(logDir);
	}
	PluginRegistry::GetInstance().LoadAndRegisterPluginsFromDirectory(pluginDir.string());
	PluginRegistry::GetInstance().InitAllPlugins();

}

void A5Ext::Shutdown()
{
	PluginRegistry::GetInstance().ShutdownAllPlugins();
	PluginRegistry::GetInstance().ClearPlugins();
}
bool g_isInitialized = false;
extern "C" A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Init() noexcept
{
		if (g_isInitialized) {
			return ERROR_SUCCESS;
		}
	try {
		A5Ext::Init();
		g_isInitialized = true;	
		return ERROR_SUCCESS;
	}
	catch (...) {
		return ERROR_UNHANDLED_EXCEPTION;
	}
}

A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Shutdown() noexcept
{
	try {
		A5Ext::Shutdown();
		return  ERROR_SUCCESS;
	}
	catch (...) {
		return ERROR_UNHANDLED_EXCEPTION;
	}
}

