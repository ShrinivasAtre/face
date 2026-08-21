#pragma once

#include "FaceMediaPipe.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

// Runtime-loaded access to the stable FaceMediaPipe C ABI.
//
// This interface deliberately exposes only the bridge's plain C data types.
// Native module handles and MediaPipe SDK types remain private.
class FaceMediaPipeRuntime
{
public:
    static constexpr std::uint32_t ExpectedApiVersion = 1;

    FaceMediaPipeRuntime();
    ~FaceMediaPipeRuntime();

    FaceMediaPipeRuntime(const FaceMediaPipeRuntime&) = delete;
    FaceMediaPipeRuntime& operator=(const FaceMediaPipeRuntime&) = delete;

    FaceMediaPipeRuntime(FaceMediaPipeRuntime&& other) noexcept;
    FaceMediaPipeRuntime& operator=(FaceMediaPipeRuntime&& other) noexcept;

    bool load(const std::filesystem::path& libraryPath);
    void unload() noexcept;

    bool isLoaded() const noexcept;
    std::uint32_t apiVersion() const noexcept;
    const std::string& diagnostic() const noexcept;

    FaceMPHandle* create(const char* modelPath) const noexcept;
    std::int32_t processBgr(
        FaceMPHandle* handle,
        const std::uint8_t* bgr,
        std::int32_t width,
        std::int32_t height,
        std::int32_t stride,
        FaceMPResult* result) const noexcept;
    const char* lastError(FaceMPHandle* handle) const noexcept;
    void destroy(FaceMPHandle* handle) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
