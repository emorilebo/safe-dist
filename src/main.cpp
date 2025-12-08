#include "scanner.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

void print_help() {
    std::cout << "safe-dist v0.1.0 - Directory Integrity Checker for CI/CD\n\n"
              << "Usage: safe-dist [OPTIONS] [DIRECTORY]\n\n"
              << "Scans a directory for debris files (e.g., .DS_Store, *.bak) and\n"
              << "optionally checks for insecure permissions.\n\n"
              << "Arguments:\n"
              << "  DIRECTORY           Path to scan (default: current directory)\n\n"
              << "Options:\n"
              << "  -s, --strict        Fail on insecure permissions (world-writable)\n"
              << "  -j, --json          Output results as JSON\n"
              << "  -e, --exclude PAT   Exclude files matching pattern (can repeat)\n"
              << "  -c, --config FILE   Load configuration from YAML file\n"
              << "  -h, --help          Show this help message\n"
              << "  -v, --version       Show version information\n\n"
              << "Examples:\n"
              << "  safe-dist ./dist               # Scan dist directory\n"
              << "  safe-dist --strict ./dist      # Strict mode (check permissions)\n"
              << "  safe-dist --json ./dist        # JSON output for CI integration\n"
              << "  safe-dist --exclude '*.log' .  # Exclude log files\n\n"
              << "Exit Codes:\n"
              << "  0  Scan passed, no issues found\n"
              << "  1  Scan failed, issues detected\n\n"
              << "Author: Godfrey Lebo <emorylebo@gmail.com>\n"
              << "Repository: https://github.com/emorilebo/safe-dist\n";
}

void print_version() {
    std::cout << "safe-dist 0.1.0\n";
}

// Simple YAML-like config parser (key: value format)
safedist::ScanConfig parse_config_file(const std::string& filepath) {
    safedist::ScanConfig config;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config file: " << filepath << "\n";
        return config;
    }
    
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Trim leading whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        
        line = line.substr(start);
        
        // Check for section headers
        if (line.find("debris_patterns:") == 0) {
            current_section = "debris";
            config.debris_patterns.clear(); // Reset default patterns
            continue;
        }
        if (line.find("exclude_patterns:") == 0) {
            current_section = "exclude";
            continue;
        }
        if (line.find("strict:") == 0) {
            std::string value = line.substr(7);
            size_t vs = value.find_first_not_of(" \t");
            if (vs != std::string::npos) {
                value = value.substr(vs);
                config.strict_mode = (value == "true" || value == "yes" || value == "1");
            }
            continue;
        }
        
        // List items (start with -)
        if (line[0] == '-' && line.size() > 2) {
            std::string item = line.substr(2);
            // Remove quotes if present
            if ((item.front() == '"' && item.back() == '"') ||
                (item.front() == '\'' && item.back() == '\'')) {
                item = item.substr(1, item.size() - 2);
            }
            
            if (current_section == "debris") {
                config.debris_patterns.push_back(item);
            } else if (current_section == "exclude") {
                config.exclude_patterns.push_back(item);
            }
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    safedist::ScanConfig config;
    std::string target_dir;
    std::string config_file;

    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];
        
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        } else if (arg == "-s" || arg == "--strict") {
            config.strict_mode = true;
        } else if (arg == "-j" || arg == "--json") {
            config.json_output = true;
        } else if ((arg == "-e" || arg == "--exclude") && i + 1 < args.size()) {
            config.exclude_patterns.push_back(args[++i]);
        } else if ((arg == "-c" || arg == "--config") && i + 1 < args.size()) {
            config_file = args[++i];
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            print_help();
            return 1;
        } else {
            target_dir = arg;
        }
    }

    // Load config file if specified
    if (!config_file.empty()) {
        safedist::ScanConfig file_config = parse_config_file(config_file);
        // Merge: command-line options take precedence
        if (!config.strict_mode) config.strict_mode = file_config.strict_mode;
        if (config.debris_patterns == safedist::ScanConfig().debris_patterns) {
            config.debris_patterns = file_config.debris_patterns;
        }
        for (const auto& pattern : file_config.exclude_patterns) {
            config.exclude_patterns.push_back(pattern);
        }
    }

    if (target_dir.empty()) {
        target_dir = "."; // Default to current directory
    }

    // Run the scan
    if (!config.json_output) {
        std::cout << "Scanning directory: " << target_dir;
        if (config.strict_mode) std::cout << " (strict mode)";
        std::cout << "...\n";
    }

    safedist::Scanner scanner(config);
    auto result = scanner.scan(target_dir);

    // Output results
    if (config.json_output) {
        std::cout << result.to_json() << "\n";
    } else {
        if (result.passed) {
            std::cout << "✓ Scan passed! No issues found.\n";
            std::cout << "  Files scanned: " << result.files_scanned << "\n";
            std::cout << "  Directories scanned: " << result.directories_scanned << "\n";
        } else {
            std::cerr << "✗ Scan failed with " << result.errors.size() << " error(s):\n";
            for (const auto& error : result.errors) {
                std::cerr << "  [FAIL] " << error << "\n";
            }
        }
    }

    return result.passed ? 0 : 1;
}
