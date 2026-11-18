#include <format>
#include "Plugins.h"
#include <filesystem>
#include "Module.h"

#include "Log.h"
namespace A5Ext {
	PluginRegistry& A5Ext::PluginRegistry::GetInstance() {
		static PluginRegistry instance;
		return instance;
	}
	AnnoDll void PluginRegistry::AddPlugin(const std::string& name, Plugin* plugin)
	{
		auto[itterator, succsess] = plugins.try_emplace(name, plugin);
		if(!succsess)
		{
			const char* errorMessage = "Plugin with the same name already exists in the registry.";
			auto mainMessage = std::format("Failed to add plugin '{}': {}", name, errorMessage);
			MessageBoxA(nullptr, mainMessage.c_str(), "Plugin Registry Error", MB_ICONERROR | MB_OK);
			std::terminate();
		}

	}
	AnnoDll void PluginRegistry::InitAllPlugins()
	{
		for (auto& [name, plugin] : plugins)
		{
			if (plugin)
			{
				Logger::GlobalLog("Initializing plugin: " + name);
				if (!plugin->Init())
				{
					const char* errorMessage = "Plugin initialization failed.";
					auto mainMessage = std::format("Failed to initialize plugin '{}': {}", name, errorMessage);
					MessageBoxA(nullptr, mainMessage.c_str(), "Plugin Initialization Error", MB_ICONERROR | MB_OK);
					std::terminate();
				}
				Logger::GlobalLog("Successfully initialized plugin: " + name);
			}
		}
	}
	AnnoDll void PluginRegistry::ShutdownAllPlugins()
	{
		for (auto& [name, plugin] : plugins)
		{
			if (plugin)
			{
				plugin->Shutdown();
				delete plugin;
			}
		}
	}
	AnnoDll void PluginRegistry::ClearPlugins()
	{
		for (auto& [name, plugin] : plugins)
		{
			if (plugin)
			{
				delete plugin;
			}
		}
		return plugins.clear();
	}
	AnnoDll bool A5Ext::PluginRegistry::RemovePlugin(const std::string & plugin)
	{
		return plugins.contains(plugin) ? (delete plugins[plugin], plugins.erase(plugin), true) : false;
	}
	AnnoDll Plugin* PluginRegistry::GetPlugin(const std::string& name)
	{
		return plugins.contains(name) ? plugins[name] : nullptr;
	}
	AnnoDll void PluginRegistry::LoadAndRegisterPluginsFromDirectory(const std::string& directoryPath)
	{
		std::filesystem::path dirPath(directoryPath);
		if (!std::filesystem::exists(directoryPath)) {
			if (!std::filesystem::create_directory(directoryPath)) {
				MessageBoxA(nullptr, "Directory creation failed for plugin directory", "fatal internal initialisation Error", MB_ICONERROR | MB_OK);
				terminate();
			}
		}
		std::filesystem::directory_iterator itterator(directoryPath);
		for (auto& entry : itterator) {
			if (entry.is_directory()) {
				std::filesystem::path subDirPath = entry.path();
				std::filesystem::directory_iterator subDirIt(subDirPath);
				for (auto& subEntry : subDirIt) {
					if (subEntry.is_regular_file() && subEntry.path().extension() == ".dll") {
						Logger::GlobalLog("Found plugin DLL: " + subEntry.path().string());
						std::string pluginName = subEntry.path().stem().string();
						HMODULE moduleHandle = LoadLibraryW(subEntry.path().c_str());
						Plugin* plugin = new Plugin(moduleHandle, pluginName);
						AddPlugin(pluginName, plugin);
						Logger::GlobalLog("Loaded plugin: " + pluginName);
					}
				}
			}
		}
	}
	Plugin::Plugin(HMODULE moduleHandle, const std::string& pluginName)
	{
#pragma warning(push)
#pragma warning(disable: 4191) // Disable the warning for reinterpret_cast from function pointer
		auto sha256Opt = Module::GetSha256FromFile(moduleHandle);
		if (!sha256Opt) {
			Logger::GlobalLog("Failed to compute SHA-256 hash for plugin: " + pluginName);
		}
		else {
		sha256 = *sha256Opt;
		}
		init = reinterpret_cast<bool(AnnoPluginAbi*)()>(GetProcAddress(moduleHandle, "Init"));
		shutdown = reinterpret_cast<void(AnnoPluginAbi*)()>(GetProcAddress(moduleHandle, "Shutdown"));
		if (!init || !shutdown)
		{
			Logger::GlobalLog("Failed to find Init or Shutdown function for plugin: " + pluginName);
		}
#pragma warning(pop)
	}
}