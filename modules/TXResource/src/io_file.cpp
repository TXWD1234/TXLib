// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXResource

#include "impl/io_file.hpp"
#include <cstddef>
#include <format>
#include <fstream>
#include <filesystem>

namespace tx {

BinaryArray readWholeFileBin(const std::filesystem::path& filePath) {
	if (!fs::exists(filePath)) throw std::runtime_error(
		std::format("tx::readWholeFileBin(): file path does not exist. File: {}", filePath.string()));
	size_t fileSize = std::filesystem::file_size(filePath);

	BinaryArray buffer(fileSize);
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs) throw std::runtime_error(
		std::format("tx::readWholeFileBin(): Failed to open file. File: {}", filePath.string()));

	ifs.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	if (static_cast<size_t>(ifs.gcount()) != fileSize) throw std::runtime_error(
		std::format("tx::readWholeFileBin(): Failed to read entire file. Buffer size not matching file size. File: {}", filePath.string()));

	return buffer;
}
std::string readWholeFileText(const std::filesystem::path& filePath) {
	if (!fs::exists(filePath)) throw std::runtime_error(
		std::format("tx::readWholeFileText(): file path does not exist. File: {}", filePath.string()));
	size_t fileSize = std::filesystem::file_size(filePath);

	std::string buffer(fileSize, '\0');
	std::ifstream ifs(filePath);
	if (!ifs) throw std::runtime_error(
		std::format("tx::readWholeFileText(): Failed to open file. File: {}", filePath.string()));
	ifs.read(buffer.data(), fileSize);
	if (static_cast<size_t>(ifs.gcount()) != fileSize) throw std::runtime_error(
		std::format("tx::readWholeFileText(): Failed to read entire file. Buffer size not matching file size. File: {}", filePath.string()));

	return buffer;
}
} // namespace tx