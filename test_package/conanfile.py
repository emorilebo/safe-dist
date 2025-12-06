import os
from conan import ConanFile
from conan.tools.build import can_run

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualBuildEnv"
    test_type = "explicit"

    def layout(self):
        from conan.tools.cmake import cmake_layout
        cmake_layout(self)

    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    def build(self):
        from conan.tools.cmake import CMake
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            # Test that the binary exists and runs
            self.run("safe-dist --help", env="conanbuild")
