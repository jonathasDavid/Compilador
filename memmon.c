// Minimal memory monitor: wrappers around malloc/realloc/free that track current and peak allocations
#include "memmon.h"
#include <stdlib.h>
#include <stdio.h>

static size_t mem_limit_bytes = 0;
static size_t mem_current = 0;
static size_t mem_peak = 0;
static int memmon_over_limit = 0;

void memmon_init(size_t limit_kb) {
    mem_limit_bytes = limit_kb * 1024UL;
    mem_current = 0;
    mem_peak = 0;
}

void memmon_report_peak(void) {
    fprintf(stderr, "MEMMON: peak usage %lu bytes, current %lu bytes, limit %lu bytes\n", (unsigned long)mem_peak, (unsigned long)mem_current, (unsigned long)mem_limit_bytes);
}

// We store allocation sizes in a small header before the returned pointer.
// Layout: [size_t sz][user data]
#define HDR_SIZE sizeof(size_t)

static void handle_alloc_change(long delta) {
    if (delta > 0) mem_current += (size_t)delta;
    else mem_current -= (size_t)(-delta);
    if (mem_current > mem_peak) mem_peak = mem_current;
    if (mem_limit_bytes > 0) {
        double pct = (mem_current * 100.0) / (double)mem_limit_bytes;
        if (pct >= 100.0) {
            fprintf(stderr, "ERRO: Memoria Insuficiente: uso %lu bytes >= limite %lu bytes\n", (unsigned long)mem_current, (unsigned long)mem_limit_bytes);
            // mark over-limit; caller will receive NULL from mm_malloc/mm_realloc
            memmon_over_limit = 1;
        } else if (pct >= 90.0) {
            fprintf(stderr, "ALERTA: Uso de memoria em %.1f%% do limite (%lu/%lu bytes)\n", pct, (unsigned long)mem_current, (unsigned long)mem_limit_bytes);
        }
    // clear over-limit flag if we've dropped below the limit
    if (mem_current < mem_limit_bytes) memmon_over_limit = 0;
    }
}

void *mm_malloc(size_t sz) {
    size_t total = sz + HDR_SIZE;
    void *p = malloc(total);
    if (!p) return NULL;
    *((size_t*)p) = sz;
    void *user = (char*)p + HDR_SIZE;
    handle_alloc_change((long)sz);
    if (memmon_over_limit) {
    // undo accounting, free allocation and return NULL to caller so it can handle OOM
    handle_alloc_change(-((long)sz));
    free(p);
        return NULL;
    }
    return user;
}

void *mm_realloc(void *ptr, size_t sz) {
    if (!ptr) return mm_malloc(sz);
    void *real = (char*)ptr - HDR_SIZE;
    size_t oldsz = *((size_t*)real);
    size_t total = sz + HDR_SIZE;
    void *p = realloc(real, total);
    if (!p) return NULL;
    *((size_t*)p) = sz;
    void *user = (char*)p + HDR_SIZE;
    handle_alloc_change((long)sz - (long)oldsz);
    if (memmon_over_limit) {
        // undo accounting for realloc change, free and return NULL
        handle_alloc_change(-((long)sz - (long)oldsz));
        free(p);
        return NULL;
    }
    return user;
}

void mm_free(void *ptr) {
    if (!ptr) return;
    void *real = (char*)ptr - HDR_SIZE;
    size_t sz = *((size_t*)real);
    handle_alloc_change(-((long)sz));
    free(real);
}

size_t memmon_current_bytes(void) { return mem_current; }
size_t memmon_peak_bytes(void) { return mem_peak; }
