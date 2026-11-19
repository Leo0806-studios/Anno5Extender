#include "INIT.h"
#include "Plugins.h"
#include "Module.h"
#include <filesystem>
#include <fstream>
#include <atomic>
#include "CBehaviourAutoBuild.h"
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
	AddressLibrary::InitializeCBehaviourAutoBuildBindings();
	PluginRegistry::GetInstance().LoadAndRegisterPluginsFromDirectory(pluginDir.string());
	PluginRegistry::GetInstance().InitAllPlugins();

}

void A5Ext::Shutdown()
{
	PluginRegistry::GetInstance().ShutdownAllPlugins();
	PluginRegistry::GetInstance().ClearPlugins();
}
namespace {
enum class InitializationState { Uninitialized, Initializing, Initialized, Failed, ShuttingDown };
std::atomic<InitializationState> initializationState{InitializationState::Uninitialized};
std::atomic<DWORD> initializationError{ERROR_SUCCESS};
}

extern "C" A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Init() noexcept
{
	auto expected = InitializationState::Uninitialized;
	if (!initializationState.compare_exchange_strong(expected, InitializationState::Initializing)) {
		if (expected == InitializationState::Initialized) return ERROR_SUCCESS;
		if (expected == InitializationState::Failed) return initializationError.load();
		return ERROR_BUSY;
	}
	try {
		A5Ext::Init();
		initializationError.store(ERROR_SUCCESS);
		initializationState.store(InitializationState::Initialized);
		return ERROR_SUCCESS;
	}
	catch (...) {
		initializationError.store(ERROR_UNHANDLED_EXCEPTION);
		initializationState.store(InitializationState::Failed);
		return ERROR_UNHANDLED_EXCEPTION;
	}
}

A5EXT_BOOTSTRAP_API DWORD __cdecl Anno5Extender_Shutdown() noexcept
{
	auto expected = initializationState.load();
	for (;;) {
		if (expected == InitializationState::Uninitialized) return ERROR_SUCCESS;
		if (expected == InitializationState::Initializing || expected == InitializationState::ShuttingDown)
			return ERROR_BUSY;
		if (initializationState.compare_exchange_weak(expected, InitializationState::ShuttingDown)) break;
	}
	try {
		A5Ext::Shutdown();
		initializationError.store(ERROR_SUCCESS);
		initializationState.store(InitializationState::Uninitialized);
		return ERROR_SUCCESS;
	}
	catch (...) {
		initializationError.store(ERROR_UNHANDLED_EXCEPTION);
		initializationState.store(InitializationState::Failed);
		return ERROR_UNHANDLED_EXCEPTION;
	}
}
