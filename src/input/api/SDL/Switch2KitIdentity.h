#pragma once

#include <algorithm>
#include <string_view>

namespace CemuSwitch2Kit
{
inline bool ValidPhysicalKey(std::string_view key)
{
	return key.size() == 36 && key.starts_with("s2k:") &&
		std::all_of(key.begin() + 4, key.end(), [](char c) {
			return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
		});
}
} // namespace CemuSwitch2Kit
