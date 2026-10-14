#include "config.h"

#include <string>

namespace safedist {

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

// Removes a trailing "# comment" that is not inside quotes.
std::string strip_comment(const std::string& text) {
    char quote = '\0';
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (quote != '\0') {
            if (c == quote) quote = '\0';
        } else if (c == '"' || c == '\'') {
            quote = c;
        } else if (c == '#' && (i == 0 || text[i - 1] == ' ' || text[i - 1] == '\t')) {
            return text.substr(0, i);
        }
    }
    return text;
}

std::string unquote(const std::string& text) {
    if (text.size() >= 2 && (text.front() == '"' || text.front() == '\'') && text.back() == text.front()) {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

bool parse_bool(const std::string& value) {
    return value == "true" || value == "yes" || value == "on" || value == "1";
}

}  // namespace

ScanConfig parse_config(std::istream& input) {
    ScanConfig config;
    std::vector<std::string>* current_list = nullptr;

    std::string raw_line;
    while (std::getline(input, raw_line)) {
        const std::string line = trim(strip_comment(raw_line));
        if (line.empty()) continue;

        if (line.front() == '-') {
            const std::string item = unquote(trim(line.substr(1)));
            if (current_list != nullptr && !item.empty()) current_list->push_back(item);
            continue;
        }

        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const std::string key = trim(line.substr(0, colon));
        const std::string value = unquote(trim(line.substr(colon + 1)));

        current_list = nullptr;
        if (key == "strict") {
            config.strict_mode = parse_bool(value);
        } else if (key == "debris_patterns") {
            config.debris_patterns.clear();
            current_list = &config.debris_patterns;
        } else if (key == "sensitive_patterns") {
            config.sensitive_patterns.clear();
            current_list = &config.sensitive_patterns;
        } else if (key == "exclude_patterns") {
            current_list = &config.exclude_patterns;
        }
    }

    return config;
}

}  // namespace safedist
