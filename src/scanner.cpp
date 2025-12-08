#include "scanner.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

namespace safedist {

// JSON output helper
std::string ScanResult::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"passed\": " << (passed ? "true" : "false") << ",\n";
    oss << "  \"files_scanned\": " << files_scanned << ",\n";
    oss << "  \"directories_scanned\": " << directories_scanned << ",\n";
    oss << "  \"error_count\": " << errors.size() << ",\n";
    oss << "  \"errors\": [";
    
    for (size_t i = 0; i < errors.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\n    \"";
        // Escape special characters in JSON
        for (char c : errors[i]) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c;
            }
        }
        oss << "\"";
    }
    
    if (!errors.empty()) oss << "\n  ";
    oss << "]\n";
    oss << "}";
    
    return oss.str();
}

// Config-based constructor
Scanner::Scanner(const ScanConfig& config) : config_(config) {}

// Legacy constructor for backward compatibility
Scanner::Scanner(bool strict_mode) {
    config_.strict_mode = strict_mode;
}

bool Scanner::matches_pattern(const std::string& filename, const std::string& pattern) {
    // Simple pattern matching:
    // - Exact match
    // - Suffix match (e.g., "*.bak" matches "file.bak")
    // - Prefix match (e.g., "temp*" matches "tempfile")
    
    if (pattern.empty()) return false;
    
    // Exact match
    if (pattern == filename) return true;
    
    // Wildcard at start (suffix match): "*.ext"
    if (pattern.front() == '*' && pattern.size() > 1) {
        std::string suffix = pattern.substr(1);
        if (filename.size() >= suffix.size()) {
            return filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }
    
    // Wildcard at end (prefix match): "prefix*"
    if (pattern.back() == '*' && pattern.size() > 1) {
        std::string prefix = pattern.substr(0, pattern.size() - 1);
        if (filename.size() >= prefix.size()) {
            return filename.compare(0, prefix.size(), prefix) == 0;
        }
    }
    
    // Check if pattern appears in filename (for patterns like "Thumbs.db")
    return filename.find(pattern) != std::string::npos;
}

bool Scanner::should_exclude(const std::filesystem::path& path) {
    std::string path_str = path.string();
    std::string filename = path.filename().string();
    
    // Always skip .git directories
    if (path_str.find("/.git/") != std::string::npos || 
        path_str.find("\\.git\\") != std::string::npos ||
        filename == ".git") {
        return true;
    }
    
    // Check user-defined exclusion patterns
    for (const auto& pattern : config_.exclude_patterns) {
        if (matches_pattern(filename, pattern) || matches_pattern(path_str, pattern)) {
            return true;
        }
    }
    
    return false;
}

ScanResult Scanner::scan(const fs::path& target_path) {
    ScanResult result;
    result.passed = true;
    result.files_scanned = 0;
    result.directories_scanned = 0;

    if (!fs::exists(target_path)) {
        result.passed = false;
        result.errors.push_back("Target path does not exist: " + target_path.string());
        return result;
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
            const auto& path = entry.path();
            
            // Track counts
            if (entry.is_directory()) {
                result.directories_scanned++;
            } else {
                result.files_scanned++;
            }
            
            // Check exclusions
            if (should_exclude(path)) {
                continue;
            }

            if (is_debris(path)) {
                result.passed = false;
                result.errors.push_back("Found debris file: " + path.string());
            }

            if (config_.strict_mode && has_insecure_permissions(path)) {
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
    
    // Check against configured debris patterns
    for (const auto& pattern : config_.debris_patterns) {
        if (matches_pattern(filename, pattern)) {
            return true;
        }
    }
    
    return false;
}

bool Scanner::has_insecure_permissions(const fs::path& path) {
    try {
        fs::perms p = fs::status(path).permissions();
        
        // Check for world-writable (Others Write)
        return (p & fs::perms::others_write) != fs::perms::none;
    } catch (const std::exception&) {
        // On Windows, this might not work as expected
        // Return false to avoid false positives
        return false;
    }
}

} // namespace safedist
