#pragma once

#include <deque>
#include <string>
#include <cstdint>

#include "LogLocalization.h"

enum class LogChannel
{
	System,
	Combat,
	Loot,
	Shop,
	Movement,
	Interaction
};

struct LogEntry
{
	uint64_t    id      = 0;
	LogChannel  channel = LogChannel::System;
	std::string message;
	float       timeSeconds = 0.0f;
};


class LogSystem
{
public:
	explicit LogSystem(size_t maxEntries = 50);

	void Add(LogChannel channel, const std::string& msg, float timeSeconds = 0.0f);
	void AddLocalized(LogChannel channel, LogMessageType type, const LogContext& context, float timeSeconds = 0.0f);
	void Clear();

	const std::deque<LogEntry>& GetEntries() const { return m_Entries; }
	size_t GetMaxEntries() const { return m_MaxEntries; }

	void SetLocalization(LogLocalization* localization) { m_Localization = localization; }
	void SetLanguageId  (int languageId)				{ m_LanguageId = languageId;	 }

private:
	size_t				 m_MaxEntries = 50;
	uint64_t			 m_NextId     = 1;
	std::deque<LogEntry> m_Entries;
	LogLocalization*	 m_Localization = nullptr;
	int					 m_LanguageId = 1;
};

