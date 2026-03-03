#include "pch.h"
#include "LogSystem.h"

LogSystem::LogSystem(size_t maxEntries) : m_MaxEntries(maxEntries)
{
}

void LogSystem::Add(LogChannel channel, const std::string& msg, float timeSeconds)
{
	if (m_Entries.size() >= m_MaxEntries)
		m_Entries.pop_front();

	LogEntry entry;
	entry.channel	  = channel;
	entry.id		  = m_NextId++;
	entry.message     = msg;
	entry.timeSeconds = timeSeconds;
	m_Entries.push_back(entry);
}

void LogSystem::AddLocalized(LogChannel channel, LogMessageType type, const LogContext& context, float timeSeconds)
{
	std::string message;
	if (m_Localization)
	{
		message = m_Localization->Format(type, context, m_LanguageId);
	}
	else
	{
		message = context.actor + ": " + context.item;
	}

	Add(channel, message, timeSeconds);
}

void LogSystem::Clear()
{
	m_Entries.clear();
}