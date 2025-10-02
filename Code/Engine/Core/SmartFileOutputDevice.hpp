//----------------------------------------------------------------------------------------------------
// SmartFileOutputDevice.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Engine/Core/ILogOutputDevice.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "ThirdParty/json/json.hpp"

//----------------------------------------------------------------------------------------------------
// Simple rotation statistics (basic implementation)
//----------------------------------------------------------------------------------------------------
struct sRotationStats
{
	size_t      totalRotations    = 0;
	size_t      totalFilesDeleted = 0;
	std::string lastError;
};

//----------------------------------------------------------------------------------------------------
// Smart log rotation configuration - Minecraft-style log management
//----------------------------------------------------------------------------------------------------
struct sSmartRotationConfig
{
	// File rotation thresholds
	size_t             maxFileSizeBytes = 100 * 1024 * 1024;       // 100MB default
	std::chrono::hours maxTimeInterval{2};              // 2 hours default

	// File management
	String logDirectory        = "Logs";                     // Current session log name
													   String sessionPrefix       = "session";                    // Log directory path
	String currentLogName      = "latest.log";                // Archive prefix
	bool   organizeDateFolders = true;                   // Organize logs in date-based folders

	// Cleanup and retention
	std::chrono::hours retentionHours{720};            // 30 days default (720 hours)
	size_t             maxArchivedFiles      = 200;                     // Maximum archived files
	size_t             maxTotalArchiveSizeMB = 500;                // Maximum total archive size

	// Emergency disk space thresholds (in GB)
	double diskSpaceWarningGB   = 5.0;                   // Warning threshold
	double diskSpaceCriticalGB  = 2.0;                  // Critical threshold
	double diskSpaceEmergencyGB = 0.5;                 // Emergency threshold

	// JSON Parsing
	static sSmartRotationConfig FromJSON(nlohmann::json const& j)
	{
		sSmartRotationConfig config;

		// File rotation thresholds
		if (j.contains("maxFileSizeMB"))
			config.maxFileSizeBytes = j["maxFileSizeMB"].get<size_t>() * 1024 * 1024;
		if (j.contains("maxTimeIntervalHours"))
			config.maxTimeInterval = std::chrono::hours(j["maxTimeIntervalHours"].get<int>());

		// File management
		if (j.contains("logDirectory"))
			config.logDirectory = j["logDirectory"].get<std::string>();
		if (j.contains("currentLogName"))
			config.currentLogName = j["currentLogName"].get<std::string>();
		if (j.contains("sessionPrefix"))
			config.sessionPrefix = j["sessionPrefix"].get<std::string>();
		if (j.contains("organizeDateFolders"))
			config.organizeDateFolders = j["organizeDateFolders"].get<bool>();

		// Cleanup and retention
		if (j.contains("retentionDays"))
			config.retentionHours = std::chrono::hours(j["retentionDays"].get<int>() * 24);
		if (j.contains("maxArchivedFiles"))
			config.maxArchivedFiles = j["maxArchivedFiles"].get<size_t>();
		if (j.contains("maxTotalArchiveSizeMB"))
			config.maxTotalArchiveSizeMB = j["maxTotalArchiveSizeMB"].get<size_t>();

		// Emergency disk space thresholds
		if (j.contains("diskSpaceWarningGB"))
			config.diskSpaceWarningGB = j["diskSpaceWarningGB"].get<double>();
		if (j.contains("diskSpaceCriticalGB"))
			config.diskSpaceCriticalGB = j["diskSpaceCriticalGB"].get<double>();
		if (j.contains("diskSpaceEmergencyGB"))
			config.diskSpaceEmergencyGB = j["diskSpaceEmergencyGB"].get<double>();

		return config;
	}
};

//----------------------------------------------------------------------------------------------------
// Smart file output device with Minecraft-style rotation
//----------------------------------------------------------------------------------------------------
class SmartFileOutputDevice : public ILogOutputDevice
{
private:
	std::ofstream         m_currentFile;
	std::filesystem::path m_logDirectory;
	std::filesystem::path m_currentFilePath;
	sSmartRotationConfig  m_config;

	// Session tracking
	String                                m_sessionId;
	std::chrono::system_clock::time_point m_sessionStartTime;
	std::chrono::system_clock::time_point m_lastRotationTime;
	int                                   m_currentSegmentNumber;

	// File size and statistics
	size_t         m_currentFileSize;
	sRotationStats m_stats;

	// Thread safety
	mutable std::mutex m_fileMutex;
	mutable std::mutex m_rotationMutex;

	// Background processing
	std::thread       m_rotationThread;
	std::atomic<bool> m_shouldStop{false};
	std::atomic<bool> m_rotationPending{false};

public:
	explicit SmartFileOutputDevice(String const& logDirectory, sSmartRotationConfig const& config = sSmartRotationConfig{});
	~SmartFileOutputDevice();

	// ILogOutputDevice interface
	void WriteLog(const LogEntry& entry) override;
	void Flush() override;
	bool IsAvailable() const override;

	// Rotation control
	void ForceRotation();
	bool ShouldRotateBySize() const;
	bool ShouldRotateByTime() const;

	// Statistics and monitoring
	const sRotationStats& GetStats() const { return m_stats; }
	size_t                GetCurrentFileSize() const { return m_currentFileSize; }
	String                GetCurrentSessionId() const { return m_sessionId; }

	// Configuration
	void                        UpdateConfig(const sSmartRotationConfig& config);
	const sSmartRotationConfig& GetConfig() const { return m_config; }

private:
	// Internal rotation logic
	void PerformRotation();
	void RotationThreadMain();

	// File management
	std::filesystem::path GenerateNewLogFilePath();
	String                GenerateSessionId();
	void                  ArchiveCurrentFile();

	// Date-based folder organization helpers
	std::filesystem::path GetDateBasedFolderPath() const;
	String                GetTimeOnlySessionId() const;

	// Cleanup and maintenance
	void                               PerformRetentionCleanup();
	void                               EmergencyCleanup();
	std::vector<std::filesystem::path> ScanForOldLogs() const;

	// Utility functions
	bool   CreateDirectoryIfNeeded(const std::filesystem::path& path);
	void   LogRotationEvent(const String& message);
	double GetAvailableDiskSpaceGB() const;
};
