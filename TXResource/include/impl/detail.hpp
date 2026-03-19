// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXResource
// File: detail.h

#pragma once
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace std {
namespace fs = filesystem;
}

namespace tx {


// file path ******************************************************************************************************************

inline std::filesystem::path getExePath() {
#ifdef _WIN32
	static std::filesystem::path path = []() {
		wchar_t buffer[MAX_PATH];
		DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		if (length == 0)
			throw std::runtime_error("Failed to get executable path");

		return std::filesystem::path(buffer);
	}();
#elif defined(__linux__)
	static std::filesystem::path path = std::filesystem::read_symlink("/proc/self/exe");
#else
#error Unsupported platform
#endif
	return path;
}
inline std::filesystem::path getExeDir() {
	static std::filesystem::path path = getExePath().parent_path();
	return path;
}

// read file  *****************************************************************************************************************

// raw binary data
using BinaryArray = std::vector<uint8_t>;

BinaryArray readWholeFileBin(const std::filesystem::path& filePath);
std::string readWholeFileText(const std::filesystem::path& filePath);

} // namespace tx