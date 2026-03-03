#pragma once

static uint32_t AddString(std::string& table, const std::string& s)
{
	if (s.empty()) return 0;

	const uint32_t offset = static_cast<uint32_t>(table.size());
	table.append(s);
	table.push_back('\0');
	return offset;
}