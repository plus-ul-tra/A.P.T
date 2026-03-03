#include "pch.h"
#include "LogLocalization.h"
#include "CsvParser.h"

bool ParseBool(const std::string& value)
{
    return value == "1" || value == "true" || value == "TRUE" || value == "True" || value == "Prefix" || value == "prefix";
}

bool LogLocalization::LoadFromCSV(const std::string& path, std::string* errorOut)
{
    m_Languages.clear();
    std::vector<std::vector<std::string>> rows;
    HeaderMap header;

    if (Load(path, rows, header, errorOut))
    {
        return false;
    }

    for (const auto& row : rows)
    {
		LogLanguage language{};
		language.languageId		= ParseInt(GetField(row, header, "languageId"), 1);
		language.paddingPrefix  = ParseBool(GetField(row, header, "paddingPrefix"));
		language.dropPadding	= GetField(row, header, "dropPadding");
		language.missPadding	= GetField(row, header, "missPadding");
		language.attackPadding  = GetField(row, header, "attackPadding");
		language.obtainPadding  = GetField(row, header, "obtainPadding");
		language.throwPadding   = GetField(row, header, "throwPadding");
		language.buyPadding	    = GetField(row, header, "buyPadding");
		language.sellPadding	= GetField(row, header, "sellPadding");
		language.healTemplate   = GetField(row, header, "healTemplate");
		language.damageTemplate = GetField(row, header, "damageTemplate");
		language.moveTemplate	= GetField(row, header, "moveTemplate");
		language.deathTemplate  = GetField(row, header, "deathTemplate");

		m_Languages[language.languageId] = std::move(language);
    }
}

const LogLanguage* LogLocalization::GetLanguage(int languageId) const
{
	auto it = m_Languages.find(languageId);
	if(it == m_Languages.end())
	{
		return nullptr;
	}

	return &it->second;
}

std::string LogLocalization::Format(LogMessageType type, const LogContext& context, int languageId) const
{
	const LogLanguage* language = GetLanguage(languageId);
	if (!language)
		return context.actor + " : " + context.item;

	switch (type)
	{
	case LogMessageType::Drop:
		return context.actor + ": " + BuildWithPadding(language->dropPadding, context.item, language->paddingPrefix);
	case LogMessageType::Miss:
		return context.actor + ": " + BuildWithPadding(language->missPadding, context.target, language->paddingPrefix)
			+ (context.item.empty() ? "" : " " + context.item);
	case LogMessageType::Attack:
		return context.actor + ": " + BuildWithPadding(language->attackPadding, context.target, language->paddingPrefix)
			+ (context.item.empty() ? "" : " " + context.item);
	case LogMessageType::Obtain:
		return context.actor + ": " + BuildWithPadding(language->obtainPadding, context.item, language->paddingPrefix);
	case LogMessageType::Throw:
		return context.actor + ": " + BuildWithPadding(language->throwPadding, context.item, language->paddingPrefix);
	case LogMessageType::Buy:
		return context.actor + ": " + BuildWithPadding(language->buyPadding, context.item, language->paddingPrefix);
	case LogMessageType::Sell:
		return context.actor + ": " + BuildWithPadding(language->sellPadding, context.item, language->paddingPrefix);
	case LogMessageType::Heal:
		return ReplaceTokens(language->healTemplate, context);
	case LogMessageType::Damage:
		return ReplaceTokens(language->damageTemplate, context);
	case LogMessageType::Move:
		return ReplaceTokens(language->moveTemplate, context);
	case LogMessageType::Death:
		return ReplaceTokens(language->deathTemplate, context);
	default:
		return context.actor + ": " + context.item;
	}
}

std::string LogLocalization::BuildWithPadding(const std::string& padding, const std::string& subject, bool prefix) const
{
	if (padding.empty())
	{
		return subject;
	}

	if (prefix)
	{
		return padding + " " + subject;
	}
	return subject + padding;
}

std::string LogLocalization::ReplaceTokens(const std::string& templateText, const LogContext& context) const
{
	std::string result = templateText;
	auto replaceAll = [&result](const std::string& token, const std::string& value)
		{
			size_t pos = 0;
			while ((pos = result.find(token, pos)) != std::string::npos)
			{
				result.replace(pos, token.length(), value);
				pos += value.length();
			}
		};

	replaceAll("{actor}", context.actor);
	replaceAll("{item}", context.item);
	replaceAll("{target}", context.target);
	replaceAll("{value}", std::to_string(context.value));

	return result;
}
