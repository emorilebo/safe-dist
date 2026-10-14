#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace safedist {

// Files that should never ship: OS metadata, editor swap files, backups.
std::vector<std::string> default_debris_patterns();

// Files that are dangerous to ship: environment files, private keys, credential stores.
std::vector<std::string> default_sensitive_patterns();

// Configuration for the scanner.
//
// Pattern rules (debris, sensitive and exclude):
//   - A pattern without '/' matches a file or directory *name* at any depth.
//   - A pattern with '/' matches the path relative to the scanned directory, using '/' separators.
//   - '*' matches any run of characters except '/', '**' also matches '/', '?' matches one character.
//   - An excluded directory is skipped entirely, including everything beneath it.
struct ScanConfig {
    bool strict_mode = false;
    bool json_output = false;
    std::vector<std::string> debris_patterns = default_debris_patterns();
    std::vector<std::string> sensitive_patterns = default_sensitive_patterns();
    std::vector<std::string> exclude_patterns = {};
};

enum class FindingKind {
    Debris,         // junk file, e.g. .DS_Store
    Sensitive,      // secret-bearing file, e.g. .env, id_rsa
    WorldWritable,  // strict mode: anyone on the host can modify it
    SymlinkEscape,  // strict mode: symlink resolves outside the scanned directory
    ScanError,      // the scanner could not read something
};

const char* to_string(FindingKind kind);

struct Finding {
    FindingKind kind;
    std::string path;
};

struct ScanResult {
    bool passed = true;
    std::vector<std::string> errors;  // human-readable, one per finding (kept for 0.1.x compatibility)
    std::vector<Finding> findings;
    size_t files_scanned = 0;
    size_t directories_scanned = 0;

    // Convert result to a JSON document.
    std::string to_json() const;
};

class Scanner {
public:
    explicit Scanner(const ScanConfig& config);

    // Legacy constructor kept for 0.1.x compatibility.
    explicit Scanner(bool strict_mode);

    ScanResult scan(const std::filesystem::path& path);

private:
    ScanConfig config_;

    bool is_excluded(const std::string& name, const std::string& relative_path) const;
};

// Glob match where '*' stops at '/', '**' crosses '/', and '**/' also matches zero directories.
bool glob_match(std::string_view pattern, std::string_view text);

// Matches `pattern` against a name or relative path following the ScanConfig pattern rules.
bool matches_entry(const std::string& pattern, const std::string& name, const std::string& relative_path);

// Escapes a string for inclusion inside a JSON string literal.
std::string json_escape(std::string_view text);

}  // namespace safedist
