#ifndef ANIMTOOL_LOG_H
#define ANIMTOOL_LOG_H

#include <cstdio>
#include <cstdarg>

namespace logger {
    enum Level {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR        
    };

    inline Level level = LOG_INFO;

    static inline void vprint(Level l, const char* format, va_list ap) {
        if (l < level) return;

        char buf[1024] = {};
        int n = vsnprintf(buf, sizeof(buf), format, ap);
        if (n >= 0)
            fprintf(stderr, "%s\n", buf);
    }

    static inline void d(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        vprint(LOG_DEBUG, format, ap);
        va_end(ap);
    }

    static inline void i(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        vprint(LOG_INFO, format, ap);
        va_end(ap);
    }

    static inline void w(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        vprint(LOG_WARN, format, ap);
        va_end(ap);
    }

    static inline void e(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        vprint(LOG_ERROR, format, ap);
        va_end(ap);
    }
}

#endif // ANIMTOOL_LOG_H
