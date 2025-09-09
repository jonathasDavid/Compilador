// Simple memory monitor wrapper
#ifndef MEMMON_H
#define MEMMON_H

#include <stddef.h>

void memmon_init(size_t limit_kb);
void memmon_report_peak(void);

void *mm_malloc(size_t sz);
void *mm_realloc(void *ptr, size_t sz);
void mm_free(void *ptr);

size_t memmon_current_bytes(void);
size_t memmon_peak_bytes(void);

#endif // MEMMON_H
