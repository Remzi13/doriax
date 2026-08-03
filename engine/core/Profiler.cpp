#include "Profiler.h"

namespace Doriax {

    void Profiler::Init() {
#if PROFILE_ENABLED
        PROFILE_THREAD( "Main Thread" );

#if HAS_TRACY_GPU
        PROFILE_GPU_INIT();
#endif
#endif
    }

    void Profiler::BeginFrame() {
        
    }

    void Profiler::EndFrame() {
#if PROFILE_ENABLED
        PROFILE_FRAME;

#if HAS_TRACY_GPU
        PROFILE_GPU_FRAME();
#endif
#endif
    }

    void Profiler::Log( const std::string& message ) {
#if PROFILE_ENABLED
        TracyMessage( message.c_str(), message.size() );
#endif
    }

    void Profiler::Plot( const char* name, int64_t value ) {
#if PROFILE_ENABLED
        TracyPlot( name, value );
#endif
    }

    void Profiler::Plot( const char* name, double value ) {
#if PROFILE_ENABLED
        TracyPlot( name, value );
#endif
    }

} // namespace Doriax