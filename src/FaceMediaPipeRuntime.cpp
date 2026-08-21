#include "FaceMediaPipeRuntime.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <sstream>
#include <utility>

namespace
{
#ifdef _WIN32
using NativeLibrary = HMODULE;

std::string windowsErrorMessage(DWORD code)
{
    char* text = nullptr;
    const DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        0,
        reinterpret_cast<char*>(&text),
        0,
        nullptr);

    std::string message = length != 0 && text != nullptr
        ? std::string(text, length)
        : std::string("Windows error ") + std::to_string(code);

    if (text != nullptr)
    {
        LocalFree(text);
    }

    while (!message.empty() &&
           (message.back() == '\r' || message.back() == '\n'))
    {
        message.pop_back();
    }
    return message;
}
#else
using NativeLibrary = void*;
#endif
}

struct FaceMediaPipeRuntime::Impl
{
    using ApiVersionFn = std::uint32_t (*)();
    using CreateFn = FaceMPHandle* (*)(const char*);
    using ProcessBgrFn = std::int32_t (*)(
        FaceMPHandle*, const std::uint8_t*, std::int32_t, std::int32_t,
        std::int32_t, FaceMPResult*);
    using LastErrorFn = const char* (*)(FaceMPHandle*);
    using DestroyFn = void (*)(FaceMPHandle*);

    NativeLibrary library = nullptr;
    ApiVersionFn apiVersion = nullptr;
    CreateFn create = nullptr;
    ProcessBgrFn processBgr = nullptr;
    LastErrorFn lastError = nullptr;
    DestroyFn destroy = nullptr;
    std::string diagnostic;

    ~Impl()
    {
        close();
    }

    void resetFunctions() noexcept
    {
        apiVersion = nullptr;
        create = nullptr;
        processBgr = nullptr;
        lastError = nullptr;
        destroy = nullptr;
    }

    void close() noexcept
    {
        resetFunctions();
        if (library != nullptr)
        {
#ifdef _WIN32
            FreeLibrary(library);
#else
            dlclose(library);
#endif
            library = nullptr;
        }
    }

    void* symbol(const char* name)
    {
#ifdef _WIN32
        SetLastError(ERROR_SUCCESS);
        auto* address = reinterpret_cast<void*>(GetProcAddress(library, name));
        if (address == nullptr)
        {
            diagnostic = "Missing required symbol '" + std::string(name) +
                "': " + windowsErrorMessage(GetLastError());
        }
#else
        dlerror();
        void* address = dlsym(library, name);
        if (const char* error = dlerror())
        {
            diagnostic = "Missing required symbol '" + std::string(name) +
                "': " + error;
            return nullptr;
        }
#endif
        return address;
    }
};

FaceMediaPipeRuntime::FaceMediaPipeRuntime()
    : impl_(std::make_unique<Impl>())
{
}

FaceMediaPipeRuntime::~FaceMediaPipeRuntime()
{
    unload();
}

FaceMediaPipeRuntime::FaceMediaPipeRuntime(
    FaceMediaPipeRuntime&& other) noexcept = default;

FaceMediaPipeRuntime& FaceMediaPipeRuntime::operator=(
    FaceMediaPipeRuntime&& other) noexcept = default;

bool FaceMediaPipeRuntime::load(const std::filesystem::path& libraryPath)
{
    if (!impl_)
    {
        impl_ = std::make_unique<Impl>();
    }
    impl_->close();
    impl_->diagnostic.clear();

    if (libraryPath.empty())
    {
        impl_->diagnostic = "FaceMediaPipe library path is empty.";
        return false;
    }

#ifdef _WIN32
    impl_->library = LoadLibraryW(libraryPath.c_str());
    if (impl_->library == nullptr)
    {
        impl_->diagnostic = "Failed to load '" + libraryPath.string() +
            "': " + windowsErrorMessage(GetLastError());
        return false;
    }
#else
    impl_->library = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (impl_->library == nullptr)
    {
        const char* error = dlerror();
        impl_->diagnostic = "Failed to load '" + libraryPath.string() +
            "': " + (error != nullptr ? error : "unknown dlopen error");
        return false;
    }
#endif

#define FACE_MP_RESOLVE(member, type, name)                                 \
    impl_->member = reinterpret_cast<Impl::type>(impl_->symbol(name));       \
    if (impl_->member == nullptr)                                            \
    {                                                                        \
        const std::string error = impl_->diagnostic;                         \
        impl_->close();                                                      \
        impl_->diagnostic = error;                                           \
        return false;                                                        \
    }

    FACE_MP_RESOLVE(apiVersion, ApiVersionFn, "face_mp_api_version")
    FACE_MP_RESOLVE(create, CreateFn, "face_mp_create")
    FACE_MP_RESOLVE(processBgr, ProcessBgrFn, "face_mp_process_bgr")
    FACE_MP_RESOLVE(lastError, LastErrorFn, "face_mp_last_error")
    FACE_MP_RESOLVE(destroy, DestroyFn, "face_mp_destroy")

#undef FACE_MP_RESOLVE

    const std::uint32_t version = impl_->apiVersion();
    if (version != ExpectedApiVersion)
    {
        std::ostringstream message;
        message << "Incompatible FaceMediaPipe API version: expected "
                << ExpectedApiVersion << ", got " << version << '.';
        impl_->close();
        impl_->diagnostic = message.str();
        return false;
    }

    return true;
}

void FaceMediaPipeRuntime::unload() noexcept
{
    if (impl_)
    {
        impl_->close();
        impl_->diagnostic.clear();
    }
}

bool FaceMediaPipeRuntime::isLoaded() const noexcept
{
    return impl_ && impl_->library != nullptr;
}

std::uint32_t FaceMediaPipeRuntime::apiVersion() const noexcept
{
    return isLoaded() ? impl_->apiVersion() : 0;
}

const std::string& FaceMediaPipeRuntime::diagnostic() const noexcept
{
    static const std::string empty;
    return impl_ ? impl_->diagnostic : empty;
}

FaceMPHandle* FaceMediaPipeRuntime::create(const char* modelPath) const noexcept
{
    return isLoaded() ? impl_->create(modelPath) : nullptr;
}

std::int32_t FaceMediaPipeRuntime::processBgr(
    FaceMPHandle* handle,
    const std::uint8_t* bgr,
    std::int32_t width,
    std::int32_t height,
    std::int32_t stride,
    FaceMPResult* result) const noexcept
{
    return isLoaded()
        ? impl_->processBgr(handle, bgr, width, height, stride, result)
        : 0;
}

const char* FaceMediaPipeRuntime::lastError(
    FaceMPHandle* handle) const noexcept
{
    return isLoaded() ? impl_->lastError(handle) : "FaceMediaPipe is not loaded.";
}

void FaceMediaPipeRuntime::destroy(FaceMPHandle* handle) const noexcept
{
    if (isLoaded())
    {
        impl_->destroy(handle);
    }
}
