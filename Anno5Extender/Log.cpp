#include "Log.h"
#include "Module.h"
#include <mutex>
namespace A5Ext {
	AnnoDll void Logger::flush()
	{
		logFile.flush();
	}
	Logger::Logger(const std::string& pluginName)
	{
		this->pluginName = pluginName;
		logFilePath = std::filesystem::path(LogDirectory) / (pluginName + ".log");
		logFilePath = Module::GetInstance().GetFilePath().parent_path() / logFilePath;
		if (std::filesystem::exists(logFilePath)) {
			std::filesystem::remove(logFilePath);
		}
		logFile.open(logFilePath, std::ios::out | std::ios::app);
		if (logFile.fail())
		{
			MessageBoxA(nullptr, ("Failed to open log file: " + logFilePath.string()).c_str(), "Error", MB_ICONERROR | MB_OK);
			ExitProcess(EXIT_FAILURE);
		}
		isInitialized = true;
	}
	AnnoDll Logger Logger::Create(const std::string& PluginName)
	{
		return  Logger(PluginName);
	}
	AnnoDll void Logger::SetBuffered(bool buffered)
	{
		this->isBuffered = buffered;
	}
	AnnoDll void Logger::Log(const std::string& message)
	{
		if(!isInitialized)
		{
			return;
		}
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		logFile << "[" << pluginName << "] " << now << " - " << message << std::endl;
		if (!isBuffered) {

			flush();
		}
	}
	AnnoDll void Logger::GlobalLog(const std::string& message)
	{
		static std::filesystem::path logFilePath =Module::GetInstance().GetFilePath().parent_path() / std::filesystem::path(LogDirectory) / ("Global.log");
		static bool Init = [&]() {
			if (std::filesystem::exists(logFilePath)) {
				std::filesystem::remove(logFilePath);
			}
			return true;
			}();
		static std::ofstream logFile(logFilePath, std::ios::out | std::ios::app);
		static std::mutex mutex;
		std::scoped_lock<std::mutex> lock(mutex);
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		logFile << "[Global] " << now << " - " << message << std::endl;
		logFile.flush();


	}
}