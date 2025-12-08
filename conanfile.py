from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout

class SafeDistPackage(ConanFile):
    name = "safe-dist"
    version = "0.1.0"
    description = "A simple C++ directory integrity checker for CI/CD pipelines."
    author = "Godfrey Lebo <emorylebo@gmail.com>"
    license = "MIT"
    url = "https://github.com/emorilebo/safe-dist"
    homepage = "https://github.com/emorilebo/safe-dist"
    topics = ("security", "cli", "integrity", "deployment", "ci-cd")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    
    # Sources are exported with the package
    exports_sources = "CMakeLists.txt", "src/*"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = [] # This is a tool, not a library, so no libs to link
