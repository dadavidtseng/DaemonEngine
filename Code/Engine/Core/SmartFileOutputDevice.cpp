//----------------------------------------------------------------------------------------------------
// SmartFileOutputDevice.cpp
//----------------------------------------------------------------------------------------------------

#include "EngineCommon.hpp"
#include "Engine/Core/LogSubsystem.hpp"
#include "Engine/Core/SmartFileOutputDevice.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

#include "Engine/Core/StringUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//----------------------------------------------------------------------------------------------------
// SmartFileOutputDevice implementation
//----------------------------------------------------------------------------------------------------

SmartFileOutputDevice::SmartFileOutputDevice(const String& logDirectory, const sSmartRotationConfig& config)
	: m_logDirectory(logDirectory)
	  , m_config(config)
	  , m_currentSegmentNumber(1)
	  , m_currentFileSize(0)
	  , m_sessionStartTime(std::chrono::system_clock::now())
	  , m_lastRotationTime(std::chrono::system_clock::now())
{
	// Generate session ID (timestamp-based, Minecraft-style)
	m_sessionId = GenerateSessionId();

	// Create log directory if needed
	CreateDirectoryIfNeeded(m_logDirectory);

	// Set up current log file (latest.log)
	m_currentFilePath = m_logDirectory / m_config.currentLogName;

	// Rotate existing latest.log if it exists (application startup rotation)
	if (std::filesystem::exists(m_currentFilePath))
	{
		ArchiveCurrentFile();
	}

	// Open new latest.log for current session
	m_currentFile.open(m_currentFilePath, std::ios::out | std::ios::trunc);
	if (!m_currentFile.is_open())
	{
		throw std::runtime_error("Failed to create latest.log file");
	}

	// Start background rotation thread
	m_rotationThread = std::thread(&SmartFileOutputDevice::RotationThreadMain, this);

	LogRotationEvent("SmartFileOutputDevice initialized - Minecraft-style rotation active");
}

SmartFileOutputDevice::~SmartFileOutputDevice()
{
	// Stop background thread
	m_shouldStop = true;
	if (m_rotationThread.joinable())
	{
		m_rotationThread.join();
	}

	// Close current file (but DON'T archive it - Minecraft keeps latest.log after shutdown)
	if (m_currentFile.is_open())
	{
		m_currentFile.close();
	}

	// NOTE: Minecraft-style behavior - latest.log remains after shutdown
	// It will be archived on NEXT startup, not on shutdown
}

//----------------------------------------------------------------------------------------------------
// Core SmartFileOutputDevice methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::WriteLog(const LogEntry& entry)
{
	std::lock_guard<std::mutex> lock(m_fileMutex);

	if (!m_currentFile.is_open())
	{
		return; // Fail silently if file not available
	}

	// Format log entry (similar to regular FileOutputDevice)
	String formattedEntry = Stringf("[%s][%s][%s:%d] %s\n",
									entry.m_timestamp.c_str(),
									entry.m_category.c_str(),
									entry.m_fileName.c_str(),
									entry.m_lineNum,
									entry.m_message.c_str()
	);

	// Write to current file
	m_currentFile << formattedEntry;
	m_currentFile.flush();

	// Track file size
	m_currentFileSize += formattedEntry.length();

	// Check if rotation is needed
	if (ShouldRotateBySize() || ShouldRotateByTime())
	{
		m_rotationPending = true;
	}
}

void SmartFileOutputDevice::Flush()
{
	std::lock_guard<std::mutex> lock(m_fileMutex);
	if (m_currentFile.is_open())
	{
		m_currentFile.flush();
	}
}

bool SmartFileOutputDevice::IsAvailable() const
{
	std::lock_guard<std::mutex> lock(m_fileMutex);
	return m_currentFile.is_open();
}

//----------------------------------------------------------------------------------------------------
// Rotation control methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::ForceRotation()
{
	std::lock_guard<std::mutex> lock(m_rotationMutex);
	PerformRotation();
}

bool SmartFileOutputDevice::ShouldRotateBySize() const
{
	return m_currentFileSize >= m_config.maxFileSizeBytes;
}

bool SmartFileOutputDevice::ShouldRotateByTime() const
{
	auto now                   = std::chrono::system_clock::now();
	auto timeSinceLastRotation = std::chrono::duration_cast<std::chrono::hours>(now - m_lastRotationTime);
	return timeSinceLastRotation >= m_config.maxTimeInterval;
}

void SmartFileOutputDevice::UpdateConfig(const sSmartRotationConfig& config)
{
	std::lock_guard<std::mutex> lock(m_rotationMutex);
	m_config = config;
}

