# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.1] - 2026-10-14

### Fixed
- Release workflow: archives were uploaded together with their staging folders, so the publish job
  failed while computing checksums and the 0.2.0 tag got no release binaries. No code changes.

## [0.2.0] - 2026-10-14

### Added
- Sensitive-file detection on by default: `.env` variants, `.netrc`, `.pypirc`, `.git-credentials`,
  `.htpasswd`, `**/.aws/credentials`, SSH private keys, `*.p12`, `*.pfx`, `*.jks`, `*.keystore`.
  Configurable via `sensitive_patterns`.
- More default debris patterns: `._*` (AppleDouble), `__MACOSX`, `desktop.ini`, `*.swp`, `*.swo`, `*~`,
  `*.orig`, `*.rej`.
- Strict mode flags symlinks that resolve outside the scanned directory.
- `.safe-dist.yaml` in the working directory is now loaded automatically, as the docs always claimed;
  `--no-config` disables it.
- JSON report gains `tool`, `version` and a structured `findings` array (`kind` + `path`).
- GitHub Action (`uses: emorilebo/safe-dist@v0.2.0`) with error annotations.
- Release workflow publishing static Linux (x86_64, arm64), universal macOS and Windows binaries with checksums.
- Unit test suite (CTest), sanitizer CI job, and a CI job that dogfoods the action.

### Fixed
- Patterns were substring-matched, so `.tmp` flagged `index.tmpl` and `.bak` flagged `bakery.js`.
  Patterns are now proper globs (`*`, `**`, `?`).
- Exclude patterns containing `/` (e.g. `node_modules/*`) never matched, and excluded directories were
  still traversed and counted. Excluded directories are now pruned.
- One unreadable subdirectory aborted the whole scan; it is now reported and the scan continues.
- `--strict` on Windows flagged every writable file as world-writable; the POSIX-only check is now skipped there.
- Symlinks were permission-checked through their target; they are now checked with `symlink_status`.
- JSON output did not escape control characters.
- `--exclude`/`--config` without a value printed "Unknown option".
- The version string was hard-coded in three places; it now comes from CMake.
- Conan recipe: `package_type = "application"`, C++17 validation, compiler removed from the package ID;
  test_package no longer builds a dummy executable.
- README install commands used Conan 1 syntax (`conan install safe-dist/0.1.0@`).

### Changed
- Usage errors, unreadable config files and a missing target directory now exit with code `2`
  (previously `1`), so CI can tell "dirty artifact" apart from "misconfigured step".
- A config file given with `--config` that cannot be read is now an error instead of a warning.
- Build artifacts that were accidentally committed under `test_package/build/` were removed.

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

[0.2.1]: https://github.com/emorilebo/safe-dist/releases/tag/v0.2.1
[0.2.0]: https://github.com/emorilebo/safe-dist/releases/tag/v0.2.0
[0.1.0]: https://github.com/emorilebo/safe-dist/releases/tag/v0.1.0
