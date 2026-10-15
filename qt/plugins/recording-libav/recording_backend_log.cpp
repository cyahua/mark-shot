#include "recording_backend_log.h"

#include "debug_log.h"

#include <atomic>

namespace {
std::atomic<markshot::recording::RecordingBackendLog> logSink{nullptr};
}

void markshot::recording::setRecordingBackendLog(RecordingBackendLog sink)
{
    logSink.store(sink, std::memory_order_release);
}

void markshot::debugLogV(const char *category, const char *format, va_list args)
{
    if (const auto sink = logSink.load(std::memory_order_acquire)) {
        sink(category, format, args);
    }
}

void markshot::debugLog(const char *category, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    debugLogV(category, format, args);
    va_end(args);
}
