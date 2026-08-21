#include "FaceMediaPipeRuntime.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: face_mediapipe_runtime_probe <FaceMediaPipe DLL/SO>\n";
        return 2;
    }

    FaceMediaPipeRuntime runtime;
    if (!runtime.load(std::filesystem::path(argv[1])))
    {
        std::cerr << runtime.diagnostic() << '\n';
        return 1;
    }

    std::cout << "Loaded: " << std::filesystem::absolute(argv[1]) << '\n'
              << "FaceMediaPipe API version: " << runtime.apiVersion() << '\n'
              << "Runtime loading probe PASSED\n";
    return 0;
}
