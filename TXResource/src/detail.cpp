// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXResource
// File: resource.cpp

#include "impl/detail.h"
#include <fstream>

namespace tx {

BinaryArray readWholeFileBin(const std::filesystem::path& filePath) {
	size_t fileSize = std::filesystem::file_size(filePath);

	BinaryArray buffer(fileSize);
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs) { throw std::runtime_error("tx::readWholeFileBin(): Failed to open file"); }
	ifs.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	if (ifs.gcount() != fileSize) { throw std::runtime_error("tx::readWholeFileBin(): Failed to read entire file. Buffer size not matching file size."); }
	return buffer;
}
std::string readWholeFileText(const std::filesystem::path& filePath) {
	size_t fileSize = std::filesystem::file_size(filePath);

	std::string buffer(fileSize, '\0');
	std::ifstream ifs(filePath, std::ios::binary);
	if (!ifs) { throw std::runtime_error("tx::readWholeFileText(): Failed to open file"); }
	ifs.read(buffer.data(), fileSize);
	if (ifs.gcount() != fileSize) { throw std::runtime_error("tx::readWholeFileText(): Failed to read entire file. Buffer size not matching file size."); }
	return buffer;
}
} // namespace tx