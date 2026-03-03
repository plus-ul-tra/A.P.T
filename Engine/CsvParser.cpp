#include "pch.h"
#include "CsvParser.h"

#include <fstream>

std::vector<std::string> SplitLine(const std::string& line)
{
    std::vector <std::string> values;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i)
    {
        const char ch = line[i];
        if (ch == '\"')
        {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '\"')
            {
                current.push_back('\"');
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (ch == ',' && !inQuotes)
        {
            values.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }

    values.push_back(current);

    return values;
}

std::string Trim(const std::string& value)
{
    const auto start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }

    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

HeaderMap ParseHeader(const std::string& headerLine)
{
    HeaderMap headerIndex;
    const auto headers = SplitLine(headerLine);
    for (size_t i = 0; i < headers.size(); ++i)
    {
        headerIndex[Trim(headers[i])] = i;
    }

    return headerIndex;
}

std::string GetField(const std::vector<std::string>& row, const HeaderMap& headerIndex, const std::string& key)
{
    auto it = headerIndex.find(key);
    if (it == headerIndex.end())
    {
        return "";
    }

    const size_t index = it->second;
    if (index >= row.size())
    {
        return "";
    }

    return Trim(row[index]);
}

int ParseInt(const std::string& value, int fallback)
{
    if (value.empty())
        return fallback;

    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return fallback;
    }
}

float ParseFloat(const std::string& value, float fallback)
{
	if (value.empty())
		return fallback;

	try
	{
		return std::stof(value);
	}
	catch (...)
	{
		return fallback;
	}
}

bool Load(const std::string& path, 
          std::vector<std::vector<std::string>>& outRows,
          HeaderMap& outHeader, 
          std::string* errorOut)
{
    std::ifstream stream(path);
    if (!stream.is_open())
    {
        if (errorOut)
        {
            *errorOut = "Failed to open csv file : " + path;
        }
        return false;
    }

    std::string line;
    bool headerParsed = false;

    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;

        if (!headerParsed)
        {
            outHeader = ParseHeader(line);
            headerParsed = true;
            continue;
        }

        outRows.push_back(SplitLine(line));
    }

    return headerParsed;
}
