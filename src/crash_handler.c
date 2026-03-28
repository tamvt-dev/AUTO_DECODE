#include "../include/crash_handler.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>

static LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS *exinfo) {
    log_fatal("Windows Exception: Code 0x%08lx at address 0x%p",
              exinfo->ExceptionRecord->ExceptionCode,
              exinfo->ExceptionRecord->ExceptionAddress);
    
    log_fatal("Exception Information:");
    log_fatal("  Exception Code: %lu", exinfo->ExceptionRecord->ExceptionCode);
    log_fatal("  Exception Flags: %lu", exinfo->ExceptionRecord->ExceptionFlags);
    log_fatal("  Exception Address: %p", exinfo->ExceptionRecord->ExceptionAddress);
    
    log_close();
    return EXCEPTION_EXECUTE_HANDLER;
}

static void crash_handler(int sig) {
    log_fatal("CRASH: Signal %d received", sig);
    
    switch(sig) {
        case SIGABRT:
            log_fatal("  - Abort signal");
            break;
        case SIGFPE:
            log_fatal("  - Floating point exception");
            break;
        case SIGILL:
            log_fatal("  - Illegal instruction");
            break;
        case SIGSEGV:
            log_fatal("  - Segmentation fault");
            break;
        default:
            log_fatal("  - Unknown signal");
            break;
    }
    
    log_close();
    exit(1);
}

void install_crash_handler(void) {
    SetUnhandledExceptionFilter(windows_exception_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGSEGV, crash_handler);
    log_info("Crash handler installed (Windows)");
}

#else
#include <execinfo.h>

static void crash_handler(int sig) {
    void *array[50];
    size_t size;
    
    log_fatal("CRASH: Signal %d received", sig);
    
    size = backtrace(array, 50);
    char **strings = backtrace_symbols(array, size);
    
    log_fatal("Backtrace:");
    for (size_t i = 0; i < size; i++) {
        log_fatal("  %s", strings[i]);
    }
    
    free(strings);
    log_close();
    exit(1);
}

void install_crash_handler(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGILL, crash_handler);
    log_info("Crash handler installed (Unix)");
}

#endif