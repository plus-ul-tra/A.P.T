#pragma once

#include <string>
#include <unordered_map>

struct LogLanguage
{
	int languageId = 1;
	bool paddingPrefix = false;
	std::string dropPadding;
	std::string missPadding;
	std::string attackPadding;
	std::string obtainPadding;
	std::string throwPadding;
	std::string buyPadding;
	std::string sellPadding;
	std::string healTemplate;
	std::string damageTemplate;
	std::string moveTemplate;
	std::string deathTemplate;
};

struct LogContext
{
	std::string actor;
	std::string item;
	std::string target;
	int value = 0;
};

enum class LogMessageType
{
	Drop,
	Miss,
	Attack,
	Obtain,
	Throw,
	Buy,
	Sell,
	Heal,
	Damage,
	Move,
	Death
};

class LogLocalization
{
public:
	bool LoadFromCSV(const std::string& path, std::string* errorOut = nullptr);
	const LogLanguage* GetLanguage(int languageId) const;

	std::string Format(LogMessageType type, const LogContext& context, int languageId) const;

private:
	std::string BuildWithPadding(const std::string& padding, const std::string& subject, bool prefix) const;
	std::string ReplaceTokens   (const std::string& templateText, const LogContext& context) const;

	std::unordered_map<int, LogLanguage> m_Languages;
};

