#include "scanner.h"

#include <cstdio>
#include <sstream>
#include <system_error>

#ifndef SAFE_DIST_VERSION
#define SAFE_DIST_VERSION "unknown"
#endif

namespace fs = std::filesystem;

namespace safedist {

std::vector<std::string> default_debris_patterns() {
    return {
        ".DS_Store", "._*", "__MACOSX", "Thumbs.db", "desktop.ini",
        "*.bak", "*.tmp", "*.swp", "*.swo", "*~", "*.orig", "*.rej",
    };
}

std::vector<std::string> default_sensitive_patterns() {
    return {
        ".env", ".env.local", ".env.*.local", ".env.development", ".env.production", ".env.staging",
        ".git-credentials", ".netrc", ".pypirc", ".htpasswd", "**/.aws/credentials",
        "id_rsa", "id_dsa", "id_ecdsa", "id_ed25519",
        "*.p12", "*.pfx", "*.jks", "*.keystore",
    };
}

const char* to_string(FindingKind kind) {
    switch (kind) {
        case FindingKind::Debris: return "debris";
        case FindingKind::Sensitive: return "sensitive";
        case FindingKind::WorldWritable: return "world_writable";
        case FindingKind::SymlinkEscape: return "symlink_escape";
        case FindingKind::ScanError: return "scan_error";
    }
    return "unknown";
}

std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buffer[7];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(c));
                    out += buffer;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::string ScanResult::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"tool\": \"safe-dist\",\n";
    oss << "  \"version\": \"" << SAFE_DIST_VERSION << "\",\n";
    oss << "  \"passed\": " << (passed ? "true" : "false") << ",\n";
    oss << "  \"files_scanned\": " << files_scanned << ",\n";
    oss << "  \"directories_scanned\": " << directories_scanned << ",\n";
    oss << "  \"error_count\": " << errors.size() << ",\n";

    oss << "  \"errors\": [";
    for (size_t i = 0; i < errors.size(); ++i) {
        oss << (i == 0 ? "\n" : ",\n") << "    \"" << json_escape(errors[i]) << "\"";
    }
    oss << (errors.empty() ? "],\n" : "\n  ],\n");

    oss << "  \"findings\": [";
    for (size_t i = 0; i < findings.size(); ++i) {
        oss << (i == 0 ? "\n" : ",\n") << "    {\"kind\": \"" << to_string(findings[i].kind)
            << "\", \"path\": \"" << json_escape(findings[i].path) << "\"}";
    }
    oss << (findings.empty() ? "]\n" : "\n  ]\n");
    oss << "}";
    return oss.str();
}

namespace {

class GlobMatcher {
public:
    GlobMatcher(std::string_view pattern, std::string_view text)
        : pattern_(pattern), text_(text), memo_((pattern.size() + 1) * (text.size() + 1), kUnknown) {}

    bool match() { return match_from(0, 0); }

private:
    static constexpr signed char kUnknown = -1;

    std::string_view pattern_;
    std::string_view text_;
    std::vector<signed char> memo_;

    bool match_from(size_t p, size_t t) {
        signed char& cached = memo_[p * (text_.size() + 1) + t];
        if (cached == kUnknown) {
            cached = compute(p, t) ? 1 : 0;
        }
        return cached == 1;
    }

    bool compute(size_t p, size_t t) {
        if (p == pattern_.size()) {
            return t == text_.size();
        }

        const bool double_star = pattern_.compare(p, 2, "**") == 0;
        if (double_star && p + 2 < pattern_.size() && pattern_[p + 2] == '/') {
            // "**/" matches zero directories, or any prefix that ends in '/'.
            if (match_from(p + 3, t)) return true;
            for (size_t k = t; k < text_.size(); ++k) {
                if (text_[k] == '/' && match_from(p + 3, k + 1)) return true;
            }
            return false;
        }
        if (double_star) {
            for (size_t k = t; k <= text_.size(); ++k) {
                if (match_from(p + 2, k)) return true;
            }
            return false;
        }
        if (pattern_[p] == '*') {
            for (size_t k = t; k <= text_.size(); ++k) {
                if (match_from(p + 1, k)) return true;
                if (k < text_.size() && text_[k] == '/') break;
            }
            return false;
        }
        if (t == text_.size()) {
            return false;
        }
        if (pattern_[p] == '?') {
            return text_[t] != '/' && match_from(p + 1, t + 1);
        }
        return pattern_[p] == text_[t] && match_from(p + 1, t + 1);
    }
};

std::string normalize_pattern(std::string pattern) {
    while (pattern.rfind("./", 0) == 0) pattern.erase(0, 2);
    while (!pattern.empty() && pattern.front() == '/') pattern.erase(0, 1);
    while (!pattern.empty() && pattern.back() == '/') pattern.pop_back();
    return pattern;
}

bool matches_any(const std::vector<std::string>& patterns, const std::string& name, const std::string& relative_path) {
    for (const auto& pattern : patterns) {
        if (matches_entry(pattern, name, relative_path)) return true;
    }
    return false;
}

bool is_within(const fs::path& candidate, const fs::path& root) {
    const fs::path relative = candidate.lexically_relative(root);
    if (relative.empty()) return false;
    const auto first = relative.begin();
    return first == relative.end() || *first != "..";
}

}  // namespace

