#include <stdio.h>
#include "memmon.h"

int main(void) {
    // initialize monitor with default 2048 KB
    memmon_init(2048);
    fprintf(stderr, "TEST: memmon init 2048 KB\n");

    // allocate ~91% of 2048KB to trigger ALERTA
    size_t almost = (size_t)(2048UL * 1024UL * 0.91);
    void *p = mm_malloc(almost);
    if (!p) { fprintf(stderr, "alloc failed\n"); return 2; }
    fprintf(stderr, "TEST: allocated ~91%% (%lu bytes) -> expect ALERTA\n", (unsigned long)almost);

    // free and allocate >100% to force ERRO (exit)
    mm_free(p);
    size_t too_much = (size_t)(2048UL * 1024UL + 1000);
    fprintf(stderr, "TEST: allocating too much (%lu bytes) -> expect ERRO and exit\n", (unsigned long)too_much);
    void *q = mm_malloc(too_much);
    if (!q) {
        fprintf(stderr, "TEST: mm_malloc returned NULL as expected for over-limit\n");
        memmon_report_peak();
        return 1; // expected error
    }
    // if allocation succeeded unexpectedly, free and report
    mm_free(q);
    memmon_report_peak();
    return 2; // unexpected success
}
