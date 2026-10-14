// Dependency-free unit tests for safe-dist. Run via `ctest` or the `safe_dist_tests` binary.

#include "config.h"
#include "scanner.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using safedist::FindingKind;

namespace {

int g_failures = 0;

#define CHECK(condition)                                                                   \
    do {                                                                                   \
        if (!(condition)) {                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition "\n"; \
            ++g_failures;                                                                  \
        }                                                                                  \
    } while (0)

class TempTree {
public:
    TempTree() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = fs::temp_directory_path() / ("safe-dist-test-" + std::to_string(stamp));
        fs::create_directories(root_);
    }
    ~TempTree() {
        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(root_, ec); !ec && it != fs::recursive_directory_iterator();
             it.increment(ec)) {
            fs::permissions(it->path(), fs::perms::owner_all, fs::perm_options::add, ec);
        }
        fs::remove_all(root_, ec);
    }
    TempTree(const TempTree&) = delete;
    TempTree& operator=(const TempTree&) = delete;

    const fs::path& root() const { return root_; }

    fs::path file(const std::string& relative, const std::string& contents = "x") const {
        const fs::path path = root_ / relative;
        fs::create_directories(path.parent_path());
        std::ofstream(path) << contents;
        return path;
    }

    fs::path dir(const std::string& relative) const {
        const fs::path path = root_ / relative;
        fs::create_directories(path);
        return path;
    }

private:
    fs::path root_;
};

size_t count_kind(const safedist::ScanResult& result, FindingKind kind) {
    return static_cast<size_t>(std::count_if(result.findings.begin(), result.findings.end(),
                                             [kind](const safedist::Finding& f) { return f.kind == kind; }));
}

bool has_finding(const safedist::ScanResult& result, FindingKind kind, const std::string& path_suffix) {
    return std::any_of(result.findings.begin(), result.findings.end(), [&](const safedist::Finding& f) {
        const std::string generic = fs::path(f.path).generic_string();
        return f.kind == kind && generic.size() >= path_suffix.size() &&
               generic.compare(generic.size() - path_suffix.size(), path_suffix.size(), path_suffix) == 0;
    });
}

void test_glob_match() {
    using safedist::glob_match;
    CHECK(glob_match("*.bak", "app.js.bak"));
    CHECK(!glob_match("*.bak", "bakery.js"));
    CHECK(!glob_match("*.tmp", "index.tmpl"));
    CHECK(glob_match(".DS_Store", ".DS_Store"));
    CHECK(!glob_match(".tmp", "index.tmpl"));  // 0.1.0 substring-matched this: false positive
    CHECK(glob_match("._*", "._photo.jpg"));
    CHECK(glob_match("*~", "notes.txt~"));
    CHECK(glob_match("file?.txt", "file1.txt"));
    CHECK(!glob_match("file?.txt", "file12.txt"));
    CHECK(glob_match("test/*", "test/a.txt"));
    CHECK(!glob_match("test/*", "test/sub/a.txt"));  // '*' stops at '/'
    CHECK(glob_match("test/**", "test/sub/a.txt"));
    CHECK(glob_match("**/.aws/credentials", ".aws/credentials"));
    CHECK(glob_match("**/.aws/credentials", "home/user/.aws/credentials"));
    CHECK(!glob_match("**/.aws/credentials", "x.aws/credentials"));
    CHECK(glob_match(".env.*.local", ".env.production.local"));
    CHECK(!glob_match(".env", ".env.example"));
    CHECK(glob_match("*", ""));
    CHECK(!glob_match("", "a"));
}

void test_matches_entry() {
    using safedist::matches_entry;
    CHECK(matches_entry("node_modules", "node_modules", "web/node_modules"));
    CHECK(matches_entry("node_modules/", "node_modules", "node_modules"));
    CHECK(matches_entry("./build/*.map", "app.js.map", "build/app.js.map"));
    CHECK(!matches_entry("build/*.map", "app.js.map", "web/build/app.js.map"));
    CHECK(matches_entry("*.log", "server.log", "logs/server.log"));
    CHECK(!matches_entry("", "anything", "anything"));
}

