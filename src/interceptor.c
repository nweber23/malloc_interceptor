#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

/* Function pointers to real allocation functions */
static void *(*real_malloc)(size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void (*real_free)(void *) = NULL;

/* Atomic allocation counter (absolute, includes init) */
static _Atomic uint64_t alloc_count = 0;
/* base_count stores how many allocations happened during init (so we offset them) */
static _Atomic uint64_t base_count = 0;

/* Configuration parsed from env */
#define MAX_FAIL_ENTRIES 512
static uint64_t fail_points[MAX_FAIL_ENTRIES];
static size_t fail_points_count = 0;
static _Atomic uint64_t fail_every = 0; /* 0 means disabled */

/* Extra config */
static _Atomic int64_t fail_offset = 0; /* signed offset applied to visible index */
static _Atomic int debug_mode = 0;      /* if set, write diagnostics when making fail decision */

/* Helper: parse comma-separated numbers/ranges, e.g. "5,10-12,20" */
static void parse_fail_at(const char *s) {
    if (!s) return;
    char *copy = strdup(s);
    if (!copy) return;
    char *tok = strtok(copy, ",");
    while (tok && fail_points_count < MAX_FAIL_ENTRIES) {
        while (*tok == ' ') tok++;
        char *dash = strchr(tok, '-');
        if (dash) {
            *dash = '\0';
            uint64_t a = strtoull(tok, NULL, 10);
            uint64_t b = strtoull(dash + 1, NULL, 10);
            if (a == 0 || b == 0) {
                /* ignore zero or invalid ranges */
            } else {
                if (a > b) { uint64_t t = a; a = b; b = t; }
                for (uint64_t v = a; v <= b && fail_points_count < MAX_FAIL_ENTRIES; ++v) {
                    fail_points[fail_points_count++] = v;
                    if (v == UINT64_MAX) break; /* avoid overflow */
                }
            }
        } else {
            uint64_t v = strtoull(tok, NULL, 10);
            if (v > 0 && fail_points_count < MAX_FAIL_ENTRIES)
                fail_points[fail_points_count++] = v;
        }
        tok = strtok(NULL, ",");
    }
    free(copy);
}

/* Check whether current visible index (after applying offset) should fail. */
static int should_fail(uint64_t visible_index) {
    if (visible_index == 0) return 0;
    int64_t offset = atomic_load(&fail_offset);
    int64_t adjusted = (int64_t)visible_index + offset;
    if (adjusted <= 0) return 0; /* adjusted indices <= 0 are never considered */
    uint64_t adj = (uint64_t) adjusted;

    uint64_t e = atomic_load(&fail_every);
    if (e > 0 && (adj % e) == 0) return 1;
    for (size_t i = 0; i < fail_points_count; ++i) {
        if (fail_points[i] == adj) return 1;
    }
    return 0;
}

/* Initialize orig functions and parse env (constructor) */
__attribute__((constructor))
static void init_malloc_fail(void) {
    /* parse env first so behavior starts immediately */
    const char *env_at = getenv("MALLOC_FAIL_AT");
    const char *env_every = getenv("MALLOC_FAIL_EVERY");
    const char *env_offset = getenv("MALLOC_FAIL_OFFSET");
    const char *env_debug = getenv("MALLOC_FAIL_DEBUG");

    if (env_at) parse_fail_at(env_at);
    if (env_every) {
        uint64_t v = strtoull(env_every, NULL, 10);
        if (v > 0) atomic_store(&fail_every, v);
    }
    if (env_offset) {
        long long o = strtoll(env_offset, NULL, 10);
        atomic_store(&fail_offset, (int64_t)o);
    }
    if (env_debug) atomic_store(&debug_mode, 1);

    /* load the real functions */
    /* Use RTLD_NEXT to find the next occurrence of these symbols */
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    real_free = dlsym(RTLD_NEXT, "free");

    /* record how many allocations have already been observed during init/setup */
    uint64_t seen = atomic_load(&alloc_count);
    atomic_store(&base_count, seen);

    /* If dlsym failed, print a warning (but keep going). Use write() to avoid malloc recursion. */
    if (!real_malloc || !real_calloc || !real_realloc || !real_free) {
        const char *msg = "interceptor: warning: dlsym failed to load real allocators\n";
        write(2, msg, strlen(msg));
    }
}

/* Interposed malloc/calloc/realloc/free */

/* Helper to compute visible index (1-based) for allocations after init.
 * Returns 0 if the allocation is considered "pre-init" and not visible.
 */
static inline uint64_t compute_visible_index(uint64_t absolute_count) {
    uint64_t base = atomic_load(&base_count);
    if (absolute_count > base) return absolute_count - base;
    return 0;
}

/* Helper to emit debug info when a fail decision is made */
static void emit_decision_debug(const char *fn, uint64_t absolute_count, uint64_t visible, int decision) {
    if (!atomic_load(&debug_mode)) return;
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
                       "interceptor: %s abs=%" PRIu64 " vis=%" PRIu64 " offset=%lld decision=%d\n",
                       fn, absolute_count, visible, (long long)atomic_load(&fail_offset), decision);
    if (len > 0) write(2, buf, (size_t)len);
}

void *malloc(size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1; /* absolute count (includes init) */
    uint64_t visible = compute_visible_index(c);

    int will_fail = 0;
    if (visible > 0) will_fail = should_fail(visible);

    emit_decision_debug("malloc", c, visible, will_fail);

    if (will_fail) {
        errno = ENOMEM;
        return NULL;
    }

    if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
    return real_malloc ? real_malloc(size) : NULL;
}

void *calloc(size_t nmemb, size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1;
    uint64_t visible = compute_visible_index(c);

    int will_fail = 0;
    if (visible > 0) will_fail = should_fail(visible);

    emit_decision_debug("calloc", c, visible, will_fail);

    if (will_fail) {
        errno = ENOMEM;
        return NULL;
    }

    if (!real_calloc) real_calloc = dlsym(RTLD_NEXT, "calloc");
    return real_calloc ? real_calloc(nmemb, size) : NULL;
}

void *realloc(void *ptr, size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1;
    uint64_t visible = compute_visible_index(c);

    int will_fail = 0;
    if (visible > 0) will_fail = should_fail(visible);

    emit_decision_debug("realloc", c, visible, will_fail);

    if (will_fail) {
        errno = ENOMEM;
        return NULL;
    }

    if (!real_realloc) real_realloc = dlsym(RTLD_NEXT, "realloc");
    return real_realloc ? real_realloc(ptr, size) : NULL;
}

void free(void *ptr) {
    if (!real_free) real_free = dlsym(RTLD_NEXT, "free");
    if (real_free) real_free(ptr);
}
