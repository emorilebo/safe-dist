#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <set>

namespace safedist {

// Configuration for the scanner
struct ScanConfig {
    bool strict_mode = false;
    bool json_output = false;
    std::vector<std::string> debris_patterns = {".DS_Store", ".bak", ".tmp", "Thumbs.db"};
    std::vector<std::string> exclude_patterns = {};
};

struct ScanResult {
    bool passed;
    std::vector<std::string> errors;
    size_t files_scanned = 0;
    size_t directories_scanned = 0;
    
    // Convert result to JSON string
    std::string to_json() const;
};

class Scanner {
public:
    explicit Scanner(const ScanConfig& config);
    
    // Legacy constructor for backward compatibility
    explicit Scanner(bool strict_mode);
    
    ScanResult scan(const std::filesystem::path& path);

private:
    ScanConfig config_;
    
    // Checks if a file has insecure permissions (e.g., world-writable)
    bool has_insecure_permissions(const std::filesystem::path& path);
    
    // Checks if a file is considered "debris" based on patterns
    bool is_debris(const std::filesystem::path& path);
    
    // Checks if a path should be excluded
    bool should_exclude(const std::filesystem::path& path);
    
    // Simple glob pattern matching
    bool matches_pattern(const std::string& filename, const std::string& pattern);
};

} // namespace safedist