void test_json_escape() {
    CHECK(safedist::json_escape("a\"b\\c") == "a\\\"b\\\\c");
    CHECK(safedist::json_escape("line\nbreak\ttab") == "line\\nbreak\\ttab");
    CHECK(safedist::json_escape(std::string("bell\x07", 5)) == "bell\\u0007");
}

void test_parse_config() {
    std::istringstream input(
        "# safe-dist config\n"
        "strict: yes   # inline comment\n"
        "debris_patterns:\n"
        "  - \"*.bak\"\n"
        "  - '#hash#'\n"
        "  - Thumbs.db  # trailing comment\n"
        "future_option: 42\n"
        "exclude_patterns:\n"
        "  - node_modules\n"
        "  -\n");
    const safedist::ScanConfig config = safedist::parse_config(input);
    CHECK(config.strict_mode);
    CHECK((config.debris_patterns == std::vector<std::string>{"*.bak", "#hash#", "Thumbs.db"}));
    CHECK((config.exclude_patterns == std::vector<std::string>{"node_modules"}));
    CHECK(config.sensitive_patterns == safedist::default_sensitive_patterns());

    std::istringstream empty("");
    CHECK(safedist::parse_config(empty).debris_patterns == safedist::default_debris_patterns());
}

void test_clean_tree_passes() {
    TempTree tree;
    tree.file("index.html");
    tree.file("templates/page.tmpl");
    tree.file("assets/bakery.js");
    tree.file(".env.example");

    safedist::Scanner scanner{safedist::ScanConfig{}};
    const auto result = scanner.scan(tree.root());
    CHECK(result.passed);
    CHECK(result.findings.empty());
    CHECK(result.files_scanned == 4);
    CHECK(result.directories_scanned == 2);
}

void test_debris_and_sensitive_detected() {
    TempTree tree;
    tree.file("index.html");
    tree.file(".DS_Store");
    tree.file("assets/app.js.bak");
    tree.file("__MACOSX/._index.html");
    tree.file(".env", "API_KEY=do-not-ship-me");
    tree.file("deploy/id_ed25519");
    tree.file("config/.aws/credentials");

    safedist::Scanner scanner{safedist::ScanConfig{}};
    const auto result = scanner.scan(tree.root());
    CHECK(!result.passed);
    CHECK(has_finding(result, FindingKind::Debris, "/.DS_Store"));
    CHECK(has_finding(result, FindingKind::Debris, "/assets/app.js.bak"));
    CHECK(has_finding(result, FindingKind::Debris, "/__MACOSX"));
    CHECK(has_finding(result, FindingKind::Sensitive, "/.env"));
    CHECK(has_finding(result, FindingKind::Sensitive, "/deploy/id_ed25519"));
    CHECK(has_finding(result, FindingKind::Sensitive, "/config/.aws/credentials"));
    CHECK(count_kind(result, FindingKind::Debris) == 3);  // __MACOSX is reported once, not per child
    CHECK(result.errors.size() == result.findings.size());
}

void test_excluded_directories_are_pruned() {
    TempTree tree;
    tree.file("app.js");
    tree.file("node_modules/pkg/old.bak");
    tree.file("web/node_modules/pkg/.DS_Store");
    tree.file("test/fixtures/sample.tmp");
    tree.file(".git/objects/aa.tmp");
    tree.file("logs/server.log");

    safedist::ScanConfig config;
    config.exclude_patterns = {"node_modules", "test/*", "*.log"};
    safedist::Scanner scanner(config);
    const auto result = scanner.scan(tree.root());
    CHECK(result.passed);
    CHECK(result.findings.empty());
    // app.js + test (dir) + logs (dir) + web (dir); nothing inside excluded trees is counted.
    CHECK(result.files_scanned == 1);
    CHECK(result.directories_scanned == 3);
}

void test_missing_target_is_reported() {
    safedist::Scanner scanner(false);
    const auto result = scanner.scan(fs::path("definitely/not/here"));
    CHECK(!result.passed);
    CHECK(count_kind(result, FindingKind::ScanError) == 1);
}

