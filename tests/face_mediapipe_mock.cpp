#include "FaceMediaPipe.h"

#include <cstdint>

struct FaceMPHandle
{
    int value;
};

extern "C" FACE_MEDIAPIPE_API std::uint32_t face_mp_api_version()
{
#ifdef FACE_MP_MOCK_WRONG_VERSION
    return 99;
#else
    return 1;
#endif
}

extern "C" FACE_MEDIAPIPE_API FaceMPHandle* face_mp_create(const char*)
{
    static FaceMPHandle handle{42};
    return &handle;
}

extern "C" FACE_MEDIAPIPE_API std::int32_t face_mp_process_bgr(
    FaceMPHandle*,
    const std::uint8_t*,
    std::int32_t,
    std::int32_t,
    std::int32_t,
    FaceMPResult* result)
{
    if (result != nullptr)
    {
        result->detected = 1;
    }
    return 1;
}

extern "C" FACE_MEDIAPIPE_API const char* face_mp_last_error(FaceMPHandle*)
{
    return "mock diagnostic";
}

#ifndef FACE_MP_MOCK_MISSING_DESTROY
extern "C" FACE_MEDIAPIPE_API void face_mp_destroy(FaceMPHandle*)
{
}
#endif
