#pragma once

#include "scanner.h"

#include <istream>
#include <string>

namespace safedist {

// Name of the configuration file discovered in the working directory when --config is not given.
inline constexpr const char* kDefaultConfigFile = ".safe-dist.yaml";

// Parses the small YAML subset safe-dist understands:
//
//   strict: true
//   debris_patterns:      # replaces the built-in list
//     - "*.bak"
//   sensitive_patterns:   # replaces the built-in list
//     - ".env"
//   exclude_patterns:     # appended to command-line excludes
//     - node_modules
//
// Unknown keys are ignored so that newer config files keep working with older binaries.
ScanConfig parse_config(std::istream& input);

}  // namespace safedist
