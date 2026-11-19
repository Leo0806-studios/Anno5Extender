#pragma once
#pragma warning(push,0)
#include <Windows.h>
#pragma warning(pop)
#include <vector>
#include <cstdint>
#include "Macros.h"
#include <unordered_map>
#include  <string>
#include <array>
namespace A5Ext {
#pragma warning(push)
#pragma warning(disable:4820) // Alignment padding for the plugin initialization flag.
	class Plugin {
		bool(AnnoPluginAbi* init)();
		void(AnnoPluginAbi* shutdown)();
		HMODULE moduleHandle = nullptr;
		bool initialized = false;
		std::array<uint8_t, 256 / 8> sha256{{0}};
	public:
		Plugin(HMODULE moduleHandle, const std::string& pluginName);
		bool Init() {
			if (init) {
				initialized = init();
				return initialized;
			}
			return false;
		}
		void Shutdown() {
			if (initialized && shutdown) {
				shutdown();
				initialized = false;
			}
		}
	};

#pragma warning(pop)
	class PluginRegistry {
		std::unordered_map<std::string, Plugin*> plugins;
	public:
		AnnoDll static PluginRegistry& GetInstance();
		AnnoDll void AddPlugin(const std::string& name, Plugin* plugin);
		AnnoDll void InitAllPlugins();
		AnnoDll void ShutdownAllPlugins();
		AnnoDll void ClearPlugins();
		AnnoDll bool RemovePlugin(const std::string& plugin);
		AnnoDll Plugin* GetPlugin(const std::string& name);

		AnnoDll void LoadAndRegisterPluginsFromDirectory(const std::string& directoryPath);
	};
}