bool glob_match(std::string_view pattern, std::string_view text) {
    return GlobMatcher(pattern, text).match();
}

bool matches_entry(const std::string& pattern, const std::string& name, const std::string& relative_path) {
    const std::string normalized = normalize_pattern(pattern);
    if (normalized.empty()) return false;
    if (normalized.find('/') != std::string::npos) {
        return glob_match(normalized, relative_path);
    }
    return glob_match(normalized, name);
}

Scanner::Scanner(const ScanConfig& config) : config_(config) {}

Scanner::Scanner(bool strict_mode) {
    config_.strict_mode = strict_mode;
}

bool Scanner::is_excluded(const std::string& name, const std::string& relative_path) const {
    // Version-control internals are never part of a distribution check.
    if (name == ".git") return true;
    return matches_any(config_.exclude_patterns, name, relative_path);
}

ScanResult Scanner::scan(const fs::path& target_path) {
    ScanResult result;

    auto report = [&result](FindingKind kind, const std::string& path, const std::string& message) {
        result.passed = false;
        result.findings.push_back({kind, path});
        result.errors.push_back(message);
    };

    std::error_code ec;
    const fs::file_status root_status = fs::status(target_path, ec);
    if (ec || !fs::exists(root_status)) {
        report(FindingKind::ScanError, target_path.string(), "Target path does not exist: " + target_path.string());
        return result;
    }
    if (!fs::is_directory(root_status)) {
        report(FindingKind::ScanError, target_path.string(), "Target path is not a directory: " + target_path.string());
        return result;
    }

    const fs::path canonical_root = fs::weakly_canonical(target_path, ec);

    fs::recursive_directory_iterator it(target_path, ec);
    if (ec) {
        report(FindingKind::ScanError, target_path.string(),
               "Cannot read directory: " + target_path.string() + " (" + ec.message() + ")");
        return result;
    }

    for (const fs::recursive_directory_iterator end; it != end; it.increment(ec)) {
        if (ec) break;

        const fs::directory_entry& entry = *it;
        const fs::path& path = entry.path();
        const std::string name = path.filename().string();
        const std::string relative_path = path.lexically_relative(target_path).generic_string();

        std::error_code status_ec;
        const fs::file_status link_status = entry.symlink_status(status_ec);
        const bool is_symlink = !status_ec && fs::is_symlink(link_status);
        const bool is_directory = !status_ec && fs::is_directory(link_status);

        if (is_excluded(name, relative_path)) {
            if (is_directory) it.disable_recursion_pending();
            continue;
        }

        if (is_directory) {
            result.directories_scanned++;
        } else {
            result.files_scanned++;
        }

        if (matches_any(config_.sensitive_patterns, name, relative_path)) {
            report(FindingKind::Sensitive, path.string(), "Found sensitive file: " + path.string());
            if (is_directory) it.disable_recursion_pending();
            continue;
        }
        if (matches_any(config_.debris_patterns, name, relative_path)) {
            report(FindingKind::Debris, path.string(), "Found debris file: " + path.string());
            if (is_directory) it.disable_recursion_pending();
            continue;
        }

        if (config_.strict_mode && is_symlink && !canonical_root.empty()) {
            std::error_code resolve_ec;
            const fs::path resolved = fs::weakly_canonical(path, resolve_ec);
            if (!resolve_ec && !is_within(resolved, canonical_root)) {
                report(FindingKind::SymlinkEscape, path.string(),
                       "Symlink escapes scanned directory: " + path.string() + " -> " + resolved.string());
            }
        }

#ifndef _WIN32
        // POSIX permission bits are meaningless on Windows, where every writable file reports 0777.
        if (config_.strict_mode && !is_symlink && !status_ec &&
            (link_status.permissions() & fs::perms::others_write) != fs::perms::none) {
            report(FindingKind::WorldWritable, path.string(), "Insecure permissions (world-writable): " + path.string());
        }
#endif

        if (is_directory) {
            std::error_code open_ec;
            fs::directory_iterator probe(path, open_ec);
            if (open_ec) {
                report(FindingKind::ScanError, path.string(),
                       "Cannot read directory: " + path.string() + " (" + open_ec.message() + ")");
                it.disable_recursion_pending();
            }
        }
    }

    if (ec) {
        report(FindingKind::ScanError, target_path.string(),
               "Scan aborted while reading " + target_path.string() + " (" + ec.message() + ")");
    }

    return result;
}

}  // namespace safedist
