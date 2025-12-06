#include "scanner.h"
#include <iostream>
#include <string>
#include <vector>

void print_help() {
    std::cout << "Usage: safe-dist [OPTIONS] <DIRECTORY>\n"
              << "Scans a directory for known bad file patterns and insecure permissions.\n\n"
              << "Options:\n"
              << "  -s, --strict    Fail on insecure permissions (e.g. world-writable)\n"
              << "  -h, --help      Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    bool strict = false;
    std::string target_dir;

    for (const auto& arg : args) {
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "-s" || arg == "--strict") {
            strict = true;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            print_help();
            return 1;
        } else {
            target_dir = arg;
        }
    }

    if (target_dir.empty()) {
        target_dir = "."; // Default to current directory
    }

    std::cout << "Scanning directory: " << target_dir << (strict ? " (strict mode)" : "") << "...\n";

    safedist::Scanner scanner(strict);
    auto result = scanner.scan(target_dir);

    if (result.passed) {
        std::cout << "Scan passed! No issues found.\n";
        return 0;
    } else {
        std::cerr << "Scan failed with " << result.errors.size() << " errors:\n";
        for (const auto& error : result.errors) {
            std::cerr << "  [FAIL] " << error << "\n";
        }
        return 1;
    }
}
