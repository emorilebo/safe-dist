#include <iostream>

int main() {
    // This C++ test isn't strictly necessary for a CLI tool package 
    // because we verify the tool binary in conanfile.py's test() method.
    std::cout << "Test package built successfully!\n";
    return 0;
}
