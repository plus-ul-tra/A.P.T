#pragma once

#include <string>
#include <unordered_map>
#include <vector>


using HeaderMap = std::unordered_map<std::string, size_t>;

std::vector<std::string> SplitLine(const std::string& line);
std::string Trim       (const std::string& value);
HeaderMap   ParseHeader(const std::string& headerLine);
std::string GetField   (const std::vector<std::string>& row,
						const HeaderMap& headerIndex, 
						const std::string& key);

int   ParseInt  (const std::string& value, int fallback = 0);
float ParseFloat(const std::string& value, float fallback = 0.0f);

bool  Load(const std::string& path,
	       std::vector<std::vector<std::string>>& outRows,
	       HeaderMap& outHeader,
	       std::string* errorOut);


