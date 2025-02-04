#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <stdio.h>
#pragma once

#define LOG(prefix, ...) __log_impl(prefix, __FUNCTION__, __FILE_NAME__, __LINE__, __VA_ARGS__)
#define LOGV(prefix, ...) __logv_impl(prefix, __FUNCTION__, __FILE_NAME__, __LINE__, __VA_ARGS__)

void __attribute__((format(printf, 5, 6))) __log_impl(
    const char *prefix,
    const char *function,
    const char *file,
    int line,
    const char *format,
    ...
);

void __logv_impl(
    const char *prefix,
    const char *function,
    const char *file,
    int line,
    const char *format,
    va_list args
);

#endif /** UTIL_LOG_H */