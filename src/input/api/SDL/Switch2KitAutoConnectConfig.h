#pragma once

#include <boost/property_tree/ini_parser.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace CemuSwitch2Kit
{
// This application-owned file is separate from controllerProfiles. Existing INI
// entries/sections survive updates; malformed or unreadable files are not reset.
class AutoConnectConfig
{
public:
	static bool Load(const std::filesystem::path& path, bool& enabled)
	{
		boost::property_tree::ptree data;
		bool value = false;
		if (!Read(path, data, value))
			return false;
		enabled = value;
		return true;
	}

	// The caller supplies Cemu's atomic file writer. Read the current document
	// again before a save, rather than clobbering it with a startup-time snapshot.
	template <typename WriteAtomic>
	static bool Save(const std::filesystem::path& path, bool enabled, WriteAtomic&& writeAtomic)
	{
		boost::property_tree::ptree data;
		bool previous = false;
		if (!Read(path, data, previous))
			return false;
		try
		{
			data.put("Settings.AutoConnect", enabled ? "true" : "false");
			std::ostringstream stream;
			boost::property_tree::write_ini(stream, data);
			// A successful save must remain readable on the next launch. Adding
			// the key or changing true to false can grow a valid near-limit INI.
			const auto bytes = stream.str();
			if (bytes.size() > MaxBytes)
				return false;
			return writeAtomic(path, bytes);
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

private:
	static constexpr std::size_t MaxBytes = 65536;

	static bool Read(const std::filesystem::path& path, boost::property_tree::ptree& data, bool& enabled)
	{
		try
		{
			std::error_code error;
			const auto status = std::filesystem::symlink_status(path, error);
			if (status.type() == std::filesystem::file_type::not_found &&
				(!error || error == std::errc::no_such_file_or_directory))
			{
				enabled = false;
				return true;
			}
			// Do not replace symlinks, directories, special files, or files whose
			// status is inaccessible. Bound this small preference file's input.
			if (error || !std::filesystem::is_regular_file(status))
				return false;
			const auto size = std::filesystem::file_size(path, error);
			if (error || size > MaxBytes)
				return false;
			std::ifstream file(path, std::ios::binary);
			if (!file)
				return false;
			std::string bytes(MaxBytes + 1, '\0');
			file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
			if (file.bad() || !file.eof() || file.gcount() > static_cast<std::streamsize>(MaxBytes))
				return false;
			bytes.resize(static_cast<std::size_t>(file.gcount()));
			if (bytes.find('\0') != std::string::npos)
				return false;
			std::istringstream input(bytes);
			boost::property_tree::read_ini(input, data);
			const auto value = data.get_optional<std::string>("Settings.AutoConnect");
			if (!value || *value == "false" || *value == "0")
				enabled = false;
			else if (*value == "true" || *value == "1")
				enabled = true;
			else
				return false;
			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}
};
}
