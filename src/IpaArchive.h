#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace IpaArchive {
// Same baseline resource layout as Android ImportActivity; no extracted data.
const std::vector<std::string>& requiredEntries();
// Empty means valid. Failures are recoverable and never call engine Error().
std::string validate(const std::filesystem::path& path);
}