//----------------------------------------------------------------------------------------------------
// Internal rotation logic
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::PerformRotation()
{
	if (m_currentFileSize == 0)
	{
		return; // Nothing to rotate
	}

	// Close current file
	{
		std::lock_guard<std::mutex> fileLock(m_fileMutex);
		if (m_currentFile.is_open())
		{
			m_currentFile.close();
		}
	}

	// Archive the current file
	ArchiveCurrentFile();

	// Increment segment number for next rotation
	m_currentSegmentNumber++;

	// Open new latest.log
	{
		std::lock_guard<std::mutex> fileLock(m_fileMutex);
		m_currentFile.open(m_currentFilePath, std::ios::out | std::ios::trunc);
		m_currentFileSize  = 0;
		m_lastRotationTime = std::chrono::system_clock::now();
	}

	// Update statistics
	m_stats.totalRotations++;

	// Perform cleanup if needed
	PerformRetentionCleanup();

	LogRotationEvent(Stringf("Log rotation completed - segment %d", m_currentSegmentNumber));
}

void SmartFileOutputDevice::RotationThreadMain()
{
	while (!m_shouldStop)
	{
		// Check for pending rotation
		if (m_rotationPending && !m_shouldStop)
		{
			std::lock_guard<std::mutex> lock(m_rotationMutex);

			// Double-check m_shouldStop after acquiring lock (shutdown might have started)
			if (!m_shouldStop)
			{
				PerformRotation();
			}
			m_rotationPending = false;
		}

		// Check disk space periodically
		double availableSpaceGB = GetAvailableDiskSpaceGB();
		if (availableSpaceGB < m_config.diskSpaceEmergencyGB)
		{
			EmergencyCleanup();
		}

		// Sleep for a short period
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

//----------------------------------------------------------------------------------------------------
// File management methods
//----------------------------------------------------------------------------------------------------

std::filesystem::path SmartFileOutputDevice::GenerateNewLogFilePath()
{
	// Date-based folder organization: Logs/YYYY-MM-DD/session-HHMMSS-segXXX.log
	std::filesystem::path dateFolderPath = GetDateBasedFolderPath();

	String filename = Stringf("%s-%s-seg%03d.log",
							  m_config.sessionPrefix.c_str(),
							  GetTimeOnlySessionId().c_str(),
							  m_currentSegmentNumber
	);

	return dateFolderPath / filename;
}

String SmartFileOutputDevice::GenerateSessionId()
{
	auto now    = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
#ifdef _WIN32
	// Use secure version on Windows
	struct tm timeinfo;
	localtime_s(&timeinfo, &time_t);
	ss << std::put_time(&timeinfo, "%Y-%m-%d-%H%M%S");
#else
	ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d-%H%M%S");
#endif
	return ss.str();
}

void SmartFileOutputDevice::ArchiveCurrentFile()
{
	if (!std::filesystem::exists(m_currentFilePath))
	{
		return; // Nothing to archive
	}

	// Generate archive file path with date-based folder organization
	std::filesystem::path archivePath = GenerateNewLogFilePath();

	try
	{
		// Create date-based directory if needed
		CreateDirectoryIfNeeded(archivePath.parent_path());

		// Simply move the file to the date-based folder
		std::filesystem::rename(m_currentFilePath, archivePath);

		LogRotationEvent(Stringf("Archived log file: %s", archivePath.string().c_str()));
	}
	catch (const std::exception& e)
	{
		m_stats.lastError = e.what();
		LogRotationEvent(Stringf("Archive failed: %s", e.what()));
	}
}

std::filesystem::path SmartFileOutputDevice::GetDateBasedFolderPath() const
{
	if (!m_config.organizeDateFolders)
	{
		return m_logDirectory; // Use base directory if date organization is disabled
	}

	// Generate date folder path: Logs/YYYY-MM-DD
	auto now    = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	struct tm         timeinfo;
#ifdef _WIN32
	localtime_s(&timeinfo, &time_t);
#else
	localtime_r(&time_t, &timeinfo);
#endif
	ss << std::put_time(&timeinfo, "%Y-%m-%d");

	return m_logDirectory / ss.str();
}

String SmartFileOutputDevice::GetTimeOnlySessionId() const
{
	// Extract time portion from session ID (remove date prefix)
	// Original format: YYYY-MM-DD-HHMMSS -> HHMMSS
	size_t lastDashPos = m_sessionId.find_last_of('-');
	if (lastDashPos != String::npos && lastDashPos < m_sessionId.length() - 1)
	{
		return m_sessionId.substr(lastDashPos + 1);
	}
	return m_sessionId; // Fallback to full session ID
}

//----------------------------------------------------------------------------------------------------
// Cleanup and maintenance methods
//----------------------------------------------------------------------------------------------------

void SmartFileOutputDevice::PerformRetentionCleanup()
{
	try
	{
		auto oldLogs = ScanForOldLogs();

		// Remove logs older than retention period
		auto now = std::chrono::system_clock::now();
		for (const auto& logPath : oldLogs)
		{
			try
			{
				auto lastWrite = std::filesystem::last_write_time(logPath);
				auto sctp      = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					lastWrite - std::filesystem::file_time_type::clock::now() + now);
				auto fileAge = std::chrono::duration_cast<std::chrono::hours>(now - sctp);

				if (fileAge > m_config.retentionHours)
				{
					std::filesystem::remove(logPath);
					m_stats.totalFilesDeleted++;
					LogRotationEvent(Stringf("Deleted old log: %s", logPath.filename().string().c_str()));
				}
			}
			catch (const std::exception&)
			{
				// Skip files that can't be processed
				continue;
			}
		}

		// Enforce maximum number of archived files
		if (oldLogs.size() > m_config.maxArchivedFiles)
		{
			// Sort by modification time (oldest first)
			std::sort(oldLogs.begin(), oldLogs.end(), [](const auto& a, const auto& b) {
				return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
			});

			// Remove excess files
			size_t filesToDelete = oldLogs.size() - m_config.maxArchivedFiles;
			for (size_t i = 0; i < filesToDelete; ++i)
			{
				std::filesystem::remove(oldLogs[i]);
				m_stats.totalFilesDeleted++;
				LogRotationEvent(Stringf("Deleted excess log: %s", oldLogs[i].filename().string().c_str()));
			}
		}
	}
	catch (const std::exception& e)
	{
		m_stats.lastError = e.what();
		LogRotationEvent(Stringf("Cleanup failed: %s", e.what()));
	}
}

void SmartFileOutputDevice::EmergencyCleanup()
{
	LogRotationEvent("Emergency disk space cleanup initiated");

	try
	{
		auto oldLogs = ScanForOldLogs();

		// Delete up to 50% of old log files in emergency
		size_t maxFiles      = oldLogs.size();
		size_t halfFiles     = maxFiles / 2;
		size_t filesToDelete = (halfFiles < maxFiles) ? halfFiles : maxFiles;

		// Sort by modification time (oldest first)
		std::sort(oldLogs.begin(), oldLogs.end(), [](const auto& a, const auto& b) {
			return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
		});

		for (size_t i = 0; i < filesToDelete; ++i)
		{
			std::filesystem::remove(oldLogs[i]);
			m_stats.totalFilesDeleted++;
		}

		LogRotationEvent(Stringf("Emergency cleanup: deleted %d log files", static_cast<int>(filesToDelete)));
	}
	catch (const std::exception& e)
	{
		m_stats.lastError = e.what();
		LogRotationEvent(Stringf("Emergency cleanup failed: %s", e.what()));
	}
}

std::vector<std::filesystem::path> SmartFileOutputDevice::ScanForOldLogs() const
{
	std::vector<std::filesystem::path> logFiles;

	try
	{
		if (m_config.organizeDateFolders)
		{
			// Scan date-based subfolders
			for (const auto& entry : std::filesystem::directory_iterator(m_logDirectory))
			{
				if (entry.is_directory())
				{
					// Check if it's a date folder (YYYY-MM-DD format)
					String folderName = entry.path().filename().string();
					if (folderName.length() == 10 && folderName[4] == '-' && folderName[7] == '-')
					{
						// Scan files in date folder
						try
						{
							for (const auto& logEntry : std::filesystem::directory_iterator(entry.path()))
							{
								if (logEntry.is_regular_file())
								{
									String filename = logEntry.path().filename().string();
									if (filename.find(m_config.sessionPrefix.c_str()) == 0)
									{
										logFiles.push_back(logEntry.path());
									}
								}
							}
						}
						catch (const std::exception&)
						{
							// Skip folders that can't be read
							continue;
						}
					}
				}
			}
		}
		else
		{
			// Scan base directory (non-date organized mode)
			for (const auto& entry : std::filesystem::directory_iterator(m_logDirectory))
			{
				if (entry.is_regular_file())
				{
					const auto& path     = entry.path();
					String      filename = path.filename().string();

					// Skip current latest.log
					if (filename == m_config.currentLogName.c_str())
					{
						continue;
					}

					// Check if it's a session log file (contains session prefix)
					if (filename.find(m_config.sessionPrefix.c_str()) == 0)
					{
						logFiles.push_back(path);
					}
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		UNUSED(e)
		// Log scan failed, return empty vector
	}

	return logFiles;
}

//----------------------------------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------------------------------

bool SmartFileOutputDevice::CreateDirectoryIfNeeded(const std::filesystem::path& path)
{
	try
	{
		if (!std::filesystem::exists(path))
		{
			std::filesystem::create_directories(path);
			LogRotationEvent(Stringf("Created log directory: %s", path.string().c_str()));
			return true;
		}
		return true;
	}
	catch (const std::exception& e)
	{
		LogRotationEvent(Stringf("Failed to create directory: %s", e.what()));
		return false;
	}
}

void SmartFileOutputDevice::LogRotationEvent(const String& message)
{
	UNUSED(message)
	// Simple logging to console/debug output
	// In a full implementation, this would use the parent LogSubsystem
#ifdef _DEBUG
	OutputDebugStringA(Stringf("[SmartRotation] %s\n", message.c_str()).c_str());
#endif
}

double SmartFileOutputDevice::GetAvailableDiskSpaceGB() const
{
	try
	{
		std::filesystem::space_info const spaceInfo = std::filesystem::space(m_logDirectory);
		return static_cast<double>(spaceInfo.available) / (1024.0 * 1024.0 * 1024.0);
	}
	catch (std::exception const&)
	{
		return 100.0; // Return a safe default if space check fails
	}
}
