#include "config.h"
#include "scanner.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#ifndef SAFE_DIST_VERSION
#define SAFE_DIST_VERSION "unknown"
#endif

namespace {

constexpr int kExitPassed = 0;
constexpr int kExitFindings = 1;
constexpr int kExitUsageError = 2;

void print_help() {
    std::cout << "safe-dist " SAFE_DIST_VERSION " - catch debris and secrets in build output before you ship it\n\n"
              << "Usage: safe-dist [OPTIONS] [DIRECTORY]\n\n"
              << "Scans DIRECTORY (default: current directory) for debris files (.DS_Store, *.bak, ...)\n"
              << "and sensitive files (.env, id_rsa, *.p12, ...). Strict mode also flags world-writable\n"
              << "entries and symlinks that point outside DIRECTORY.\n\n"
              << "Options:\n"
              << "  -s, --strict        Also check permissions and symlink targets\n"
              << "  -j, --json          Print a JSON report to stdout\n"
              << "  -e, --exclude PAT   Skip entries matching PAT (repeatable)\n"
              << "  -c, --config FILE   Load configuration from FILE\n"
              << "      --no-config     Do not auto-load ./" << safedist::kDefaultConfigFile << "\n"
              << "  -h, --help          Show this help message\n"
              << "  -v, --version       Show version information\n\n"
              << "Patterns: without '/' they match names at any depth; with '/' they match paths\n"
              << "relative to DIRECTORY. '*' stops at '/', '**' does not. Excluding a directory\n"
              << "skips everything inside it.\n\n"
              << "Examples:\n"
              << "  safe-dist ./dist                       # scan the dist directory\n"
              << "  safe-dist --strict ./dist              # plus permissions and symlinks\n"
              << "  safe-dist --json ./dist > report.json  # machine-readable report\n"
              << "  safe-dist -e node_modules -e '*.log' . # skip noise\n\n"
              << "Exit codes:\n"
              << "  0  Scan passed, no issues found\n"
              << "  1  Issues found (or part of DIRECTORY could not be read)\n"
              << "  2  Usage error, unreadable config file, or DIRECTORY is missing\n\n"
              << "Docs: https://github.com/emorilebo/safe-dist\n";
}

struct CommandLine {
    bool strict = false;
    bool json = false;
    bool no_config = false;
    std::vector<std::string> excludes;
    std::optional<std::string> config_file;
    std::string target_dir = ".";
};

enum class ParseOutcome { Run, ExitOk, ExitUsageError };

ParseOutcome parse_command_line(int argc, char* argv[], CommandLine& cli) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    bool target_seen = false;

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        const bool has_value = i + 1 < args.size();

        if (arg == "-h" || arg == "--help") {
            print_help();
            return ParseOutcome::ExitOk;
        }
        if (arg == "-v" || arg == "--version") {
            std::cout << "safe-dist " SAFE_DIST_VERSION "\n";
            return ParseOutcome::ExitOk;
        }
        if (arg == "-s" || arg == "--strict") {
            cli.strict = true;
        } else if (arg == "-j" || arg == "--json") {
            cli.json = true;
        } else if (arg == "--no-config") {
            cli.no_config = true;
        } else if (arg == "-e" || arg == "--exclude" || arg == "-c" || arg == "--config") {
            if (!has_value) {
                std::cerr << "safe-dist: option '" << arg << "' requires a value\n";
                return ParseOutcome::ExitUsageError;
            }
            if (arg == "-e" || arg == "--exclude") {
                cli.excludes.push_back(args[++i]);
            } else {
                cli.config_file = args[++i];
            }
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "safe-dist: unknown option '" << arg << "' (see --help)\n";
            return ParseOutcome::ExitUsageError;
        } else if (target_seen) {
            std::cerr << "safe-dist: only one DIRECTORY can be scanned per run (got '" << cli.target_dir
                      << "' and '" << arg << "')\n";
            return ParseOutcome::ExitUsageError;
        } else {
            cli.target_dir = arg;
            target_seen = true;
        }
    }
    return ParseOutcome::Run;
}

// Returns the effective configuration, or std::nullopt if an explicitly requested config file is unreadable.
std::optional<safedist::ScanConfig> load_config(const CommandLine& cli) {
    safedist::ScanConfig config;

    std::optional<std::string> config_path = cli.config_file;
    if (!config_path && !cli.no_config) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(safedist::kDefaultConfigFile, ec)) {
            config_path = safedist::kDefaultConfigFile;
        }
    }

    if (config_path) {
        std::ifstream file(*config_path);
        if (!file) {
            std::cerr << "safe-dist: cannot read config file '" << *config_path << "'\n";
            return std::nullopt;
        }
        config = safedist::parse_config(file);
        if (!cli.json) std::cout << "Using config: " << *config_path << "\n";
    }

    config.strict_mode = config.strict_mode || cli.strict;
    config.json_output = cli.json;
    config.exclude_patterns.insert(config.exclude_patterns.end(), cli.excludes.begin(), cli.excludes.end());
    return config;
}

}  // namespace

int main(int argc, char* argv[]) {
    CommandLine cli;
    switch (parse_command_line(argc, argv, cli)) {
        case ParseOutcome::ExitOk: return kExitPassed;
        case ParseOutcome::ExitUsageError: return kExitUsageError;
        case ParseOutcome::Run: break;
    }

    const std::optional<safedist::ScanConfig> config = load_config(cli);
    if (!config) return kExitUsageError;

    std::error_code ec;
    if (!std::filesystem::is_directory(cli.target_dir, ec)) {
        std::cerr << "safe-dist: '" << cli.target_dir << "' is not a directory\n";
        return kExitUsageError;
    }

    if (!config->json_output) {
        std::cout << "Scanning directory: " << cli.target_dir << (config->strict_mode ? " (strict mode)" : "")
                  << "...\n";
    }

    safedist::Scanner scanner(*config);
    const safedist::ScanResult result = scanner.scan(cli.target_dir);

    if (config->json_output) {
        std::cout << result.to_json() << "\n";
    } else if (result.passed) {
        std::cout << "PASS: no issues found.\n"
                  << "  Files scanned: " << result.files_scanned << "\n"
                  << "  Directories scanned: " << result.directories_scanned << "\n";
    } else {
        std::cerr << "FAIL: " << result.errors.size() << " issue(s) found:\n";
        for (const auto& error : result.errors) {
            std::cerr << "  [FAIL] " << error << "\n";
        }
    }

    return result.passed ? kExitPassed : kExitFindings;
}
