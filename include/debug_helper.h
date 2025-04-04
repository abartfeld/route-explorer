#pragma once

#include <QDebug>
#include <QString>
#include <csignal>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <unistd.h>

/**
 * Helper class for debugging segmentation faults
 * Installs a signal handler that prints a stack trace when a segfault occurs
 */
class DebugHelper {
public:
    static void installSignalHandlers() {
        std::signal(SIGSEGV, signalHandler);
        std::signal(SIGABRT, signalHandler);
        qDebug() << "DebugHelper: Signal handlers installed for segfault debugging";
    }
    
    static void printCurrentStackTrace() {
        void *array[50];
        size_t size = backtrace(array, 50);
        
        qDebug() << "========= Stack Trace =========";
        
        // Skip first frame (this function)
        for (size_t i = 1; i < size; i++) {
            Dl_info info;
            if (dladdr(array[i], &info)) {
                // Try to demangle the symbol name
                int status;
                char* demangled = abi::__cxa_demangle(info.dli_sname ? info.dli_sname : "", NULL, NULL, &status);
                
                QString symbolName = demangled ? demangled : (info.dli_sname ? info.dli_sname : "??");
                QString objPath = info.dli_fname ? info.dli_fname : "??";
                
                ptrdiff_t offset = static_cast<char*>(array[i]) - static_cast<char*>(info.dli_saddr);
                
                // Fix: Convert void* to quintptr before using in QString::arg()
                quintptr addrValue = reinterpret_cast<quintptr>(array[i]);
                
                qDebug().noquote() << QString("#%1: %2 in %3 (0x%4 + 0x%5)")
                    .arg(i)
                    .arg(symbolName)
                    .arg(objPath)
                    .arg(addrValue, 0, 16)  // Fixed: use quintptr instead of void*
                    .arg(offset, 0, 16);
                
                if (demangled) {
                    free(demangled);
                }
            } else {
                // Fix: Convert void* to quintptr before using in QString::arg()
                quintptr addrValue = reinterpret_cast<quintptr>(array[i]);
                
                qDebug().noquote() << QString("#%1: ?? (0x%2)")
                    .arg(i)
                    .arg(addrValue, 0, 16);  // Already using quintptr
            }
        }
        qDebug() << "================================";
    }

private:
    static void signalHandler(int sig) {
        std::signal(sig, SIG_DFL); // Reset handler
        
        // Print signal info
        qCritical() << "!!!!! CRASH DETECTED !!!!!";
        qCritical() << "Signal:" << sig << (sig == SIGSEGV ? "(SIGSEGV)" : sig == SIGABRT ? "(SIGABRT)" : "");
        
        // Print stack trace
        printCurrentStackTrace();
        
        // Reset signal and let program crash normally
        std::raise(sig);
    }
};
