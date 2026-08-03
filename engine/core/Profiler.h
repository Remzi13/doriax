#pragma once

#include "Export.h"

#include <string>

#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>

#if defined(SOKOL_GLCORE) || defined(DORIAX_OPENGL)
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <tracy/TracyOpenGL.hpp>
#define HAS_TRACY_GPU 1
#endif

#define PROFILE_ENABLED 1
#else
#define PROFILE_ENABLED 0
#endif

// ============================================================================
//        ( ,  Tracy )
// ============================================================================

#if PROFILE_ENABLED


#define PROFILE_FRAME FrameMark
#define PROFILE_FUNCTION() ZoneScoped
#define PROFILE_FUNCTION_N(name) ZoneScopedN(name)
#define PROFILE_SCOPE(name) ZoneScopedN(name)
#define PROFILE_THREAD(name) tracy::SetThreadName(name)

#define PROFILE_TEXT(text, size) ZoneText(text, size)
#define PROFILE_LOG(text, size) TracyMessage(text, size)
    
// ������� �������� (��������, FPS, ���-�� DrawCalls, ���������� ������)
#define PROFILE_PLOT(name, val) TracyPlot(name, val)

// ����� ������
#define PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
#define PROFILE_FREE(ptr) TracyFree(ptr)

// GPU �������������� (OpenGL)
#if HAS_TRACY_GPU
#define PROFILE_GPU_INIT() TracyGpuContext
#define PROFILE_GPU_FRAME() TracyGpuCollect
#define PROFILE_GPU_ZONE(name) TracyGpuZone(name)
#else
#define PROFILE_GPU_INIT()
#define PROFILE_GPU_FRAME()
#define PROFILE_GPU_ZONE(name)
#endif

#else

    // ������ �������� ��� ����� ��� Tracy (TRACY_ENABLE=OFF)
#define PROFILE_FRAME
#define PROFILE_FUNCTION()
#define PROFILE_FUNCTION_N(name)
#define PROFILE_SCOPE(name)
#define PROFILE_THREAD(name)
#define PROFILE_TEXT(text, size)
#define PROFILE_LOG(text, size)
#define PROFILE_PLOT(name, val)
#define PROFILE_ALLOC(ptr, size)
#define PROFILE_FREE(ptr)
#define PROFILE_GPU_INIT()
#define PROFILE_GPU_FRAME()
#define PROFILE_GPU_ZONE(name)

#endif

namespace Doriax {

    class DORIAX_API Profiler {
    public:
        // ������������� �������������� ������ (��������, GPU)
        static void Init();

        // ���������� ��������� ���������� (��������, ���� GPU ���������)
        static void BeginFrame();
        static void EndFrame();

        // ������� C++ ������-������� ��� ����������� � ���������
        static void Log( const std::string& message );
        static void Plot( const char* name, int64_t value );
        static void Plot( const char* name, double value );
    };

} // namespace Doriax