#include "FaceMediaPipeRuntime.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

namespace
{
bool expect(bool condition, const char* label)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << label << '\n';
    }
    return condition;
}
}

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: face_mediapipe_runtime_test "
                  << "<valid-library> <missing-symbol-library> "
                  << "<wrong-version-library>\n";
        return 2;
    }

    const std::filesystem::path valid = argv[1];
    const std::filesystem::path missingSymbol = argv[2];
    const std::filesystem::path wrongVersion = argv[3];
    bool ok = true;

    FaceMediaPipeRuntime runtime;
    ok &= expect(!runtime.isLoaded(), "initially unloaded");
    ok &= expect(runtime.apiVersion() == 0, "unloaded version is zero");

    ok &= expect(!runtime.load({}), "empty path rejected");
    ok &= expect(!runtime.diagnostic().empty(), "empty path diagnostic");

    const auto absent = valid.parent_path() / "definitely-absent-library";
    ok &= expect(!runtime.load(absent), "missing file rejected");
    ok &= expect(!runtime.isLoaded(), "missing file leaves unloaded");
    ok &= expect(!runtime.diagnostic().empty(), "missing file diagnostic");

    ok &= expect(!runtime.load(missingSymbol), "missing symbol rejected");
    ok &= expect(!runtime.isLoaded(), "missing symbol leaves unloaded");
    ok &= expect(
        runtime.diagnostic().find("face_mp_destroy") != std::string::npos,
        "missing symbol named in diagnostic");

    ok &= expect(!runtime.load(wrongVersion), "wrong version rejected");
    ok &= expect(!runtime.isLoaded(), "wrong version leaves unloaded");
    ok &= expect(
        runtime.diagnostic().find("expected 1, got 99") != std::string::npos,
        "wrong version diagnostic");

    ok &= expect(runtime.load(valid), "valid library loads");
    ok &= expect(runtime.isLoaded(), "valid library reports loaded");
    ok &= expect(runtime.apiVersion() == 1, "valid API version");
    ok &= expect(runtime.diagnostic().empty(), "success clears diagnostic");

    FaceMPHandle* handle = runtime.create("unused.task");
    ok &= expect(handle != nullptr, "resolved create callable");
    FaceMPResult result{};
    ok &= expect(
        runtime.processBgr(handle, nullptr, 0, 0, 0, &result) == 1 &&
            result.detected == 1,
        "resolved process callable");
    ok &= expect(
        std::string(runtime.lastError(handle)) == "mock diagnostic",
        "resolved last-error callable");
    runtime.destroy(handle);

    FaceMediaPipeRuntime moved(std::move(runtime));
    ok &= expect(!runtime.isLoaded(), "move construction clears source");
    ok &= expect(moved.isLoaded(), "move construction transfers ownership");

    FaceMediaPipeRuntime assigned;
    assigned = std::move(moved);
    ok &= expect(!moved.isLoaded(), "move assignment clears source");
    ok &= expect(assigned.isLoaded(), "move assignment transfers ownership");
    assigned.unload();
    ok &= expect(!assigned.isLoaded(), "explicit unload");
    ok &= expect(assigned.diagnostic().empty(), "unload clears diagnostic");

    ok &= expect(assigned.load(valid), "reload after unload");
    ok &= expect(!assigned.load(absent), "failed reload rejected");
    ok &= expect(!assigned.isLoaded(), "failed reload closes prior library");
    ok &= expect(assigned.load(valid), "recovery after failure");

    if (!ok)
    {
        return 1;
    }
    std::cout << "FaceMediaPipe runtime loader tests PASSED\n";
    return 0;
}
