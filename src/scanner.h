#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace safedist {

struct ScanResult {
    bool passed;
    std::vector<std::string> errors;
};

class Scanner {
public:
    Scanner(bool strict_mode);
    ScanResult scan(const std::filesystem::path& path);

private:
    bool strict_mode_;
    
    // Checks if a file has insecure permissions (e.g., world-writable)
    bool has_insecure_permissions(const std::filesystem::path& path);
    // Checks if a file is considered "debris" (e.g., .DS_Store, .bak)
    bool is_debris(const std::filesystem::path& path);
};

} // namespace safedist
