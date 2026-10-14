# safe-dist

**Catch `.DS_Store`, `*.bak`, `.env` files and private keys in your build output before you ship it.**
A tiny, dependency-free C++17 CLI and GitHub Action that acts as a release-hygiene gate for CI/CD pipelines.

[![CI](https://github.com/emorilebo/safe-dist/actions/workflows/ci.yml/badge.svg)](https://github.com/emorilebo/safe-dist/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/emorilebo/safe-dist)](https://github.com/emorilebo/safe-dist/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

Every `.DS_Store` in a production bucket is a little postcard that says *"a Mac was here."*
Every `.env` in a public `dist/` folder is a much less charming postcard. safe-dist reads the
postcards before the internet does.

```text
$ safe-dist ./dist
Scanning directory: ./dist...
FAIL: 3 issue(s) found:
  [FAIL] Found debris file: ./dist/.DS_Store
  [FAIL] Found sensitive file: ./dist/.env
  [FAIL] Found debris file: ./dist/assets/app.js.bak
```

## Why

`.gitignore` keeps junk out of your repository. It does nothing for the folder your build tool
assembles, the Docker context you copy, or the `public/` directory a static-site generator
publishes wholesale. Those are the folders that actually reach users, and they are where
`.env` files, editor swap files, AppleDouble `._*` forks and `id_rsa` keys quietly hitch a ride.

safe-dist scans that folder and fails the pipeline with exit code `1` if anything that should
not ship is there. Think of it as airport security for `dist/`: no liquids over 100 ml, no
private keys, and the `.DS_Store` goes in the bin.

- **Zero dependencies.** One static binary; builds with any C++17 compiler.
- **CI-native.** Meaningful exit codes, a JSON report, and a GitHub Action that annotates failures.
- **Predictable patterns.** Glob rules that match names *or* relative paths, and excluded
  directories are skipped entirely (so `node_modules` costs nothing).

## What it catches

| Check | Default patterns | When |
|-------|------------------|------|
| **Debris** | `.DS_Store`, `._*`, `__MACOSX`, `Thumbs.db`, `desktop.ini`, `*.bak`, `*.tmp`, `*.swp`, `*.swo`, `*~`, `*.orig`, `*.rej` | Always |
| **Sensitive files** | `.env`, `.env.local`, `.env.*.local`, `.env.development`, `.env.production`, `.env.staging`, `.git-credentials`, `.netrc`, `.pypirc`, `.htpasswd`, `**/.aws/credentials`, `id_rsa`, `id_dsa`, `id_ecdsa`, `id_ed25519`, `*.p12`, `*.pfx`, `*.jks`, `*.keystore` | Always |
| **World-writable entries** | any file or directory with the `o+w` bit (POSIX only) | `--strict` |
| **Escaping symlinks** | symlinks that resolve outside the scanned directory | `--strict` |
| **Unreadable directories** | anything the scanner cannot open is reported, never silently skipped | Always |

`.git` directories are always skipped. Both pattern lists are configurable.

## Install

### GitHub Action (recommended)

```yaml
- uses: actions/checkout@v4
- run: npm ci && npm run build   # your build
- uses: emorilebo/safe-dist@v0.2.1
  with:
    path: dist
    strict: true
    exclude: |
      node_modules
```

The action compiles safe-dist from source on the runner (Linux, macOS and Windows; CMake is
preinstalled on GitHub-hosted runners), runs the scan, and turns every finding into an error
annotation on the run. Inputs: `path`, `strict`, `exclude` (newline-separated), `config`, `json-report`.

### Prebuilt binaries

Every [release](https://github.com/emorilebo/safe-dist/releases) ships binaries for Linux
(x86_64 and arm64, statically linked), macOS (universal) and Windows (x86_64), plus `SHA256SUMS.txt`.

```bash
curl -sSL https://github.com/emorilebo/safe-dist/releases/download/v0.2.1/safe-dist-v0.2.1-linux-x86_64.tar.gz | tar xz
./safe-dist-v0.2.1-linux-x86_64/safe-dist --version
```

### Build from source

```bash
git clone https://github.com/emorilebo/safe-dist.git
cd safe-dist
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --prefix ~/.local   # installs bin/safe-dist
```

Requires CMake 3.15+ and a C++17 compiler with `<filesystem>` (GCC 8+, Clang 9+, Apple Clang 11+ / macOS 10.15+, MSVC 2019+).

### Conan 2

safe-dist ships a Conan 2 recipe, and a [ConanCenter submission](https://github.com/conan-io/conan-center-index/pull/29065)
is in review. Until it is merged, create the package from this repository:

```bash
git clone https://github.com/emorilebo/safe-dist.git && cd safe-dist
conan create . --build=missing
```

Then consume it as a tool requirement anywhere on that machine:

```bash
conan install --tool-requires=safe-dist/0.2.1 --build=missing
source conanbuild.sh        # puts safe-dist on PATH (conanbuild.bat on Windows)
safe-dist --strict ./dist
```

## Usage

```text
Usage: safe-dist [OPTIONS] [DIRECTORY]

  -s, --strict        Also check permissions and symlink targets
  -j, --json          Print a JSON report to stdout
  -e, --exclude PAT   Skip entries matching PAT (repeatable)
  -c, --config FILE   Load configuration from FILE
      --no-config     Do not auto-load ./.safe-dist.yaml
  -h, --help          Show this help message
  -v, --version       Show version information
```

`DIRECTORY` defaults to the current directory.

```bash
safe-dist ./dist                         # debris + sensitive files
safe-dist --strict ./dist                # plus permissions and symlinks
safe-dist --json ./dist > report.json    # machine-readable report
safe-dist -e node_modules -e '*.log' .   # skip noise
```

### Exit codes

| Code | Meaning |
|------|---------|
| `0` | Scan passed, no issues found |
| `1` | Issues found, or part of the directory could not be read |
| `2` | Usage error, unreadable config file, or the target is not a directory |

### Pattern rules

- A pattern **without** `/` matches a file or directory **name** at any depth: `node_modules`, `*.bak`.
- A pattern **with** `/` matches the **path relative to the scanned directory**: `assets/*.map`, `**/.aws/credentials`.
- `*` matches anything except `/`, `**` matches across directories, `?` matches a single character.
- Excluding a directory skips everything inside it, and excluded entries are not counted.

## Configuration

Drop a `.safe-dist.yaml` in the directory you run safe-dist from and it is picked up automatically
(`--config FILE` to point elsewhere, `--no-config` to ignore it). See
[`.safe-dist.yaml.example`](.safe-dist.yaml.example) for every option.

```yaml
strict: true

# Listing debris_patterns or sensitive_patterns REPLACES the built-in list.
debris_patterns:
  - ".DS_Store"
  - "*.bak"

# Appended to any --exclude flags.
exclude_patterns:
  - node_modules
  - "*.md"
```

Command-line flags win: `--strict` turns strict mode on even if the file says `false`.

## JSON report

```json
{
  "tool": "safe-dist",
  "version": "0.2.1",
  "passed": false,
  "files_scanned": 6,
  "directories_scanned": 1,
  "error_count": 3,
  "errors": [
    "Found debris file: dist/.DS_Store",
    "Found sensitive file: dist/.env",
    "Found debris file: dist/assets/app.js.bak"
  ],
  "findings": [
    {"kind": "debris", "path": "dist/.DS_Store"},
    {"kind": "sensitive", "path": "dist/.env"},
    {"kind": "debris", "path": "dist/assets/app.js.bak"}
  ]
}
```

`kind` is one of `debris`, `sensitive`, `world_writable`, `symlink_escape`, `scan_error`.

## Other CI systems

**GitLab CI**

```yaml
release-hygiene:
  stage: test
  script:
    - curl -sSL https://github.com/emorilebo/safe-dist/releases/download/v0.2.1/safe-dist-v0.2.1-linux-x86_64.tar.gz | tar xz
    - ./safe-dist-v0.2.1-linux-x86_64/safe-dist --strict dist
```

**Any shell**: safe-dist is a single binary with exit codes, so `safe-dist dist || exit 1` works in
Jenkins, Buildkite, CircleCI, a Makefile, or a pre-release script on your laptop.

## How it works

safe-dist walks the directory with `std::filesystem`, prunes excluded directories before
descending into them, and checks each entry's name and relative path against glob patterns
(sensitive first, then debris). In strict mode it inspects POSIX permission bits without following
symlinks, and resolves symlinks to check they stay inside the scanned root. A directory that cannot
be opened is reported as a finding: a gate that silently skips what it can't read is not a gate.

## Limitations

- **Names, not contents.** safe-dist will not find an API key pasted into `main.js`. Pair it with a
  content scanner such as [gitleaks](https://github.com/gitleaks/gitleaks) or
  [TruffleHog](https://github.com/trufflesecurity/trufflehog). safe-dist is the lint roller, not the forensics lab.
- **Archives are not unpacked.** It scans directories, not the contents of `.zip`/`.tar` files.
- **Windows permissions.** `std::filesystem` reports every writable file as `0777` on Windows, so the
  world-writable check only runs on POSIX systems (Linux, macOS, BSD).
- **Pattern matching is case-sensitive.**

## FAQ

### How do I stop `.DS_Store` files from being deployed?
Run `safe-dist ./dist` (or the `emorilebo/safe-dist` GitHub Action) after your build and before your
deploy step. It exits with code `1` when `.DS_Store`, `._*` AppleDouble files, `__MACOSX` folders or
`Thumbs.db` are present, which fails the pipeline before anything is uploaded.

### How do I check a build folder for `.env` files and private keys before deploying?
safe-dist flags `.env` variants, `.netrc`, `.pypirc`, `.git-credentials`, `.aws/credentials`, SSH private
keys (`id_rsa`, `id_ed25519`, ...) and certificate/keystore bundles (`*.p12`, `*.pfx`, `*.jks`, `*.keystore`) by
default. Point it at the folder you ship, e.g. `safe-dist --strict build/`.

### Does safe-dist scan file contents for secrets?
No. safe-dist checks file names, paths, permissions and symlink targets, which keeps it fast and its
results predictable. For secrets embedded inside files, run a content scanner like gitleaks or TruffleHog
alongside it.

### Isn't `.gitignore` or `.dockerignore` enough?
Ignore files only apply to the tool that reads them. Build steps that copy directories, static-site
generators and packaging scripts can still sweep local files into your output. safe-dist checks the
final artifact folder, which is the thing your users actually receive.

### Can I use safe-dist with Conan?
Yes. The repository contains a Conan 2 recipe (`conan create .`), and the package is consumed as a tool
requirement (`conan install --tool-requires=safe-dist/0.2.1`). A ConanCenter listing is in review.

### Why does `--strict` skip the permission check on Windows?
Windows has no POSIX "others" permission bit; the C++ standard library reports every writable file as
world-writable there. Rather than fail every Windows build, safe-dist only runs that check on POSIX systems.

## Roadmap

- SARIF output for GitHub code scanning
- Scanning inside `.zip` / `.tar.gz` artifacts
- ConanCenter listing (in review)

## Contributing

Issues and pull requests are welcome. Please run the tests before submitting:

```bash
cmake -S . -B build -DSAFE_DIST_BUILD_TESTS=ON && cmake --build build && ctest --test-dir build --output-on-failure
```

## Related projects by the same author

- [npm_sentinel](https://crates.io/crates/npm_sentinel): detect npm supply-chain attacks in lifecycle scripts (Rust)
- [rust_secure_dependency_audit](https://crates.io/crates/rust_secure_dependency_audit): health, license and maintenance audit for Rust dependencies
- [rag-poison-guard](https://www.npmjs.com/package/rag-poison-guard): sanitize untrusted content against indirect prompt injection in RAG
- [redact-ai-stream](https://www.npmjs.com/package/redact-ai-stream): bi-directional PII redaction stream for AI applications
- [hallucination-validator](https://www.npmjs.com/package/hallucination-validator): validate LLM output for link rot, dangerous code and hallucinations

## About the author

Built by **[Godfrey Lebo](https://godfreylebo.dev)**, Senior Backend & Product Engineer and founder of Sether.
I build the backend, connect the product, and stay responsible for what happens after launch. Lately
that means open-source tooling for AI security and software supply-chain security.

[Website](https://godfreylebo.dev) · [GitHub](https://github.com/emorilebo) · [X / Twitter](https://x.com/emori_lebo) · [LinkedIn](https://www.linkedin.com/in/godfreylebo) · [Email](mailto:emorylebo@gmail.com)

## License

[MIT](LICENSE) © Godfrey Lebo
