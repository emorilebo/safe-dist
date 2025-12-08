# safe-dist

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Conan](https://img.shields.io/badge/conan-C%2B%2B17-blue)](https://conan.io/)

A lightweight C++17 directory integrity checker for CI/CD pipelines. Scans your distribution artifacts for debris files and insecure permissions before deployment.

## Why safe-dist?

Before deploying software, you want to ensure your distribution is clean:

- ❌ No `.DS_Store`, `Thumbs.db`, or backup files leaked into production
- ❌ No world-writable files that could be security vulnerabilities
- ✅ Only the files you intended to ship

`safe-dist` acts as a quality gate in your CI/CD pipeline, failing the build if issues are found.

## Installation

### Via Conan (Recommended)

```bash
# Add to your conanfile.txt
[requires]
safe-dist/0.1.0

# Or install directly
conan install safe-dist/0.1.0@ --build=missing
```

### Build from Source

```bash
git clone https://github.com/emorilebo/safe-dist.git
cd safe-dist
conan create . --build=missing
```

## Usage

```bash
# Basic scan (current directory)
safe-dist

# Scan specific directory
safe-dist /path/to/dist

# Strict mode: also check for insecure permissions
safe-dist --strict /path/to/dist

# JSON output for CI/CD integration
safe-dist --json /path/to/dist

# Show help
safe-dist --help
```

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Scan passed, no issues found |
| 1 | Scan failed, issues detected |

## What It Detects

### Debris Files (Always Checked)
- `.DS_Store` (macOS Finder metadata)
- `Thumbs.db` (Windows thumbnail cache)
- `*.bak` (Backup files)
- `*.tmp` (Temporary files)

### Permission Issues (Strict Mode Only)
- World-writable files (`chmod o+w`)

## CI/CD Integration

### GitHub Actions

```yaml
name: Build and Verify
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup Conan
        run: pip install conan
        
      - name: Build
        run: |
          # Your build steps here
          make build
          
      - name: Verify Distribution
        run: |
          conan install safe-dist/0.1.0@ --build=missing
          safe-dist --strict ./dist
```

### GitLab CI

```yaml
verify_dist:
  stage: test
  script:
    - pip install conan
    - conan install safe-dist/0.1.0@ --build=missing
    - safe-dist --strict ./dist
  allow_failure: false
```

## Configuration

### Exclusion Patterns

Use `--exclude` to skip specific patterns:

```bash
safe-dist --exclude "*.log" --exclude "test/*" ./dist
```

### Configuration File

Create `.safe-dist.yaml` in your project root:

```yaml
# Custom debris patterns to detect
debris_patterns:
  - "*.bak"
  - "*.tmp"
  - ".DS_Store"
  - "Thumbs.db"
  - "*.swp"
  - "*~"

# Patterns to exclude from scanning
exclude_patterns:
  - ".git/*"
  - "node_modules/*"
  - "*.md"

# Enable strict mode by default
strict: false
```

Then run:

```bash
safe-dist --config .safe-dist.yaml ./dist
```

## Development

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.15+
- Conan 2.x

### Building

```bash
# Clone the repository
git clone https://github.com/emorilebo/safe-dist.git
cd safe-dist

# Create and test the package
conan create . --build=missing

# Or build locally for development
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Running Tests

```bash
conan test test_package safe-dist/0.1.0@
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**Godfrey Lebo**
- Website: [godfreylebo.dev](https://godfreylebo.dev)
- GitHub: [@emorilebo](https://github.com/emorilebo)
- LinkedIn: [Godfrey Lebo](https://linkedin.com/in/godfreylebo)
