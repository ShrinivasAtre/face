#pragma once

#include <stdint.h>

#ifdef _WIN32
    #ifdef FACE_MEDIAPIPE_BUILD
        #define FACE_MEDIAPIPE_API __declspec(dllexport)
    #else
        #define FACE_MEDIAPIPE_API __declspec(dllimport)
    #endif
#else
    #define FACE_MEDIAPIPE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opaque handle owned by the MediaPipe shared library.
 *
 * The application must never access the contents of this structure.
 */
typedef struct FaceMPHandle FaceMPHandle;

/*
 * One facial landmark.
 *
 * x/y are expressed in image pixels by the C API. z is backend/model
 * dependent and is retained when MediaPipe provides it.
 */
typedef struct FaceMPLandmark
{
    float x;
    float y;
    float z;
} FaceMPLandmark;

/*
 * Result storage supplied by the caller.
 *
 * Before calling face_mp_process_bgr(), the caller must set:
 *
 *     landmarks       -> storage for FaceMPLandmark entries
 *     landmark_capacity -> number of entries available in that storage
 *
 * The library never allocates memory for the landmark array and never
 * retains the caller's pointer after face_mp_process_bgr() returns.
 */
typedef struct FaceMPResult
{
    int32_t detected;

    int32_t face_x;
    int32_t face_y;
    int32_t face_width;
    int32_t face_height;

    int32_t landmark_count;

    FaceMPLandmark* landmarks;
    int32_t landmark_capacity;
} FaceMPResult;

/*
 * Return the ABI version implemented by the shared library.
 *
 * This allows the application to reject an incompatible DLL/SO before
 * creating a MediaPipe instance.
 */
FACE_MEDIAPIPE_API
uint32_t face_mp_api_version(void);

/*
 * Create and initialize a MediaPipe Face Landmarker instance.
 *
 * model_path must point to a MediaPipe Face Landmarker .task model.
 *
 * Returns NULL on failure. When creation fails there is no handle on which
 * face_mp_last_error() can be called; callers should therefore also use the
 * library's diagnostic output while integrating creation failures.
 */
FACE_MEDIAPIPE_API
FaceMPHandle* face_mp_create(const char* model_path);

/*
 * Process one BGR image using the MediaPipe Face Landmarker.
 *
 * The image uses the same memory layout as a continuous or strided OpenCV
 * CV_8UC3 BGR image:
 *
 *     bgr    -> first pixel
 *     width  -> image width in pixels
 *     height -> image height in pixels
 *     stride -> bytes between successive rows
 *
 * The caller owns the image memory. The library does not retain the pointer
 * after this function returns.
 *
 * Returns 1 on successful processing and 0 on failure.
 * A successful call may still report detected == 0 when no face is found.
 */
FACE_MEDIAPIPE_API
int32_t face_mp_process_bgr(
    FaceMPHandle* handle,
    const uint8_t* bgr,
    int32_t width,
    int32_t height,
    int32_t stride,
    FaceMPResult* result);

/*
 * Return the last error recorded by this handle.
 *
 * The returned string is owned by the library and remains valid until the
 * next API call on the same handle or until face_mp_destroy() is called.
 * Returns an empty string when no error has been recorded.
 */
FACE_MEDIAPIPE_API
const char* face_mp_last_error(FaceMPHandle* handle);

/*
 * Destroy a MediaPipe instance created by face_mp_create().
 *
 * Passing NULL is allowed and has no effect.
 */
FACE_MEDIAPIPE_API
void face_mp_destroy(FaceMPHandle* handle);

#ifdef __cplusplus
}
#endif
