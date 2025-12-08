# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-12-08

### Added
- Initial release
- Directory scanning for debris files (`.DS_Store`, `Thumbs.db`, `*.bak`, `*.tmp`)
- Strict mode for detecting world-writable files
- `--json` flag for machine-readable output
- `--config` flag for custom configuration files
- `--exclude` flag for exclusion patterns
- `.safe-dist.yaml` configuration file support
- Conan 2.x packaging support
- Cross-platform support (Linux, macOS, Windows)
- GitHub Actions CI workflow

### Security
- Detects insecure file permissions (world-writable) in strict mode
