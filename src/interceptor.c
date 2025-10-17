#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

/* Function pointers to real allocation functions */
static void *(*real_malloc)(size_t) = NULL;
static void *(*real_calloc)(size_t, size_t) = NULL;
static void *(*real_realloc)(void *, size_t) = NULL;
static void (*real_free)(void *) = NULL;

/* Atomic allocation counter */
static _Atomic uint64_t alloc_count = 0;

/* Configuration parsed from env */
#define MAX_FAIL_ENTRIES 256
static uint64_t fail_points[MAX_FAIL_ENTRIES];
static size_t fail_points_count = 0;
static _Atomic uint64_t fail_every = 0; /* 0 means disabled */

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
            if (a > b) { uint64_t t = a; a = b; b = t; }
            for (uint64_t v = a; v <= b && fail_points_count < MAX_FAIL_ENTRIES; ++v) {
                fail_points[fail_points_count++] = v;
                if (v == UINT64_MAX) break; /* avoid overflow */
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

/* Check whether current count should fail. */
static int should_fail(uint64_t count) {
    uint64_t e = atomic_load(&fail_every);
    if (e > 0 && (count % e) == 0) return 1;
    for (size_t i = 0; i < fail_points_count; ++i) {
        if (fail_points[i] == count) return 1;
    }
    return 0;
}

/* Initialize orig functions and parse env (constructor) */
__attribute__((constructor))
static void init_malloc_fail(void) {
    /* parse env first so behavior starts immediately */
    const char *env_at = getenv("MALLOC_FAIL_AT");
    const char *env_every = getenv("MALLOC_FAIL_EVERY");
    if (env_at) parse_fail_at(env_at);
    if (env_every) {
        uint64_t v = strtoull(env_every, NULL, 10);
        if (v > 0) atomic_store(&fail_every, v);
    }

    /* load the real functions */
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    real_free = dlsym(RTLD_NEXT, "free");

    /* If dlsym failed, print a warning (but keep going) */
    if (!real_malloc || !real_calloc || !real_realloc || !real_free) {
        /* Avoid using fprintf which may call malloc; use write */
        const char *msg = "malloc_fail: warning: dlsym failed to load real allocators\n";
        write(2, msg, strlen(msg));
    }
}

/* Interposed malloc/calloc/realloc/free */

void *malloc(size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1; /* counts start at 1 */
    if (should_fail(c)) {
        /* simulate allocation failure */
        return NULL;
    }
    if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
    return real_malloc ? real_malloc(size) : NULL;
}

void *calloc(size_t nmemb, size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1;
    if (should_fail(c)) {
        return NULL;
    }
    if (!real_calloc) real_calloc = dlsym(RTLD_NEXT, "calloc");
    return real_calloc ? real_calloc(nmemb, size) : NULL;
}

void *realloc(void *ptr, size_t size) {
    uint64_t c = atomic_fetch_add(&alloc_count, 1) + 1;
    if (should_fail(c)) {
        /* In real realloc, returning NULL keeps original ptr intact */
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
