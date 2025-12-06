#include "scanner.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace safedist {

Scanner::Scanner(bool strict_mode) : strict_mode_(strict_mode) {}

ScanResult Scanner::scan(const fs::path& target_path) {
    ScanResult result;
    result.passed = true;

    if (!fs::exists(target_path)) {
        result.passed = false;
        result.errors.push_back("Target path does not exist: " + target_path.string());
        return result;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
            const auto& path = entry.path();
            
            // Skip .git directories
            if (path.string().find("/.git/") != std::string::npos || path.filename() == ".git") {
                continue;
            }

            if (is_debris(path)) {
                result.passed = false;
                result.errors.push_back("Found debris file: " + path.string());
            }

            if (strict_mode_ && has_insecure_permissions(path)) {
                 result.passed = false;
                 result.errors.push_back("Insecure permissions (world-writable): " + path.string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        result.passed = false;
        result.errors.push_back(std::string("Filesystem error: ") + e.what());
    }

    return result;
}

bool Scanner::is_debris(const fs::path& path) {
    std::string filename = path.filename().string();
    
    // Common debris patterns
    if (filename == ".DS_Store" || 
        filename.rfind(".bak") != std::string::npos ||
        filename.rfind(".tmp") != std::string::npos ||
        filename.rfind("Thumbs.db") != std::string::npos) {
        return true;
    }
    return false;
}

bool Scanner::has_insecure_permissions(const fs::path& path) {
    fs::perms p = fs::status(path).permissions();
    
    // Check for world-writable (Others Write)
    // Note: This is checking the bitmask
    return (p & fs::perms::others_write) != fs::perms::none;
}

} // namespace safedist
