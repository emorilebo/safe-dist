import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy


class SafeDistConan(ConanFile):
    name = "safe-dist"
    version = "0.2.0"
    description = "Catch debris and secrets (.DS_Store, *.bak, .env, private keys) in build output before you ship it."
    license = "MIT"
    author = "Godfrey Lebo <emorylebo@gmail.com>"
    url = "https://github.com/emorilebo/safe-dist"
    homepage = "https://github.com/emorilebo/safe-dist"
    topics = ("security", "cli", "ci-cd", "secrets", "release-hygiene")
    package_type = "application"
    settings = "os", "arch", "compiler", "build_type"

    exports_sources = "CMakeLists.txt", "src/*", "LICENSE"

    def layout(self):
        cmake_layout(self)

    def validate(self):
        check_min_cppstd(self, 17)

    def package_id(self):
        del self.info.settings.compiler

    def generate(self):
        CMakeToolchain(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        CMake(self).install()

    def package_info(self):
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