void test_json_report_shape() {
    TempTree tree;
    tree.file("Thumbs.db");
    safedist::Scanner scanner{safedist::ScanConfig{}};
    const std::string json = scanner.scan(tree.root()).to_json();
    CHECK(json.find("\"tool\": \"safe-dist\"") != std::string::npos);
    CHECK(json.find("\"passed\": false") != std::string::npos);
    CHECK(json.find("\"kind\": \"debris\"") != std::string::npos);
    CHECK(json.find("\"error_count\": 1") != std::string::npos);
}

#ifndef _WIN32
void test_strict_world_writable() {
    TempTree tree;
    const fs::path shared = tree.file("shared.txt");
    fs::permissions(shared, fs::perms::others_write, fs::perm_options::add);

    safedist::ScanConfig relaxed;
    CHECK(safedist::Scanner(relaxed).scan(tree.root()).passed);

    safedist::ScanConfig strict;
    strict.strict_mode = true;
    const auto result = safedist::Scanner(strict).scan(tree.root());
    CHECK(!result.passed);
    CHECK(has_finding(result, FindingKind::WorldWritable, "/shared.txt"));
}

void test_strict_symlink_escape() {
    TempTree outside;
    const fs::path secret = outside.file("secret.txt");
    TempTree tree;
    tree.file("real.txt");
    fs::create_symlink(secret, tree.root() / "escape.txt");
    fs::create_symlink(tree.root() / "real.txt", tree.root() / "inside.txt");

    safedist::ScanConfig strict;
    strict.strict_mode = true;
    const auto result = safedist::Scanner(strict).scan(tree.root());
    CHECK(has_finding(result, FindingKind::SymlinkEscape, "/escape.txt"));
    CHECK(!has_finding(result, FindingKind::SymlinkEscape, "/inside.txt"));
    // Symlink permission bits are always 0777 on Linux; they must not be reported as world-writable.
    CHECK(count_kind(result, FindingKind::WorldWritable) == 0);
}

void test_unreadable_directory_does_not_abort_scan() {
    TempTree tree;
    tree.file("locked/inner.txt");
    tree.file("z-visible/.DS_Store");
    const fs::path locked = tree.root() / "locked";
    fs::permissions(locked, fs::perms::none);

    std::error_code probe_ec;
    fs::directory_iterator probe(locked, probe_ec);
    if (!probe_ec) {
        std::cout << "  (skipped: running with privileges that ignore permissions)\n";
        fs::permissions(locked, fs::perms::owner_all);
        return;
    }

    const auto result = safedist::Scanner(safedist::ScanConfig{}).scan(tree.root());
    CHECK(!result.passed);
    CHECK(has_finding(result, FindingKind::ScanError, "/locked"));
    CHECK(has_finding(result, FindingKind::Debris, "/z-visible/.DS_Store"));
    fs::permissions(locked, fs::perms::owner_all);
}
#endif

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"glob_match", test_glob_match},
        {"matches_entry", test_matches_entry},
        {"json_escape", test_json_escape},
        {"parse_config", test_parse_config},
        {"clean_tree_passes", test_clean_tree_passes},
        {"debris_and_sensitive_detected", test_debris_and_sensitive_detected},
        {"excluded_directories_are_pruned", test_excluded_directories_are_pruned},
        {"missing_target_is_reported", test_missing_target_is_reported},
        {"json_report_shape", test_json_report_shape},
#ifndef _WIN32
        {"strict_world_writable", test_strict_world_writable},
        {"strict_symlink_escape", test_strict_symlink_escape},
        {"unreadable_directory_does_not_abort_scan", test_unreadable_directory_does_not_abort_scan},
#endif
    };

    for (const auto& [name, test] : tests) {
        std::cout << "[ RUN  ] " << name << "\n";
        const int before = g_failures;
        test();
        std::cout << (g_failures == before ? "[  OK  ] " : "[ FAIL ] ") << name << "\n";
    }

    if (g_failures > 0) {
        std::cerr << g_failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "All " << tests.size() << " tests passed\n";
    return 0;
}
