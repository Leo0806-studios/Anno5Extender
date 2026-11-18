#pragma once
#include "Macros.h"
#include <string>
#include <filesystem>
#include <fstream>
#pragma warning(push)
#pragma warning(disable:4820)
namespace A5Ext {

	class Logger {
	public:
		constexpr static inline std::string_view LogDirectory ="Anno5Extender/Logs";
	private:
		std::string pluginName;
		std::filesystem::path logFilePath;
		std::ofstream logFile;
		bool isBuffered = false;
		AnnoDll void flush();
		AnnoDll Logger(const std::string& pluginName);
	public:
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&& other) :
			pluginName(std::move(other.pluginName)),
			logFilePath(std::move(other.logFilePath)),
			logFile(std::move(other.logFile)),
			isBuffered(other.isBuffered)
		{}

		Logger& operator=(Logger&& other) {
			if (this != &other) {
				pluginName = std::move(other.pluginName);
				logFilePath = std::move(other.logFilePath);
				logFile = std::move(other.logFile);
				isBuffered = other.isBuffered;
			}
			return *this;
		}
		AnnoDll static Logger Create(const std::string& PluginName);
		AnnoDll void SetBuffered(bool buffered);
		AnnoDll void Log(const std::string& message);
		AnnoDll static void GlobalLog(const std::string& message);
	};
#pragma warning(pop)
}